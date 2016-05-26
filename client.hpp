//
//  client.hpp
//  proxy
//
//  Created by Vladislav Kiryukhin on 13.05.16.
//
//

#ifndef client_hpp
#define client_hpp

#include <stdio.h>
#include <iostream>
#include <cstdlib>
#include "file_descriptor.hpp"
#include "socket.hpp"
#include "server.hpp"

class client {

public:
    client(const client&) = delete;
    client& operator=(client const&) = delete;
    
    client(int fd);
    ~client();
    
    int get_fd();
    int get_server_fd();
    bool has_server();
    
    void bind(class server* new_server);
    void unbind();
    
    std::string& get_buffer();
    size_t get_buffer_size();
    void append(std::string&);
    bool has_capacity();
    
    size_t read(size_t);
    size_t write();
    
    void flush_server_buffer();
    void flush_client_buffer();
    
    std::string get_host();
    
private:
    std::string buffer;
    class socket socket;
    std::unique_ptr<class server> server;
};

#endif /* client_hpp */
