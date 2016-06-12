//
//  proxy_server.hpp
//  proxy
//
//  Created by Vladislav Kiryukhin on 19.02.16.
//
//

#ifndef proxy_server_hpp
#define proxy_server_hpp

#include <stdio.h>

#include "events_queue.hpp"
#include "socket.hpp"
#include "client.hpp"
#include "DNS_resolver.hpp"
#include "lru_cache.hpp"
#include "http.hpp"

class proxy_server {

public:
    proxy_server(int port);
    
    proxy_server(proxy_server const& rhs) = delete;
    proxy_server& operator=(proxy_server const& rhs) = delete;
    
    ~proxy_server();
    
    void run();
    void terminate();
    
    // events
    void connect_client(struct kevent&);
    void read_from_client(struct kevent&);
    void write_to_server(struct kevent&);
    void write_to_client(struct kevent&);
    void read_from_server(struct kevent&);
    void disconnect_client(struct kevent&);
    void disconnect_server(struct kevent&);
    void resolver_callback(struct kevent&);
    
    void reset_timer(int fd);
    
private:
    bool working;
    
    events_queue queue;
    class socket main_socket;
    std::map<uintptr_t, std::unique_ptr<client> > clients;
    std::map<uintptr_t, server* > servers;
    lru_cache<std::string, http_response> cache;
    DNS_resolver resolver;
    file_descriptor pipe_fd;
};

#endif /* proxy_server_hpp */
