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
    return this->socket.get_fd();
}

int server::get_client_fd() {
    assert(this->client);
    return this->client->get_fd();
}

void server::set_host(const std::string& host) {
    this->host = host;
}

std::string server::get_host() {
    return this->host;
}

void server::bind(class client* new_client) {
    this->client = new_client;
}

void server::append(std::string& data) {
    this->buffer.append(data);
}

std::string& server::get_buffer() {
    return this->buffer;
}

size_t server::get_buffer_size() {
    return this->buffer.size();
}

size_t server::write() {
    try {
        size_t written_cnt = this->socket.write(this->buffer);
        this->buffer.erase(0, written_cnt);
        if (this->client) {
            this->flush_client_buffer();
        }
        return written_cnt;
    } catch (...) {
        return 0;
    }
}

std::string server::read(size_t length) {
    assert(this->client);
    if (this->client->get_buffer_size() >= client::BUFFER_SIZE) {
        return "";
    }
    try {
        std::string data = this->socket.read(length);
        this->buffer.append(data);
        return data;
    } catch (...) {
        return "";
    }
}

void server::flush_client_buffer() {
    assert(this->client);
    if (this->get_buffer_size() < server::BUFFER_SIZE) {
        this->client->flush_client_buffer();
    }
}

void server::flush_server_buffer() {
    assert(this->client);
    this->client->get_buffer().append(this->buffer);
    this->buffer.clear();
}

void server::disconnect() {
    assert(this->client);
    this->client->unbind();
}

server::~server() { }

