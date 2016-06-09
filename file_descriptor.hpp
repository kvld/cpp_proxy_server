//
//  file_descriptor.hpp
//  proxy
//
//  Created by Vladislav Kiryukhin on 15.02.16.
//
//

#ifndef file_descriptor_hpp
#define file_descriptor_hpp

#include <stdio.h>

class file_descriptor {
    
public:
    file_descriptor();
    
    file_descriptor& operator=(const file_descriptor& rhs) = delete;
    file_descriptor(const file_descriptor&) = delete;
    
    file_descriptor(int fd);
    file_descriptor(file_descriptor&& rhs);
    file_descriptor& operator=(file_descriptor&& rhs);
    
    ~file_descriptor();
    
    void set_fd(int fd);
    int get_fd();
    
    void make_nonblocking();
    
protected:
    int fd;
    
};

#endif /* file_descriptor_hpp */
