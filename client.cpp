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
    if (server) {
        unbind();
    }
    if (request) {
        request.reset(nullptr);
    }
    if (response) {
        response.reset(nullptr);
    }
}

int client::get_fd() {
    return socket.get_fd();
}

int client::get_server_fd() {
    assert(server);
    return server->get_fd();
}

bool client::has_server() {
    return server != nullptr;
}

std::string& client::get_buffer() {
    return buffer;
}

size_t client::get_buffer_size() {
    return buffer.size();
}

std::string client::get_host() {
    assert(server);
    return server->get_host();
}

size_t client::read(size_t buffer_size) {
    try {
        std::string reads = socket.read(buffer_size);
        buffer.append(reads);
        return reads.length();
    } catch (...) {
        return 0;
    }
}

size_t client::write() {
    try {
        size_t written_cnt = socket.write(buffer);
        buffer.erase(0, written_cnt);
        if (server) {
            flush_server_buffer();
        }
        return written_cnt;
    } catch (...) {
        return 0;
    }
}

void client::bind(class server *new_server) {
    server.reset(new_server);
    server->bind(this);
}

void client::unbind() {
    server.reset(nullptr);
}

void client::flush_client_buffer() {
    assert(server);
    server->append(buffer);
    buffer.clear();
}

void client::flush_server_buffer() {
    assert(server);
    if (get_buffer_size() < client::BUFFER_SIZE) {
        server->flush_server_buffer();
    }
}

void client::set_response(class http_response *rsp) {
    response.reset(rsp);
}

class http_response* client::get_response() {
    return response.get();
}

void client::set_request(class http_request *rsp) {
    request.reset(rsp);
}

class http_request* client::get_request() {
    return request.get();
}