    //
//  http.cpp
//  proxy
//
//  Created by Vladislav Kiryukhin on 31.05.16.
//  Copyright © 2016 Vladislav Kiryukhin. All rights reserved.
//

#include "http.hpp"
#include "exceptions.hpp"

#include <iostream>
#include <netdb.h>


// Request
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
        throw server_exception("Invalid host!");
    }
    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof(hints));
        
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    
    int err_no = getaddrinfo(this->get_host().c_str(), this->get_port().c_str(), &hints, &res);
    
    if (err_no != 0) {
        throw server_exception("Error while resolving!");
    }
    
    this->resolved_host = *res->ai_addr;
    
    freeaddrinfo(res);
    
    is_host_resolved = true;
    return this->resolved_host;
}

bool request::is_ended() {
    size_t body_delimeter = this->buffer.find("\r\n\r\n");
    
    if (body_delimeter == std::string::npos) {
        return false;
    }
    if (this->buffer.substr(0, 4) != "POST") {
        return true;
    }
    
    size_t content_length_pos = this->buffer.find("Content-Length: ") + 16;
    size_t content_length = 0;
    while (this->buffer[content_length_pos] != '\r') {
        content_length *= 10;
        content_length += (this->buffer[content_length_pos] - '0');
        content_length_pos++;
    }
    
    return this->buffer.substr(body_delimeter + 4).length() == content_length;
}

// Response
response::response(std::string content) : buffer(content) { }

response::~response() { };

void response::append(std::string& content) {
    this->buffer.append(content);
}

bool response::is_ended() {
    return this->buffer.find("\r\n\r\n") != std::string::npos;
}
