//
//  server.cpp
//  proxy
//
//  Created by Vladislav Kiryukhin on 13.05.16.
//
//

#include "server.hpp"
#include "socket.hpp"

#include <cassert>

server::server(struct sockaddr addr) : socket(socket::create_server_socket()) {
    if (connect(this->socket.get_fd(), &addr, sizeof(addr)) == -1) {
        if (errno != EINPROGRESS) {
            perror("Error while connecting to server occurred");
        }
    }
}

int server::get_fd() {
    return socket.get_fd();
}

int server::get_client_fd() {
    assert(client);
    return client->get_fd();
}

void server::set_host(const std::string& host) {
    this->host = host;
}

std::string server::get_host() {
    return host;
}

void server::bind(class client* new_client) {
    client = new_client;
}

void server::append(std::string& data) {
    buffer.append(data);
}

std::string& server::get_buffer() {
    return buffer;
}

size_t server::get_buffer_size() {
    return buffer.size();
}

size_t server::write() {
    try {
        size_t written_cnt = socket.write(buffer);
        buffer.erase(0, written_cnt);
        if (client) {
            this->flush_client_buffer();
        }
        return written_cnt;
    } catch (...) {
        return 0;
    }
}

std::string server::read(size_t length) {
    assert(client);
    if (client->get_buffer_size() >= client::BUFFER_SIZE) {
        return "";
    }
    try {
        std::string data = socket.read(length);
        buffer.append(data);
        return data;
    } catch (...) {
        return "";
    }
}

void server::flush_client_buffer() {
    assert(client);
    if (get_buffer_size() < server::BUFFER_SIZE) {
        client->flush_client_buffer();
    }
}

void server::flush_server_buffer() {
    assert(client);
    client->get_buffer().append(buffer);
    buffer.clear();
}

void server::disconnect() {
    assert(client);
    client->unbind();
}

server::~server() { }

