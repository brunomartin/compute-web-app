cmake_minimum_required(VERSION 3.10)

project(HttplibServer)

set (CMAKE_CXX_STANDARD 11)

include_directories(
  .
)

set(
  SERVER_SOURCES
  server.cpp
  httplib.h
  httplib.cc
)

add_executable(server ${SERVER_SOURCES})

set(
  CLIENT_SOURCES
  client.cpp
  httplib.h
  httplib.cc
)

add_executable(client ${CLIENT_SOURCES})