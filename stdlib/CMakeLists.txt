set(ALOHA_RUNTIME_SOURCES
    runtime/arena.c
    runtime/io.c
    runtime/string.c
    runtime/vector.c
)

add_library(aloha_stdlib STATIC ${ALOHA_RUNTIME_SOURCES})

set_target_properties(aloha_stdlib PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)

target_include_directories(aloha_stdlib PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/runtime
)
