cmake_minimum_required(VERSION 3.16)
set(CMAKE_C_COMPILER "clang")
set(CMAKE_CXX_COMPILER "clang++")
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")

find_package(LLVM REQUIRED CONFIG)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

project(aloha)

set(SOURCE_FILES src/main.cc src/token.cc src/lexer.cc
                 src/ast.cc src/parser.cc src/semantic_analyzer.cc
                 src/symbolTable.cc src/codegen.cc src/objgen.cc src/stdlib.cc
               )
llvm_map_components_to_libnames(LLVM_LIBS
    core
    orcjit
    native
    support
    irreader
)
add_executable(aloha ${SOURCE_FILES})
target_link_libraries(aloha ${LLVM_LIBS})
