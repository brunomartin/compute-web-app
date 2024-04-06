cmake_minimum_required(VERSION 3.10)

project(PocoZmq2)

set (CMAKE_CXX_STANDARD 11)

find_package(cppzmq REQUIRED)
find_package(Poco REQUIRED Foundation)

add_subdirectory(client)
add_subdirectory(server)
