cmake_minimum_required(VERSION 3.16)
set(CMAKE_C_COMPILER "gcc")
set(CMAKE_CXX_COMPILER "g++")
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")

project(aloha)

set(SOURCE_FILES src/main.cc)

add_executable(aloha ${SOURCE_FILES})
