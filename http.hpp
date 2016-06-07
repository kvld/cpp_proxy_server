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
#include <unordered_map>

class http_protocol {
public:
    
    enum state_t {
        FULL, BAD, START_LINE, HEADERS, EMPTY, PARTIAL
    };
    
    http_protocol();
    http_protocol(std::string);
    virtual ~http_protocol() { }
    
    std::string get_data();
    
    void set_header(std::string key, std::string val);
    std::string get_header(std::string key);
    
    void append(std::string&);
    
    bool is_ended();
    
protected:
    void parse_data();
    virtual void parse_start_line(std::string) = 0;
    void parse_headers(std::string);
    void check_body();
    
    virtual std::string get_start_line() = 0;
    std::string get_headers();
    std::string get_body();
    
    state_t state;
    std::string protocol, data;
    size_t body_begin_pos;
    std::unordered_map<std::string, std::string> headers;
};

class http_request : public http_protocol {
public:
    http_request(std::string);
    
    std::string get_host();
    std::string get_port();
    sockaddr resolve_host();
    
    std::string get_relative_URI();
    
private:
    void parse_start_line(std::string) override;
    std::string get_start_line() override;

    std::string port, host, URI, method;
    sockaddr resolved_host;
    bool is_host_resolved = false;
};

class http_response : public http_protocol {
public:
    http_response();
    http_response(std::string);
    
private:
    void parse_start_line(std::string) override;
    std::string get_start_line() override;
    
    std::string status;
};

#endif /* http_hpp */
