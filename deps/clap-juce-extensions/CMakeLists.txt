# CMAKE Support for out of tree clap plugin extensions to Juce 6
#
# To use these in your juce6 cmake project
# 1. Include this cmake file in your build path
# 2. Create your juce plugin as normal with formats VST3 etc...
# 3. After that, add the following lines (or similar) to your cmake
#    clap_juce_extensions_plugin(TARGET my-target
#          CLAP_ID "com.my-cool-plugs.my-target")
# 4. Reload your CMAKe file and my-target_CLAP will be a buildable target

cmake_minimum_required (VERSION 3.15 FATAL_ERROR)

project(clap-juce-extensions VERSION 0.1 LANGUAGES C CXX)

set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 14)


add_subdirectory(clap-libs/clap clapjuceext_clap EXCLUDE_FROM_ALL)
add_subdirectory(clap-libs/clap-helpers clapjuceext_claphelpers EXCLUDE_FROM_ALL)

add_library(clap_juce_extensions STATIC src/extensions/clap-juce-extensions.cpp)
set_property(TARGET clap_juce_extensions PROPERTY CXX_STANDARD 14)
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
set_property(TARGET clap_juce_sources PROPERTY CXX_STANDARD 14)
if (APPLE)
  target_sources(clap_juce_sources INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/src/wrapper/clap-juce-mac.mm)
endif()
target_include_directories(clap_juce_sources INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

function(clap_juce_extensions_plugin)
  set(oneValueArgs TARGET)
  set(multiValueArgs CLAP_ID CLAP_FEATURES CLAP_MANUAL_URL CLAP_SUPPORT_URL)

  cmake_parse_arguments(CJA "" "${oneValueArgs}"
          "${multiValueArgs}" ${ARGN} )
  set(target ${CJA_TARGET})
  set(claptarget ${target}_CLAP)

  message(STATUS "Creating CLAP ${claptarget} from ${target}")

  if ("${CJA_CLAP_ID}" STREQUAL "")
    message(FATAL_ERROR "You must specify CLAP_ID to add a clap" )
  endif()

  if ("${CJA_CLAP_FEATURES}" STREQUAL "")
    message(WARNING "No CLAP_FEATURES were specified! Using \"instrument\" by default.")
    set(CJA_CLAP_FEATURES instrument)
  endif()

  # we need the list of features as comma separated quoted strings
  foreach(feature IN LISTS CJA_CLAP_FEATURES)
    list (APPEND CJA_CLAP_FEATURES_PARSED "\"${feature}\"")
  endforeach()
  list (JOIN CJA_CLAP_FEATURES_PARSED ", " CJA_CLAP_FEATURES_PARSED)

  get_property(SRC TARGET clap_juce_sources PROPERTY SOURCES)
  add_library(${claptarget} MODULE ${SRC})

  set_target_properties(${claptarget} PROPERTIES
          CXX_STANDARD 14
          ARCHIVE_OUTPUT_DIRECTORY "$<GENEX_EVAL:$<TARGET_PROPERTY:${target},ARCHIVE_OUTPUT_DIRECTORY>>/CLAP"
          LIBRARY_OUTPUT_DIRECTORY "$<GENEX_EVAL:$<TARGET_PROPERTY:${target},LIBRARY_OUTPUT_DIRECTORY>>/CLAP"
          RUNTIME_OUTPUT_DIRECTORY "$<GENEX_EVAL:$<TARGET_PROPERTY:${target},RUNTIME_OUTPUT_DIRECTORY>>/CLAP")

  get_target_property(products_folder ${claptarget} LIBRARY_OUTPUT_DIRECTORY)
  set(product_name $<TARGET_PROPERTY:${target},JUCE_PRODUCT_NAME>)

  if (${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
    set_target_properties(${claptarget} PROPERTIES
            BUNDLE True
            BUNDLE_EXTENSION clap
            PREFIX ""
            OUTPUT_NAME "${product_name}"
            MACOSX_BUNDLE TRUE
            )
  else()
    set_target_properties(${claptarget} PROPERTIES
            PREFIX ""
            SUFFIX ".clap"
            OUTPUT_NAME "${product_name}"
            )
  endif()

  target_include_directories(${claptarget} PRIVATE
          $<TARGET_PROPERTY:${target},INCLUDE_DIRECTORIES>)

  target_compile_definitions(${claptarget} PRIVATE
          CLAP_ID="${CJA_CLAP_ID}"
          CLAP_FEATURES=${CJA_CLAP_FEATURES_PARSED}
          CLAP_MANUAL_URL="${CJA_CLAP_MANUAL_URL}"
          CLAP_SUPPORT_URL="${CJA_CLAP_SUPPORT_URL}")

  target_link_libraries(${target} PUBLIC clap_juce_extensions)

  target_link_libraries(${claptarget} PUBLIC clap-core clap-helpers clap_juce_sources ${target})
  set_property(TARGET ${claptarget} PROPERTY C_VISIBILITY_PRESET hidden)
  set_property(TARGET ${claptarget} PROPERTY VISIBILITY_INLINES_HIDDEN ON)

  set_target_properties(${claptarget} PROPERTIES
      POSITION_INDEPENDENT_CODE TRUE
      VISIBILITY_INLINES_HIDDEN TRUE
      C_VISBILITY_PRESET hidden
      CXX_VISIBILITY_PRESET hidden
  )

  get_target_property(docopy "${target}" JUCE_COPY_PLUGIN_AFTER_BUILD)

  if(docopy)
    message(STATUS "Copy After Build" )
    if (${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
      add_custom_command(TARGET ${claptarget} POST_BUILD
              COMMAND ${CMAKE_COMMAND} -E echo "Installing ${products_folder}/${product_name}.clap to ~/Library/Audio/Plug-Ins/CLAP/"
              COMMAND ${CMAKE_COMMAND} -E make_directory "~/Library/Audio/Plug-Ins/CLAP"
              COMMAND ${CMAKE_COMMAND} -E copy_directory "${products_folder}/${product_name}.clap" "~/Library/Audio/Plug-Ins/CLAP/${product_name}.clap"
              )
    endif()
    if (${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
      add_custom_command(TARGET ${claptarget} POST_BUILD
              COMMAND ${CMAKE_COMMAND} -E echo "Installing ${products_folder}/${product_name}.clap"
              COMMAND ${CMAKE_COMMAND} -E make_directory "~/.clap"
              COMMAND ${CMAKE_COMMAND} -E copy "${products_folder}/${product_name}.clap" "~/.clap/"
              )
    endif()
  endif()
endfunction()
