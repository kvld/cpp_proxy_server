//
//  file_descriptor.cpp
//  proxy
//
//  Created by Vladislav Kiryukhin on 15.02.16.
//
//

#include "file_descriptor.hpp"
#include "exceptions.hpp"
#include <iostream>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

file_descriptor::file_descriptor() : fd(-1) {}

file_descriptor::file_descriptor(file_descriptor&& rhs) : fd(rhs.fd) {
    rhs.fd = -1;
}

file_descriptor& file_descriptor::operator=(file_descriptor&& rhs) {
    fd = rhs.fd;
    rhs.fd = -1;
    
    return *this;
}

file_descriptor::file_descriptor(int fd) : fd(fd) {}

file_descriptor::~file_descriptor() {
    if (fd == -1) {
        return;
    }
    
    if (close(fd) == -1) {
        fprintf(stderr, "Error while closing file descriptor!\n");
    } else {
        std::cout << "Descriptor closed, fd = " << fd << std::endl;
    }
}

void file_descriptor::set_fd(int fd) {
    this->fd = fd;
}

int file_descriptor::get_fd() {
    return this->fd;
}

void file_descriptor::make_nonblocking() {
    if (fcntl(this->get_fd(), F_SETFL, O_NONBLOCK) == -1) {
        throw server_exception("Error while making nonblocking!");
    }
}
