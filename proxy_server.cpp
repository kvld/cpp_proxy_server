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

#include <netdb.h>
#include <arpa/inet.h>
#include <iostream>

proxy_server::proxy_server(int port) :
    _is_working(false),
    _is_stopped(false),
    queue(),
    main_socket(socket::create(port))
{
    queue.add_event([this](struct kevent& ev) { this->connect_client(ev); }, main_socket.get_fd(), EVFILT_READ, EV_ADD, 0, NULL);
}

proxy_server::~proxy_server() {
    
}

void proxy_server::run() {
    std::cout << "Server started." << std::endl;
    
    _is_working = true;
    
    try {
        while (is_working()) {
            int amount = queue.event_occurred();
        
            if (amount == -1) {
                std::cerr << "Error while working occurred!" << std::endl;
            } else {
                queue.execute_events();
            }
        }
    } catch (...) {
        terminate();
    }
}

void proxy_server::start() {
    std::cout << "Server started." << std::endl;
    
    _is_working = true;
}

void proxy_server::stop() {
    std::cout << "Server stopped." << std::endl;
    
    _is_stopped = true;
}

void proxy_server::terminate() {
    std::cout << "Server terminated." << std::endl;
    
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
    std::cout << "New client connected." << std::endl;
    client* new_client = new client(main_socket.get_fd());
    clients[new_client->get_fd()] = std::move(std::unique_ptr<client>(new_client));
    std::cout << "New client accepted, fd = " << new_client->get_fd() << std::endl;
    
    queue.add_event([this](struct kevent& ev) { this->read_from_client(ev); }, new_client->get_fd(), EVFILT_READ, EV_ADD, 0, NULL);
    queue.add_event([this](struct kevent& ev) { this->disconnect_client(ev); }, new_client->get_fd(), EVFILT_TIMER, EV_ADD, NOTE_SECONDS, 600);
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
    
    size_t read_cnt = cur_client->read(ev.data);
    std::cout << "Read data from client, fd = " << ev.ident << ", ev_size = " << ev.data << ", size = " << read_cnt << std::endl;
    
    class request cur_request(cur_client->get_buffer());
    
    if (cur_request.is_ended()) {
        std::cout << "Request for host [" << cur_request.get_host() << "]" << std::endl;
        
        class response* response = new class response();
        cur_client->set_response(response);
        
        if (cur_client->has_server()) {
            if (cur_client->get_host() == cur_request.get_host()) {
                cur_client->flush_client_buffer();
                queue.add_event([this](struct kevent& ev) { this->write_to_server(ev); }, cur_client->get_server_fd(), EVFILT_WRITE, EV_ADD | EV_ENABLE, 0, NULL);
                return;
            } else {
                cur_client->unbind();
            }
        }
        
        class server* server;
        
        try {
            struct sockaddr result = std::move(cur_request.resolve_host());
            server = new class server(result);
        } catch (...) {
            throw server_exception("Error while connecting to server!");
        }
        
        servers[server->get_fd()] = server;
        server->set_host(cur_request.get_host());
        cur_client->bind(server);
        std::cout << "Server with fd = " << server->get_fd() << " binded to client with fd = " << cur_client->get_fd() << std::endl;
        
        queue.add_event([this](struct kevent& kev) { this->write_to_server(kev); }, server->get_fd(), EVFILT_WRITE, EV_ADD, 0, NULL);
        cur_client->flush_client_buffer();
    }
    
}

void proxy_server::write_to_server(struct kevent& ev) {
    if (ev.flags & EV_EOF && ev.data == 0) {
        disconnect_server(ev);
        return;
    }

    std::cout << "Writing data to server, fd = " << ev.ident << std::endl;
    class server* cur_server = servers[ev.ident];
    
    int error;
    socklen_t length = sizeof(error);
    if (getsockopt(static_cast<int>(ev.ident), SOL_SOCKET, SO_ERROR, &error, &length) == -1 || error != 0) {
        std::cerr << "Need disconnect" << std::endl;
        perror("Error while connecting to server");
        disconnect_server(ev);
        return;
    }
    
    cur_server->write();
    if (cur_server->get_buffer_size() == 0) {
        queue.add_event([this](struct kevent& kev) { this->read_header_from_server(kev); }, ev.ident, EVFILT_READ, EV_ADD, 0, NULL);
        queue.delete_event(ev.ident, ev.filter);
        queue.add_event([this](struct kevent& kev) { this->write_to_client(kev); }, cur_server->get_client_fd(), EVFILT_WRITE, EV_ADD, 0, NULL);
    }
}

void proxy_server::read_header_from_server(struct kevent& ev) {
    if (ev.flags & EV_EOF && ev.data == 0) {
        disconnect_server(ev);
        return;
    }
    
    std::cout << "Reading headers from server, fd = " << ev.ident << ", size = " << ev.data << std::endl;
    
    class server* cur_server = servers[ev.ident];
    
    std::string data = cur_server->read(ev.data);
    
    class response* cur_response = clients[cur_server->get_client_fd()].get()->get_response();
    cur_response->append(data);
    
    if (cur_response->is_ended()) {
        cur_server->flush_server_buffer();
        
        queue.add_event([this](struct kevent& kev) { this->write_to_client(kev); }, cur_server->get_client_fd(), EVFILT_WRITE, EV_ADD | EV_ENABLE, 0, NULL);
        queue.add_event([this](struct kevent& kev) { this->read_from_server(kev); }, ev.ident, EVFILT_READ, EV_ADD, 0, NULL);
    }
}

void proxy_server::read_from_server(struct kevent& ev) {
    if (ev.flags & EV_EOF && ev.data == 0) {
        disconnect_server(ev);
        return;
    }
    
    std::cout << "Reading from server, fd = " << ev.ident << ", size = " << ev.data << std::endl;
    
    class server* cur_server = servers[ev.ident];
    std::string data = cur_server->read(ev.data);
    
    if (data.length() > 0) {
        class response* cur_response = clients[cur_server->get_client_fd()].get()->get_response();
        cur_response->append(data);
        
        cur_server->flush_server_buffer();
        queue.add_event(cur_server->get_client_fd(), EVFILT_WRITE, EV_ENABLE, 0, 0, NULL);
    }
    
}

void proxy_server::write_to_client(struct kevent& ev) {
    if (ev.flags & EV_EOF && ev.data == 0) {
        disconnect_client(ev);
        return;
    }
    
    std::cout << "Writing data to client, fd = " << ev.ident << std::endl;
    
    class client* cur_client = clients[ev.ident].get();
    
    cur_client->write();
    if (cur_client->has_server()) {
        cur_client->flush_server_buffer();
    }
    if (cur_client->get_buffer_size() == 0) {
        queue.add_event(ev.ident, ev.filter, EV_DISABLE, 0, 0, NULL);
    }
}

void proxy_server::disconnect_client(struct kevent& ev) {
    std::cout << "Disconnect client, fd = " << ev.ident << std::endl;
    
    class client* client = clients[ev.ident].get();
    
    if (client->has_server()) {
        std::cout << "Disconnect server, fd = " << client->get_server_fd() << std::endl;
        
        servers.erase(client->get_server_fd());
        
        queue.invalidate_events(client->get_server_fd());
        queue.delete_event(client->get_server_fd(), EVFILT_READ);
        queue.delete_event(client->get_server_fd(), EVFILT_WRITE);
    }
    queue.invalidate_events(client->get_fd());
    queue.delete_event(client->get_fd(), EVFILT_READ);
    queue.delete_event(client->get_fd(), EVFILT_WRITE);
    
    clients.erase(client->get_fd());
}

void proxy_server::disconnect_server(struct kevent& ev) {
    class server* server = servers[ev.ident];
    
    std::cout << "Disconnect server, fd = " << ev.ident << ", host = " << server->get_host() << std::endl;
    
    queue.invalidate_events(server->get_fd());
    queue.delete_event(server->get_fd(), EVFILT_READ);
    queue.delete_event(server->get_fd(), EVFILT_WRITE);
    servers.erase(server->get_fd());
    
    server->disconnect();
}

