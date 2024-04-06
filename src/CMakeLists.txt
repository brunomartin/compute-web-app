cmake_minimum_required(VERSION 3.10)

project(CwaServer)

set (CMAKE_CXX_STANDARD 11)

# include nlohmann json library
add_library(json INTERFACE)
target_include_directories(json INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/json/single_include)

add_subdirectory(log)
add_subdirectory(data)
add_subdirectory(process)