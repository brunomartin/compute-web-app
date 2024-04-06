cmake_minimum_required(VERSION 3.10)

project(PocoZmq2Server)

include_directories(
  .
  ../process
)

set(
  SERVER_SOURCES
  server.cpp

  ../process/process.h
  ../process/process.cpp
)

add_executable(server ${SERVER_SOURCES})
target_link_libraries(server libzmq Poco::Foundation)

add_dependencies(server client)

add_custom_command(
  TARGET server POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy
    $<TARGET_FILE:client> $<TARGET_FILE_DIR:server>/
  COMMAND ${CMAKE_COMMAND} -E copy
    ${CMAKE_CURRENT_SOURCE_DIR}/run_benchmark.py
    $<TARGET_FILE_DIR:server>/
  COMMAND ${CMAKE_COMMAND} -E copy
    ${CMAKE_CURRENT_SOURCE_DIR}/../client/client.py $<TARGET_FILE_DIR:server>/
  COMMAND ${CMAKE_COMMAND} -E copy
    ${CMAKE_CURRENT_SOURCE_DIR}/../client/requirements.txt $<TARGET_FILE_DIR:server>/
)
