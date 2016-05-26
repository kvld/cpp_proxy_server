//
//  events_queue.hpp
//  proxy
//
//  Created by Vladislav Kiryukhin on 18.02.16.
//
//

#ifndef events_queue_hpp
#define events_queue_hpp

#include <stdio.h>
#include <sys/event.h>
#include <cstdlib>
#include <functional>
#include <map>
#include "file_descriptor.hpp"

class events_queue {
    
public:
    events_queue(events_queue const& rhs) = delete;
    events_queue& operator=(events_queue const& rhs) = delete;
    
    events_queue();
    ~events_queue();
    
    void add_event(const struct kevent& ev);
    void add_event(uintptr_t ident, int16_t filter, uint16_t flags, uint32_t fflags, intptr_t data, void* udata);
    void add_event(std::function<void(struct kevent&)> handler, uintptr_t ident, int16_t filter, uint16_t flags, uint32_t fflags, intptr_t data);
    
    void delete_event(uintptr_t ident, int16_t filter);
    
    int event_occurred();
    void execute_events();

private:
    file_descriptor kq;
    
    class id {
    public:
        id() : ident(0), filter(0) { }
        id(uintptr_t _ident, int16_t _filter) : ident(_ident), filter(_filter) { }
        
        friend bool operator==(id const& fr, id const& sc) {
            return std::pair<uintptr_t, int16_t>{fr.ident, fr.filter} == std::pair<uintptr_t, int16_t>{sc.ident, sc.filter};
        }
        friend bool operator<(events_queue::id const& fr, events_queue::id const& sc) {
            return std::pair<uintptr_t, int16_t>{fr.ident, fr.filter} < std::pair<uintptr_t, int16_t>{sc.ident, sc.filter};
        }
    private:
        uintptr_t ident;
        int16_t filter;
    };
    
    std::map<id, std::function<void(struct kevent&)> > handlers;
    static const size_t EVENTS_LIST_SIZE = 256;
    struct kevent events[EVENTS_LIST_SIZE];
};

#endif /* events_queue_hpp */
