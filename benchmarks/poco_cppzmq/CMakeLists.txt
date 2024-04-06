cmake_minimum_required(VERSION 3.10)

project(PocoZmqServer)

set (CMAKE_CXX_STANDARD 11)

include_directories(
  .
)

find_package(cppzmq REQUIRED)
find_package(Poco REQUIRED Foundation)

set(
  SERVER_SOURCES
  server.cpp

  process.h
  process.cpp
)

add_executable(server ${SERVER_SOURCES})
target_link_libraries(server libzmq Poco::Foundation)

set(
  CLIENT_SOURCES
  client.cpp

  process.h
  process.cpp
)

add_executable(client ${CLIENT_SOURCES})
target_link_libraries(client libzmq Poco::Foundation)
add_dependencies(server client)