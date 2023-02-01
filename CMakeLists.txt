cmake_minimum_required(VERSION 3.25)
project(fileFixer)

set(CMAKE_CXX_STANDARD 23)

include_directories(include/path)
add_executable(fileFixer src/main.cpp src/worker.cpp include/worker.h src/file.cpp include/file.h include/ring/ring.h)
target_link_libraries(fileFixer fmt)