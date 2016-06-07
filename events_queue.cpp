//
//  events_queue.cpp
//  proxy
//
//  Created by Vladislav Kiryukhin on 18.02.16.
//
//

#include "events_queue.hpp"
#include "file_descriptor.hpp"
#include "exceptions.hpp"

#include <sys/types.h>
#include <sys/event.h>
#include <sys/time.h>
#include <iostream>

events_queue::events_queue() {
    int kq = kqueue();
    if (kq == -1) {
        throw server_exception("Error while creating kqueue!");
    }
    
    this->kq.set_fd(kq);
}

events_queue::~events_queue() {}

void events_queue::add_event(const struct kevent& ev) {
    if (kevent(this->kq.get_fd(), &ev, 1, NULL, 0, NULL)) {
        throw server_exception("Error in kqueue occurred!");
    }
}

void events_queue::add_event(uintptr_t ident, int16_t filter, uint16_t flags, uint32_t fflags, intptr_t data, void *udata) {
    struct kevent ev;
    EV_SET(&ev, ident, filter, flags, fflags, data, udata);
    
    return add_event(ev);
}

void events_queue::add_event(std::function<void (struct kevent &)> handler, uintptr_t ident, int16_t filter, uint16_t flags, uint32_t fflags, intptr_t data) {
#ifdef DEBUG_KQUEUE
    fprintf(stdout, "New event: ident = %d, filter = %d\n", ident, filter);
#endif
    this->handlers[id{ident, filter}] = handler;
    return add_event(ident, filter, flags, fflags, data, NULL);
}

int events_queue::event_occurred() {
    return kevent(this->kq.get_fd(), NULL, 0, events, EVENTS_LIST_SIZE, NULL);
}

void events_queue::execute_events() {
    int cnt;
    if ((cnt = event_occurred()) == -1) {
        perror("Error while getting events count!");
    } else {
        this->invalid_events.clear();
#ifdef DEBUG_KQUEUE
        std::cout << "Events: ";
        for (int i = 0; i < cnt; i++) {
            if (this->invalid_events.count(this->events[i].ident) == 0) {
                std::cout << "(" << events[i].ident << "," << events[i].filter << ")" << ' ';
            } else {
                std::cout << "[i](" << events[i].ident << "," << events[i].filter << ")" << ' ';
            }
        }
        std::cout << std::endl;
#endif
        for (int i = 0; i < cnt; i++) {
            if (this->invalid_events.count(this->events[i].ident) == 0) {
                std::function<void(struct kevent&)> handler = this->handlers[id{events[i].ident, events[i].filter}];
                handler(events[i]);
            }
        }
    }
}

void events_queue::delete_event(uintptr_t ident, int16_t filter) {
    auto event = this->handlers.find(id{ident, filter});
    if (event != this->handlers.end()) {
        this->handlers.erase(event);
        this->add_event(ident, filter, EV_DELETE, 0, 0, NULL);
    }
}

void events_queue::invalidate_events(uintptr_t ident) {
    this->invalid_events.insert(ident);
}
