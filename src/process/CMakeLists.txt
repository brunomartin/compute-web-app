cmake_minimum_required(VERSION 3.10)

project(CwaProcess)

set (CMAKE_CXX_STANDARD 11)

find_package(Poco REQUIRED Foundation)

file(
  GLOB SOURCE
  *.h
  *.cpp
)

add_library(CwaProcess ${SOURCE})
target_link_libraries(CwaProcess PUBLIC CwaData CwaLog PRIVATE Poco::Foundation)
target_include_directories(CwaProcess
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..
)

add_subdirectory(test)
add_subdirectory(benchmark)
