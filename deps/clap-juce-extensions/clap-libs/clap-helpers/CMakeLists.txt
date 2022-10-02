cmake_minimum_required(VERSION 3.17)
cmake_policy(SET CMP0100 NEW)  # handle .hh files
project(CLAP_HELPERS C CXX)
enable_testing()
find_package(Catch2 QUIET)

add_library(clap-helpers INTERFACE)
set_target_properties(clap-helpers PROPERTIES
    POSITION_INDEPENDENT_CODE ON
    CXX_STANDARD 11)
target_include_directories(clap-helpers INTERFACE include)
target_link_libraries(clap-helpers INTERFACE clap-core)

# CLAP_BUILD_TESTS is inherited from clap
if (Catch2_FOUND)
    add_executable(clap-helpers-tests EXCLUDE_FROM_ALL
        tests/plugin.cc
        tests/param-queue-tests.cc
        tests/main.cc)
    set_target_properties(clap-helpers-tests PROPERTIES CXX_STANDARD 11)
    target_link_libraries(clap-helpers-tests clap-helpers clap-core Catch2::Catch2)
    target_compile_definitions(clap-helpers-tests PUBLIC -DCATCH_CONFIG_PREFIX_ALL)
    add_test(NAME test-clap-helpers COMMAND clap-helpers-tests)
    add_dependencies(clap-tests clap-helpers-tests)
endif()

install(DIRECTORY include DESTINATION ".")
