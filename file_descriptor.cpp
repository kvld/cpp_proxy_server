//
//  file_descriptor.cpp
//  proxy
//
//  Created by Vladislav Kiryukhin on 15.02.16.
//
//

#include "file_descriptor.hpp"
#include <iostream>
#include <stdio.h>
#include <unistd.h>

file_descriptor::file_descriptor() : fd(-1) {}

file_descriptor::file_descriptor(file_descriptor&& rhs) : fd(rhs.fd) {
    rhs.fd = -1;
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
