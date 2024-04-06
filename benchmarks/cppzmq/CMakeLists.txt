cmake_minimum_required(VERSION 3.10)

project(ZmqServer)

set (CMAKE_CXX_STANDARD 11)

include_directories(
  .
)

find_package(cppzmq REQUIRED)

set(
  SERVER_SOURCES
  server.cpp
)

add_executable(server ${SERVER_SOURCES})
target_link_libraries(server libzmq)

set(
  CLIENT_SOURCES
  client.cpp
)

add_executable(client ${CLIENT_SOURCES})
target_link_libraries(client libzmq)