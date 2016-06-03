//
//  server.hpp
//  proxy
//
//  Created by Vladislav Kiryukhin on 13.05.16.
//
//

#ifndef server_hpp
#define server_hpp

#include <stdio.h>
#include <netinet/in.h>
#include <iostream>
#include "file_descriptor.hpp"
#include "socket.hpp"
#include "client.hpp"

class server {

public:
    
    static const size_t BUFFER_SIZE = 4096;
    
    server(const server&) = delete;
    server& operator=(const server&) = delete;
    
    server(int);
    server(sockaddr);
    ~server();
    
    int get_fd();
    int get_client_fd();
    void bind(class client* client);
    
    void append(std::string&);
    std::string& get_buffer();
    size_t get_buffer_size();
    
    std::string read(size_t);
    size_t write();
    
    void flush_server_buffer();
    void flush_client_buffer();
    
    void set_host(std::string const&);
    std::string get_host();
    
    void disconnect();
    
private:
    std::string buffer, host;
    class socket socket;
    class client* client;
    
    
};

#endif /* server_hpp */
