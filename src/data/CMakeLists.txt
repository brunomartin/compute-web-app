cmake_minimum_required(VERSION 3.10)

project(CwaData)

set (CMAKE_CXX_STANDARD 11)

add_library(asio INTERFACE)
target_include_directories(asio INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/asio/asio/include)
target_link_libraries(asio INTERFACE pthread)

file(
  GLOB SOURCE
  *.h
  *.cpp
)

add_library(CwaData ${SOURCE})
target_link_libraries(CwaData PUBLIC CwaLog json PRIVATE asio)
target_include_directories(CwaData
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..
)

add_subdirectory(test)
