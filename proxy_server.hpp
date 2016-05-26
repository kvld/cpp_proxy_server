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

class proxy_server {

public:
    proxy_server(int port);
    
    proxy_server(proxy_server const& rhs) = delete;
    proxy_server& operator=(proxy_server const& rhs) = delete;
    
    ~proxy_server();
    
    void run();
    void terminate();
    
    void start();
    void stop();
    
    bool is_working();
    bool is_stopped();
    
    // events
    void connect_client(struct kevent&);
    void read_from_client(struct kevent&);
    void write_to_server(struct kevent&);
    void read_header_from_server(struct kevent&);
    void write_to_client(struct kevent&);
    void read_from_server(struct kevent&);
    void disconnect_client(struct kevent&);
    void disconnect_server(struct kevent&);

    
private:
    bool _is_working;
    bool _is_stopped;
    
    events_queue queue;
    class socket main_socket;
    std::map<uintptr_t, std::unique_ptr<client> > clients;
    std::map<uintptr_t, server* > servers;
};

#endif /* proxy_server_hpp */
