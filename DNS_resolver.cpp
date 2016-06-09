//
//  DNS_resolver.cpp
//  proxy
//
//  Created by Vladislav Kiryukhin on 08.06.16.
//  Copyright © 2016 Vladislav Kiryukhin. All rights reserved.
//

#include "DNS_resolver.hpp"
#include "exceptions.hpp"
#include <unistd.h>
#include <mutex>
#include <netdb.h>

DNS_resolver::DNS_resolver(size_t threads_count) {
    working = true;
    int c = 0;
    try {
        for (c = 0; c < threads_count; c++) {
            threads.push_back(std::thread([this]() { this->resolve(); }));
        }
        fprintf(stdout, "All resolver threads started.\n");
    } catch (...) {
        working = false;
        condition.notify_all();
        for (int i = 0; i < c; i++) {
            if (threads[i].joinable()) {
                threads[i].join();
            }
        }
        throw;
    }
}

DNS_resolver::~DNS_resolver() {
    stop();
}

void DNS_resolver::stop() {
    std::unique_lock<std::mutex> ul{lk};
    working = false;
    condition.notify_all();
    ul.unlock();
    
    for (int i = 0; i < threads.size(); i++) {
        if (threads[i].joinable()) {
            threads[i].join();
        }
    }
}

void DNS_resolver::set_fd(int fd) {
    resolver_fd = fd;
}

int DNS_resolver::get_fd() {
    return resolver_fd;
}

void DNS_resolver::add_task(std::unique_ptr<http_request> request) {
    if (!working) {
        throw server_exception("Resolver doesn't running!");
    }
    std::unique_lock<std::mutex> ul{lk};
    std::cerr << "new task" << ' ' << request->get_host() << std::endl;
    tasks.push(std::move(request));
    condition.notify_one();
}

std::unique_ptr<http_request> DNS_resolver::get_task() {
    std::unique_lock<std::mutex> ul{lk};
    auto request = std::move(resolved.front());
    resolved.pop();
    return request;
}

void DNS_resolver::send() {
    std::unique_lock<std::mutex> ul{lk};
    char tmp = 'a';
    size_t cnt = write(get_fd(), &tmp, sizeof(tmp));
    if (cnt == -1) {
        ul.unlock();
        perror("Resolver: error while sending message to proxy server");
    }
}

void DNS_resolver::resolve() {
    while (working) {
        std::unique_lock<std::mutex> ul{lk};
        condition.wait(ul, [&]() {
            return (!tasks.empty() || !working);
        });
        
        if (!working) {
            return;
        }
        
        sockaddr resolved_host;
        
        auto request = std::move(tasks.front());
        tasks.pop();

        if (cache.contains(request->get_host())) {
            resolved_host = cache.get(request->get_host());
            ul.unlock();
        } else {
            ul.unlock();
            struct addrinfo hints, *res;
            memset(&hints, 0, sizeof(hints));
            
            hints.ai_family = AF_INET;
            hints.ai_socktype = SOCK_STREAM;
            hints.ai_flags = AI_PASSIVE;
            
            int err_no = getaddrinfo(request->get_host().c_str(), request->get_port().c_str(), &hints, &res);
            
            if (err_no != 0) {
                perror("Resolver: error while resolving");
            } else {
                resolved_host = *res->ai_addr;
                freeaddrinfo(res);
                ul.lock();
                cache.put(request->get_host(), resolved_host);
                ul.unlock();
            }
        }
        
        request->set_resolved_host(resolved_host);
        fprintf(stdout, "Host [%s] resolved!\n", request->get_host().c_str());
        
        ul.lock();
        resolved.push(std::move(request));
        ul.unlock();
        
        send();
    }
    
}