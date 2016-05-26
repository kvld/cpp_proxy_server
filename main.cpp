//
//  main.cpp
//  proxy
//
//  Created by Vladislav Kiryukhin on 25.05.16.
//  Copyright © 2016 Vladislav Kiryukhin. All rights reserved.
//

#include <iostream>
#include "proxy_server.hpp"

int main(int argc, const char * argv[]) {
    /*
     int kq;
     if ((kq = kqueue()) == -1) {
     exit(1);
     }
     
     int fd = open("/tmp/test", O_NONBLOCK);
     
     struct kevent ke;
     EV_SET(&ke, fd, EVFILT_VNODE, EV_ADD | EV_CLEAR, NOTE_WRITE, 0, NULL);
     if (kevent(kq, &ke, 1, NULL, 0, NULL) == -1) {
     exit(1);
     }
     
     while (true) {
     if (kevent(kq, NULL, 0, &ke, 1, NULL) != -1) {
     printf("Event occurred!\n");
     }
     }*/
    printf("Starting server at port %d...\n", 2500);
    proxy_server server(2500);
    server.run();
    return 0;
}
