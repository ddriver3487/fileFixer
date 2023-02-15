cmake_minimum_required(VERSION 3.25)
project(fileFixer)

set(CMAKE_CXX_STANDARD 23)

include_directories(include)

add_executable(fileFixer
        src/main.cpp
        include/worker.h
        include/file.h
        include/ring.h
        include/iouring.h include/newRing.h)
target_link_libraries(fileFixer fmt /usr/lib/liburing.so)