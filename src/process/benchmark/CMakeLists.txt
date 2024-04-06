cmake_minimum_required(VERSION 3.10)

project(CwaProcessBenchmark)

find_package (Eigen3 3.3 REQUIRED NO_MODULE)

file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/input)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/output)

add_executable(CwaProcessBenchmarkWorker benchmark_worker.cpp)
target_link_libraries(CwaProcessBenchmarkWorker CwaProcess Eigen3::Eigen)

add_executable(CwaProcessBenchmarkMain benchmark_main.cpp)
target_link_libraries(CwaProcessBenchmarkMain CwaProcess)
add_dependencies(CwaProcessBenchmarkMain CwaProcessBenchmarkWorker)

configure_file(benchmark.py ${CMAKE_CURRENT_BINARY_DIR}/benchmark.py)
