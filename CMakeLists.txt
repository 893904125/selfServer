cmake_minimum_required(VERSION 3.28)
project(selfServer)

set(CMAKE_CXX_STANDARD 20)

add_executable(selfServer code/main.cpp
        code/log/log.cpp
        code/log/log.h
        code/buffer/buffer.cpp
        code/buffer/buffer.h
        code/log/BlockDeque.h)
