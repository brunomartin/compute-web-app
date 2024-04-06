cmake_minimum_required(VERSION 3.10)

project(CwaLog)

set (CMAKE_CXX_STANDARD 11)

file(
  GLOB SOURCE

  *.h
  *.cpp
)

add_library(CwaLog ${SOURCE})
target_include_directories(CwaLog
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
)
