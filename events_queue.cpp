//
//  events_queue.cpp
//  proxy
//
//  Created by Vladislav Kiryukhin on 18.02.16.
//
//

#include "events_queue.hpp"
#include "file_descriptor.hpp"

#include <sys/types.h>
#include <sys/event.h>
#include <sys/time.h>

events_queue::events_queue() {
    int kq = kqueue();
    if (kq == -1) {
        // exception;
    }
    
    this->kq.set_fd(kq);
}

events_queue::~events_queue() {}

void events_queue::add_event(const struct kevent& ev) {
    if (kevent(this->kq.get_fd(), &ev, 1, NULL, 0, NULL)) {
        throw NULL;
    }
}

void events_queue::add_event(uintptr_t ident, int16_t filter, uint16_t flags, uint32_t fflags, intptr_t data, void *udata) {
    struct kevent ev;
    EV_SET(&ev, ident, filter, flags, fflags, data, udata);
    
    return add_event(ev);
}

void events_queue::add_event(std::function<void (struct kevent &)> handler, uintptr_t ident, int16_t filter, uint16_t flags, uint32_t fflags, intptr_t data) {
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
        for (int i = 0; i < cnt; i++) {
            std::function<void(struct kevent&)> handler = handlers[id{events[i].ident, events[i].filter}];
            handler(events[i]);
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

