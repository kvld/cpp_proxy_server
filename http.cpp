    //
//  http.cpp
//  proxy
//
//  Created by Vladislav Kiryukhin on 31.05.16.
//  Copyright © 2016 Vladislav Kiryukhin. All rights reserved.
//

#include "http.hpp"

#include <iostream>
#include <netdb.h>

request::request(std::string content) : buffer(content), is_host_resolved(false) { }

request::~request() { }

std::string request::append(std::string content) {
    this->buffer.append(content);
    return this->buffer;
}

std::string request::get_port() {
    if (this->port.empty()) {
        this->get_host();
    }
    
    return this->port;
}

std::string request::get_host() {
    if (this->host.empty()) {
        size_t pos_host = this->buffer.find("Host:");
        if (pos_host == std::string::npos) {
            this->host = "";
            this->port = "";
        } else {
            // "Host: hostname.com:80\r\n" -> "hostname.com:80"
            this->host = this->buffer.substr(pos_host + 6, this->buffer.find("\r\n", pos_host) - pos_host - 6);
            // "hostname.com:80" -> "hostname.com"
            this->port = (this->host.find(":") == std::string::npos) ? "80" : this->host.substr(this->host.find(":") + 1);
            this->host = (this->host.find(":") == std::string::npos) ? this->host : this->host.substr(0, this->host.find(":"));
        }
    }
    
    return this->host;
}

sockaddr request::resolve_host() {
    if (is_host_resolved) {
        return this->resolved_host;
    }
    
    if (this->get_host() == "") {
        // exception
    }
    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof(hints));
        
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    
    int err_no = getaddrinfo(this->get_host().c_str(), this->get_port().c_str(), &hints, &res);
    
    if (err_no != 0) {
        std::cerr << "Error while resolving!" << std::endl;
        // exception
        
    }
    
    this->resolved_host = *res->ai_addr;
    
    freeaddrinfo(res);
    
    is_host_resolved = true;
    return this->resolved_host;
}