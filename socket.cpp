//
//  socket.cpp
//  proxy
//
//  Created by Vladislav Kiryukhin on 18.02.16.
//
//

#include "socket.hpp"

#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <vector>

socket::socket(int fd) : file_descriptor(fd) {}

socket::~socket() {}

class socket socket::accept(int fd) {
    sockaddr_in addr;
    socklen_t length = sizeof(addr);
    
    int lfd;
    if ((lfd = ::accept(fd, (sockaddr*) &addr, &length)) == -1) {
        // exception
    }
    return socket(lfd);
}

ptrdiff_t socket::write(std::string const& msg) {
    ptrdiff_t len;
    if ((len = send(this->get_fd(), msg.c_str(), msg.size(), 0)) == -1) {
        // exception
    }
    
    return len;
}

std::string socket::read(size_t buffer_size) {
    std::vector<char> buffer(buffer_size);
    ptrdiff_t len;
    
    if ((len = recv(this->get_fd(), buffer.data(), buffer_size, 0)) == -1) {
        // exception
    }
    
    return std::string(buffer.cbegin(), buffer.cend() + len);
}

void socket::make_nonblocking() {
    if (fcntl(this->get_fd(), F_SETFL, O_NONBLOCK) == -1) {
        // exception
    }
}

int socket::create(int port) {
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;
     
    int new_socket = ::socket(PF_INET, SOCK_STREAM, 0);
    
    if (new_socket == -1) {
        // exception
    }
     
    int optval = 1;
    if (setsockopt(new_socket, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval)) == -1) {
        // exception
    }
     
    if (bind(new_socket, (sockaddr*) &addr, sizeof(addr)) == -1) {
        // exception
    }
    
    if (listen(new_socket, SOMAXCONN) == -1) {
        // exception
    }

    return new_socket;
}

class socket socket::create_server_socket() {
    int socket_fd = ::socket(AF_INET, SOCK_STREAM, 0);
    
    if (socket_fd == -1) {
        // exception
    }
    
    class socket server_socket(socket_fd);
    const int set = 1;
    if (setsockopt(server_socket.get_fd(), SOL_SOCKET, SO_NOSIGPIPE, &set, sizeof(set)) == -1) {
        // exception
    }
    
    server_socket.make_nonblocking();
    
    return server_socket;
}


