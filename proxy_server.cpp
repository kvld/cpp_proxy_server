//
//  proxy_server.cpp
//  proxy
//
//  Created by Vladislav Kiryukhin on 19.02.16.
//
//

#include "proxy_server.hpp"
#include "client.hpp"
#include "http.hpp"
#include "server.hpp"
#include "exceptions.hpp"
#include "DNS_resolver.hpp"

#include <netdb.h>
#include <arpa/inet.h>
#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

const int RESOLVER_THREADS_COUNT = 10;
const int CLIENT_TIMEOUT = 600;

proxy_server::proxy_server(int port) :
    _is_working(false),
    _is_stopped(false),
    queue(),
    main_socket(socket::create(port)),
    resolver(RESOLVER_THREADS_COUNT)
{
    int fd[2];
    pipe(fd);
    
    pipe_fd = std::move(file_descriptor(fd[0]));
    file_descriptor resolver_fd = std::move(file_descriptor(fd[1]));
    
    pipe_fd.make_nonblocking();
    resolver_fd.make_nonblocking();
    
    resolver.set_fd(std::move(resolver_fd));
    
    queue.add_event([this](struct kevent& ev) { this->connect_client(ev); }, main_socket.get_fd(), EVFILT_READ, EV_ADD, 0, NULL);
    queue.add_event([this](struct kevent& ev) { this->resolver_callback(ev); }, pipe_fd.get_fd(), EVFILT_READ, EV_ADD, 0, NULL);
}

proxy_server::~proxy_server() {
    
}

void proxy_server::run() {
    fprintf(stdout, "Server started.\n");
    
    _is_working = true;
    
    try {
        while (is_working()) {
            int amount = queue.event_occurred();
        
            if (amount == -1) {
                fprintf(stderr, "Error in events queue occurred!\n");
            } else {
                queue.execute_events();
            }
        }
    } catch (...) {
        terminate();
    }
}

void proxy_server::start() {
    fprintf(stdout, "Server started.\n");
    _is_working = true;
}

void proxy_server::stop() {
    fprintf(stdout, "Server stopped.\n");
    
    _is_stopped = true;
}

void proxy_server::terminate() {
    fprintf(stdout, "Server terminated.\n");
    
    _is_working = false;
}

bool proxy_server::is_working() {
    return this->_is_working;
}

bool proxy_server::is_stopped() {
    return this->_is_stopped;
}

// Events
void proxy_server::connect_client(struct kevent& ev) {
    client* new_client = new client(main_socket.get_fd());
    clients[new_client->get_fd()] = std::move(std::unique_ptr<client>(new_client));
    fprintf(stdout, "New client accepted, fd = %d\n", new_client->get_fd());

    queue.add_event([this](struct kevent& ev) { this->read_from_client(ev); }, new_client->get_fd(), EVFILT_READ, EV_ADD, 0, NULL);
    queue.add_event([this](struct kevent& ev) { this->disconnect_client(ev); }, new_client->get_fd(), EVFILT_TIMER, EV_ADD, NOTE_SECONDS, CLIENT_TIMEOUT);
}

void proxy_server::read_from_client(struct kevent& ev) {
    if (ev.flags & EV_EOF && ev.data == 0) {
        disconnect_client(ev);
        return;
    }
    
    if (clients.find(ev.ident) == clients.end()) {
        throw server_exception("Client not found!");
    }
    
    client* cur_client = clients[ev.ident].get();
    
    reset_timer(cur_client->get_fd());
    
    size_t read_cnt = cur_client->read(ev.data);
    fprintf(stdout, "Read data from client, fd = %lu, ev_size = %ld, size = %zu\n", ev.ident, ev.data, read_cnt);

    std::unique_ptr<http_request> cur_request(new http_request(cur_client->get_buffer()));
    
    if (cur_request->is_ended()) {
        fprintf(stdout, "Request for host [%s]\n", cur_request->get_host().c_str());
        
        class http_response* response = new class http_response();
        cur_client->set_response(response);
        
        if (cur_client->has_server()) {
            if (cur_client->get_host() == cur_request->get_host()) {
                cur_client->get_buffer() = cur_request->get_data();
                cur_client->flush_client_buffer();
                queue.add_event([this](struct kevent& ev) { this->write_to_server(ev); }, cur_client->get_server_fd(), EVFILT_WRITE, EV_ADD | EV_ENABLE, 0, NULL);
                return;
            } else {
                cur_client->unbind();
            }
        }
        
        cur_request->set_client_fd(static_cast<int>(ev.ident));
        resolver.add_task(std::move(cur_request));
    }
}

void proxy_server::resolver_callback(struct kevent& ev) {
    char tmp;
    if (read(pipe_fd.get_fd(), &tmp, sizeof(tmp)) == -1) {
        perror("Reading from resolver failed");
    }
    
    std::unique_ptr<http_request> cur_request = resolver.get_task();
    fprintf(stdout, "Resolver callback called for host [%s].\n", cur_request->get_host().c_str());
    
    class server* server;
    
    try {
        struct sockaddr result = std::move(cur_request->get_resolved_host());
        server = new class server(result);
    } catch (...) {
        throw server_exception("Error while connecting to server!");
    }
    
    client* cur_client = clients[cur_request->get_client_fd()].get();
    
    reset_timer(cur_client->get_fd());
    
    servers[server->get_fd()] = server;
    server->set_host(cur_request->get_host());
    cur_client->bind(server);
    fprintf(stdout, "Server with fd = %d binded to client with fd = %d\n", server->get_fd(), cur_client->get_fd());
    
    queue.add_event([this](struct kevent& kev) { this->write_to_server(kev); }, server->get_fd(), EVFILT_WRITE, EV_ADD, 0, NULL);
    
    cur_client->get_buffer() = std::move(cur_request->get_data());
    cur_client->flush_client_buffer();
}

void proxy_server::write_to_server(struct kevent& ev) {
    if (ev.flags & EV_EOF && ev.data == 0) {
        disconnect_server(ev);
        return;
    }

    fprintf(stdout, "Writing data to server, fd = %lu\n", ev.ident);
    class server* cur_server = servers[ev.ident];
    
    reset_timer(cur_server->get_client_fd());
    
    int error;
    socklen_t length = sizeof(error);
    if (getsockopt(static_cast<int>(ev.ident), SOL_SOCKET, SO_ERROR, &error, &length) == -1 || error != 0) {
        perror("Error while connecting to server. Disconnecting...");
        disconnect_server(ev);
        return;
    }
    
    cur_server->write();
    if (cur_server->get_buffer_size() == 0) {
        queue.add_event([this](struct kevent& kev) { this->read_from_server(kev); }, ev.ident, EVFILT_READ, EV_ADD, 0, NULL);
        queue.delete_event(ev.ident, ev.filter);
        queue.add_event([this](struct kevent& kev) { this->write_to_client(kev); }, cur_server->get_client_fd(), EVFILT_WRITE, EV_ADD, 0, NULL);
    }
}

void proxy_server::read_from_server(struct kevent& ev) {
    if (ev.flags & EV_EOF && ev.data == 0) {
        disconnect_server(ev);
        return;
    }
    
    fprintf(stdout, "Read data from server, fd = %lu, size = %ld\n", ev.ident, ev.data);
    
    class server* cur_server = servers[ev.ident];
    
    reset_timer(cur_server->get_client_fd());
    
    std::string data = cur_server->read(ev.data);
    
    class http_response* cur_response = clients[cur_server->get_client_fd()].get()->get_response();
    cur_response->append(data);
    
    if (cur_response->is_ended()) {
        cur_server->flush_server_buffer();
        
        queue.add_event([this](struct kevent& kev) { this->write_to_client(kev); }, cur_server->get_client_fd(), EVFILT_WRITE, EV_ADD | EV_ENABLE, 0, NULL);
    }
}

void proxy_server::write_to_client(struct kevent& ev) {
    if (ev.flags & EV_EOF && ev.data == 0) {
        disconnect_client(ev);
        return;
    }
    
    fprintf(stdout, "Writing data to client, fd = %lu\n", ev.ident);
    
    class client* cur_client = clients[ev.ident].get();
    
    reset_timer(cur_client->get_fd());
    
    cur_client->write();
    if (cur_client->has_server()) {
        cur_client->flush_server_buffer();
    }
    if (cur_client->get_buffer_size() == 0) {
        queue.add_event(ev.ident, ev.filter, EV_DISABLE, 0, 0, NULL);
    }
}

void proxy_server::disconnect_client(struct kevent& ev) {
    fprintf(stdout, "Disconnect client, fd = %lu\n", ev.ident);
    
    class client* client = clients[ev.ident].get();
    
    if (client->has_server()) {
        fprintf(stdout, "Disconnect server, fd = %d\n", client->get_server_fd());
        
        servers.erase(client->get_server_fd());
        
        queue.invalidate_events(client->get_server_fd());
        queue.delete_event(client->get_server_fd(), EVFILT_READ);
        queue.delete_event(client->get_server_fd(), EVFILT_WRITE);
    }
    queue.invalidate_events(client->get_fd());
    queue.delete_event(client->get_fd(), EVFILT_READ);
    queue.delete_event(client->get_fd(), EVFILT_WRITE);
    queue.delete_event(client->get_fd(), EVFILT_TIMER);
    
    clients.erase(client->get_fd());
}

void proxy_server::disconnect_server(struct kevent& ev) {
    class server* server = servers[ev.ident];
    
    fprintf(stdout, "Disconnect server, fd = %lu, host = [%s]\n", ev.ident, server->get_host().c_str());
    
    queue.invalidate_events(server->get_fd());
    queue.delete_event(server->get_fd(), EVFILT_READ);
    queue.delete_event(server->get_fd(), EVFILT_WRITE);
    servers.erase(server->get_fd());
    
    reset_timer(server->get_client_fd());
    
    server->disconnect();
}

void proxy_server::reset_timer(int fd) {
    queue.add_event(fd, EVFILT_TIMER, EV_DELETE, NOTE_SECONDS, CLIENT_TIMEOUT, NULL);
    queue.add_event(fd, EVFILT_TIMER, EV_ADD, NOTE_SECONDS, CLIENT_TIMEOUT, NULL);
}

