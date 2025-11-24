cmake_minimum_required(VERSION 3.16)

set(CMAKE_C_COMPILER "clang")
set(CMAKE_CXX_COMPILER "clang++")

project(aloha LANGUAGES C CXX)
if (NOT DEFINED CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 23)
endif()
set(CMAKE_CXX_EXTENSIONS OFF)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

# Required packages
find_package(LLVM REQUIRED CONFIG)
find_package(ZLIB REQUIRED)
find_package(LibEdit REQUIRED)
find_package(Terminfo REQUIRED)
find_package(zstd REQUIRED)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

include_directories(SYSTEM ${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

include_directories(${CMAKE_SOURCE_DIR}/src)

# Source files
file(GLOB_RECURSE SRC_FILES src/*.cc)
# exclude main.cc from the list of source files
list(FILTER SRC_FILES EXCLUDE REGEX ".*main.cc$")

# Standard library files
file(GLOB_RECURSE STDLIB_FILES stdlib/*.c stdlib/*.cc)

add_library(aloha_core STATIC ${SRC_FILES})
add_library(aloha_stdlib STATIC ${STDLIB_FILES})

# Map LLVM components to library names
llvm_map_components_to_libnames(LLVM_COMPONENT_LIBS
    core
    orcjit
    native
    support
    irreader
)

target_link_libraries(aloha_core
    ${LLVM_COMPONENT_LIBS}
    ZLIB::ZLIB
    LibEdit::LibEdit
)

add_executable(aloha src/main.cc)

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    foreach (target aloha aloha_core aloha_stdlib)
        target_compile_options(${target} PRIVATE
            -Wall
            -Wextra
            -Werror
            -Wshadow
            -Wconversion
            -Wpedantic

            #dont warn about unused variables in tests
            -Wno-unused-variable
            -Wno-unused-function
            -Wno-unused-parameter
        )
    endforeach()
endif()

target_link_libraries(aloha aloha_core aloha_stdlib)

# Optimization and Debugging Flags
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -g3 -ggdb")

# Installation setup
install(TARGETS aloha
    DESTINATION bin
)

# # For Testing
# enable_testing()
# add_subdirectory(tests)

# debug target
add_custom_target(debug
  COMMAND gdb ${CMAKE_BINARY_DIR}/aloha
  DEPENDS aloha
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)
