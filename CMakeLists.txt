cmake_minimum_required(VERSION 3.16)
set(CMAKE_C_COMPILER "clang")
set(CMAKE_CXX_COMPILER "clang++")

if (NOT DEFINED CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 23)
endif()

set(CMAKE_CXX_EXTENSIONS OFF)

project(aloha C CXX)

find_package(LLVM REQUIRED CONFIG)
find_package(ZLIB REQUIRED)
find_package(LibEdit REQUIRED)
find_package(Terminfo REQUIRED)
find_package(zstd REQUIRED)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

include_directories(SYSTEM ${LLVM_INCLUDE_DIRS})

add_definitions(${LLVM_DEFINITIONS})

set(SOURCE_FILES
    src/main.cc
    src/token.cc
    src/lexer.cc
    src/ast.cc
    src/parser.cc
    src/semantic_analyzer.cc
    src/symbolTable.cc
    src/codegen.cc
    src/objgen.cc
    src/stdlib.cc
)

llvm_map_components_to_libnames(LLVM_LIBS
    core
    orcjit
    native
    support
    irreader
)

add_executable(aloha ${SOURCE_FILES})

target_link_libraries(aloha
    ${LLVM_LIBS}
    ZLIB::ZLIB
    LibEdit::LibEdit
)

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(aloha PRIVATE
        -Wall
        -Wextra
        -Werror
        -Wshadow
        -Wconversion
        -Wpedantic
        -Wno-unused-variable
        -Wno-unused-function
        -Wno-unused-parameter
  )
endif()

set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -g")

install(TARGETS aloha
    DESTINATION bin
)
