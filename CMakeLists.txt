cmake_minimum_required(VERSION 3.25)
project(fileFixer)

set(CMAKE_CXX_STANDARD 23)

include_directories(include/path)
add_executable(fileFixer src/main.cpp src/fixer.cpp include/fixer.h)
target_link_libraries(fileFixer fmt)