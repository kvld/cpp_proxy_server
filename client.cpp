//
//  client.cpp
//  proxy
//
//  Created by Vladislav Kiryukhin on 13.05.16.
//
//

#include "client.hpp"
#include "socket.hpp"
#include <cassert>

client::client(int fd) : socket(socket::accept(fd)), server(nullptr) { };

client::~client() {

}

int client::get_fd() {
    return this->socket.get_fd();
}

int client::get_server_fd() {
    return this->server->get_fd();
}

bool client::has_server() {
    return this->server != nullptr;
}

std::string& client::get_buffer() {
    return this->buffer;
}

size_t client::get_buffer_size() {
    return this->buffer.size();
}

size_t client::read(size_t buffer_size) {
    try {
        std::string readed = this->socket.read(buffer_size);
        this->buffer.append(readed);
        return readed.size();
    } catch (...) {
        return 0;
    }
}

size_t client::write() {
    try {
        size_t writed_cnt = this->socket.write(this->buffer);
        buffer.erase(0, writed_cnt);
        return writed_cnt;
    } catch (...) {
        return 0;
    }
}

void client::bind(class server *new_server) {
    this->server.reset(new_server);
    this->server->bind(this);
}

void client::unbind() {
    
}

void client::flush_client_buffer() {
    if (!this->server) {
        // exception
    }
    this->server->append(this->buffer);
    this->buffer.clear();
}

void client::flush_server_buffer() {
    this->server->flush_server_buffer();
}