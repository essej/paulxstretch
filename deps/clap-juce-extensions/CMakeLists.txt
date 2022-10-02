# CMAKE Support for out of tree clap plugin extensions to Juce 6
#
# To use these in your juce6 cmake project
# 1. Include this cmake file in your build path
# 2. Create your juce plugin as normal with formats VST3 etc...
# 3. After that, add the following lines (or similar) to your cmake
#    clap_juce_extensions_plugin(TARGET my-target
#          CLAP_ID "com.my-cool-plugs.my-target")
# 4. Reload your CMAKe file and my-target_CLAP will be a buildable target

# The INTERFACE targets to transmit CXX_STANDARD and POSITION INDEPENDENT
# require at least CMAKE 3.21
cmake_minimum_required (VERSION 3.21 FATAL_ERROR)

get_directory_property(parent_dir PARENT_DIRECTORY)
if ("${parent_dir}" STREQUAL "")
    # OSX deployment target needs to be set before the `project()` call
    set(CMAKE_OSX_DEPLOYMENT_TARGET "10.9" CACHE STRING "Minimum OS X deployment target")
    set(is_toplevel 1)
else ()
    set(is_toplevel 0)
endif ()

project(clap-juce-extensions VERSION 0.1.0 LANGUAGES C CXX)

if (CMAKE_CXX_STANDARD)
  set(CLAP_CXX_STANDARD ${CMAKE_CXX_STANDARD})
else()
  set(CLAP_CXX_STANDARD 14)
endif()
if (${CLAP_CXX_STANDARD} LESS 14)
  set(CLAP_CXX_STANDARD 14)
endif()

message( STATUS "Building CLAP with CLAP_CXX_STANDARD=${CLAP_CXX_STANDARD}")
set(CMAKE_CXX_EXTENSIONS OFF)

option(CLAP_JUCE_EXTENSIONS_BUILD_EXAMPLES "Add targets for building and running clap-juce-extensions examples" ${is_toplevel})
if(CLAP_JUCE_EXTENSIONS_BUILD_EXAMPLES)
    # The examples need JUCE to be imported before the CLAP helper targets
    set(CLAP_JUCE_VERSION "6.1.5" CACHE STRING "Version of JUCE to use for building example plugins")
    message(STATUS "Building examples with JUCE version: ${CLAP_JUCE_VERSION}")
    include(examples/cmake/CPM.cmake)
    CPMAddPackage("gh:juce-framework/JUCE#${CLAP_JUCE_VERSION}")
endif()

add_subdirectory(clap-libs/clap clapjuceext_clap EXCLUDE_FROM_ALL)
add_subdirectory(clap-libs/clap-helpers clapjuceext_claphelpers EXCLUDE_FROM_ALL)

add_library(clap_juce_extensions STATIC src/extensions/clap-juce-extensions.cpp)
set_property(TARGET clap_juce_extensions PROPERTY CXX_STANDARD ${CLAP_CXX_STANDARD})
target_include_directories(clap_juce_extensions PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_definitions(clap_juce_extensions PUBLIC
        HAS_CLAP_JUCE_EXTENSIONS=1)
target_link_libraries(clap_juce_extensions PUBLIC clap-core clap-helpers)

set_target_properties(clap_juce_extensions PROPERTIES
    POSITION_INDEPENDENT_CODE TRUE
    VISIBILITY_INLINES_HIDDEN TRUE
    C_VISBILITY_PRESET hidden
    CXX_VISIBILITY_PRESET hidden
)

add_library(clap_juce_sources INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/src/wrapper/clap-juce-wrapper.cpp)
set_property(TARGET clap_juce_sources PROPERTY CXX_STANDARD ${CLAP_CXX_STANDARD})
set_property(TARGET clap_juce_sources PROPERTY CLAP_JUCE_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
if (APPLE)
  target_sources(clap_juce_sources INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/src/wrapper/clap-juce-mac.mm)
endif()
target_include_directories(clap_juce_sources INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

include(cmake/ClapTargetHelpers.cmake)

if(CLAP_JUCE_EXTENSIONS_BUILD_EXAMPLES)
    message(STATUS "Configuring clap-juce-extensions examples")
    add_subdirectory(examples)
endif()
