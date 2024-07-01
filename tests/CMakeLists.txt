include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG release-1.12.1
)
FetchContent_MakeAvailable(googletest)

include_directories(${CMAKE_SOURCE_DIR}/src)

file(GLOB_RECURSE TEST_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cc"
)

add_executable(run_aloha_tests ${TEST_SOURCES})

target_link_libraries(run_aloha_tests
    PRIVATE
        gtest
        gtest_main
        aloha_core
)

include(GoogleTest)
gtest_discover_tests(run_aloha_tests)

target_compile_options(run_aloha_tests PRIVATE -Wall -Wextra -Wpedantic)
