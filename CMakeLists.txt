cmake_minimum_required(VERSION 3.25)
project(fileFixer)

set(CMAKE_CXX_STANDARD 23)

include_directories(
        include/path
        extern/liburing/src/include
        )

add_executable(fileFixer
        src/main.cpp
        include/worker.h
        include/file.h
        include/ring.h
        )
target_link_libraries(fileFixer fmt)