//
//  socket.hpp
//  proxy
//
//  Created by Vladislav Kiryukhin on 18.02.16.
//
//

#ifndef socket_hpp
#define socket_hpp

#include <stdio.h>
#include <string>

#include "file_descriptor.hpp"

class socket : public file_descriptor {
public:
    socket() = default;
    socket(int fd);
    socket(socket&&) = default;
    socket& operator=(socket&&) = default;
    
    socket& operator=(const socket& rhs) = delete;
    socket(const socket&) = delete;
    
    ~socket();
    
    ssize_t write(std::string const& msg);
    std::string read(size_t buffer_size);
    
    static socket accept(int fd);
    static int create(int port);
    static socket create_server_socket();
};

#endif /* socket_hpp */