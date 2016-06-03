//
//  http.hpp
//  proxy
//
//  Created by Vladislav Kiryukhin on 31.05.16.
//  Copyright © 2016 Vladislav Kiryukhin. All rights reserved.
//

#ifndef http_hpp
#define http_hpp

#include <stdio.h>
#include <string>
#include <netdb.h>

class request {
public:
    request(std::string request);
    ~request();
    
    std::string append(std::string);
    std::string get_host();
    std::string get_port();
    sockaddr resolve_host();
    bool is_ended();
private:
    std::string buffer;
    std::string host;
    std::string port;
    sockaddr resolved_host;
    bool is_host_resolved;
};

class response {
public:
    response() = default;
    response(std::string request);
    ~response();
    
    void append(std::string&);
    bool is_ended();
    
private:
    std::string buffer;
};

#endif /* http_hpp */
