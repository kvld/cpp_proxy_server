//
//  DNS_resolver.hpp
//  proxy
//
//  Created by Vladislav Kiryukhin on 08.06.16.
//  Copyright © 2016 Vladislav Kiryukhin. All rights reserved.
//

#ifndef DNS_resolver_hpp
#define DNS_resolver_hpp

#include <stdio.h>
#include <iostream>
#include "lru_cache.hpp"
#include "file_descriptor.hpp"
#include "http.hpp"
#include <netdb.h>
#include <thread>
#include <mutex>
#include <vector>
#include <queue>

class DNS_resolver {
public:
    DNS_resolver(size_t thread_count = 2);
    ~DNS_resolver();
    
    void resolve();
    void send();
    void stop();
    
    void add_task(std::unique_ptr<http_request>);
    std::unique_ptr<http_request> get_task();
    
    void set_fd(int fd);
    int get_fd();
private:
    bool working = false;
    std::mutex lk;
    std::condition_variable condition;
    std::vector<std::thread> threads;
    lru_cache<std::string, sockaddr> cache;
    std::queue<std::unique_ptr<http_request> > tasks, resolved;
    int resolver_fd;
    
};

#endif /* DNS_resolver_hpp */
