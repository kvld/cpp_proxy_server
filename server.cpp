//
//  server.cpp
//  proxy
//
//  Created by Vladislav Kiryukhin on 13.05.16.
//
//

#include "server.hpp"
#include "socket.hpp"

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
    return this->client->get_fd();
}

void server::set_host(const std::string& host) {
    this->host = host;
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
        size_t writed_cnt = this->socket.write(this->buffer);
        this->buffer.erase(0, writed_cnt);
        return writed_cnt;
    } catch (...) {
        return 0;
    }
}

std::string server::read(size_t length) {
    try {
        std::string data = this->socket.read(length);
        this->buffer.append(data);
        return data;
    } catch (...) {
        return 0;
    }
}

void server::flush_client_buffer() {
    this->client->flush_client_buffer();
}

void server::flush_server_buffer() {
    this->client->get_buffer().append(this->buffer);
    this->buffer.clear();
}

server::~server() { }

