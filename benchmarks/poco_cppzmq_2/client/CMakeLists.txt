cmake_minimum_required(VERSION 3.10)

project(PocoZmq2Client)

set (CMAKE_CXX_STANDARD 11)

include_directories(
  .
  ../process
)

set(
  CLIENT_SOURCES
  client.cpp

  ../process/process.h
  ../process/process.cpp
)

add_executable(client ${CLIENT_SOURCES})
target_link_libraries(client libzmq Poco::Foundation)

add_custom_command(
  TARGET client POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy
    ${CMAKE_CURRENT_SOURCE_DIR}/client.py $<TARGET_FILE_DIR:client>/
  COMMAND ${CMAKE_COMMAND} -E copy
  ${CMAKE_CURRENT_SOURCE_DIR}/requirements.txt $<TARGET_FILE_DIR:client>/
)