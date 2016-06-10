cmake_minimum_required(VERSION 3.2)
project(proxy_server)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -pthread")

set(SOURCE_FILES
	DNS_resolver.cpp
	DNS_resolver.hpp
	client.cpp
	client.hpp
	events_queue.cpp
	events_queue.hpp
	exceptions.hpp
	file_descriptor.cpp
	file_descriptor.hpp
	http.cpp
	http.hpp
	lru_cache.hpp
	main.cpp
	proxy_server.cpp
	proxy_server.hpp
	server.cpp
	server.hpp
	socket.cpp
	socket.hpp)

add_executable(proxy_server ${SOURCE_FILES} main.cpp)
