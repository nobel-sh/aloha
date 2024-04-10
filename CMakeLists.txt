cmake_minimum_required(VERSION 3.16)
set(CMAKE_C_COMPILER "clang")
set(CMAKE_CXX_COMPILER "clang++")
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")

project(aloha)

set(SOURCE_FILES src/main.cc src/token.cc src/lexer.cc)

add_executable(aloha ${SOURCE_FILES})
