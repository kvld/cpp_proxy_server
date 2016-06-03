//
//  client.cpp
//  proxy
//
//  Created by Vladislav Kiryukhin on 13.05.16.
//
//

#include "client.hpp"
#include "socket.hpp"
#include "exceptions.hpp"
#include <cassert>

client::client(int fd) : socket(socket::accept(fd)), server(nullptr) { };

client::~client() {

}

int client::get_fd() {
    return this->socket.get_fd();
}

int client::get_server_fd() {
    assert(this->server);
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

std::string client::get_host() {
    assert(this->server);
    return this->server->get_host();
}

size_t client::read(size_t buffer_size) {
    try {
        std::string reads = this->socket.read(buffer_size);
        this->buffer.append(reads);
        return reads.length();
    } catch (...) {
        return 0;
    }
}

size_t client::write() {
    try {
        size_t written_cnt = this->socket.write(this->buffer);
        this->buffer.erase(0, written_cnt);
        if (this->server) {
            this->flush_server_buffer();
        }
        return written_cnt;
    } catch (...) {
        return 0;
    }
}

void client::bind(class server *new_server) {
    this->server.reset(new_server);
    this->server->bind(this);
}

void client::unbind() {
    this->server.reset(nullptr);
}

void client::flush_client_buffer() {
    assert(this->server);
    this->server->append(this->buffer);
    this->buffer.clear();
}

void client::flush_server_buffer() {
    assert(this->server);
    if (this->get_buffer_size() < client::BUFFER_SIZE) {
        this->server->flush_server_buffer();
    }
}

void client::set_response(class response *rsp) {
    this->response.reset(rsp);
}

class response* client::get_response() {
    return this->response.get();
}