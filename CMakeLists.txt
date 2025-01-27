# To get started on a new plugin, copy this entire folder (containing this file and C++ sources) to
# a convenient location, and then start making modifications.

# The first line of any CMake project should be a call to `cmake_minimum_required`, which checks
# that the installed CMake will be able to understand the following CMakeLists, and ensures that
# CMake's behaviour is compatible with the named version. This is a standard CMake command, so more
# information can be found in the CMake docs.

cmake_minimum_required(VERSION 3.15)


if (WIN32)
    #set (CMAKE_GENERATOR_TOOLSET ClangCL)
    #static linking in Windows
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

# If we are compiling for Mac OS we want to target OS versions down to 10.9
option(UniversalBinary "Build universal binary for mac" ON)

if (APPLE)
    set (CMAKE_OSX_DEPLOYMENT_TARGET "10.11" CACHE INTERNAL "")
    if (UniversalBinary)
        set(CMAKE_OSX_ARCHITECTURES "x86_64;arm64" CACHE INTERNAL "")
        #set(CMAKE_OSX_ARCHITECTURES arm64 x86_64)        
    endif()
endif()







# The top-level CMakeLists.txt file for a project must contain a literal, direct call to the
# `project()` command. `project()` sets up some helpful variables that describe source/binary
# directories, and the current project version. This is a standard CMake command.

project(PaulXStretch VERSION 1.6.0)

set(BUILDVERSION 112)


# If you've installed JUCE somehow (via a package manager, or directly using the CMake install
# target), you'll need to tell this project that it depends on the installed copy of JUCE. If you've
# included JUCE directly in your source tree (perhaps as a submodule), you'll need to tell CMake to
# include that subdirectory as part of the build.

# find_package(JUCE CONFIG REQUIRED)        # If you've installed JUCE to your system
# or


# By default we don't want Xcode schemes to be made for modules, etc
set(CMAKE_XCODE_GENERATE_SCHEME OFF)

# No ZERO_CHECK target (it helps bust cache for cmake)
set(CMAKE_SUPPRESS_REGENERATION true) 

# prevent install all
#set(CMAKE_SKIP_INSTALL_ALL_DEPENDENCY true)

# Adds all the module sources so they appear correctly in the IDE
# Must be set before JUCE is added as a sub-dir (or any targets are made)
# https://github.com/juce-framework/JUCE/commit/6b1b4cf7f6b1008db44411f2c8887d71a3348889
set_property(GLOBAL PROPERTY USE_FOLDERS YES)

# This is a failed attempt to bury ALL_BUILD in Targets/
# This should be called before any target is made
# Bug in Xcode? https://gitlab.kitware.com/cmake/cmake/-/issues/21383
set_property(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER "Targets")

# Create a /Modules directory in the IDE with the JUCE Module code
option(JUCE_ENABLE_MODULE_SOURCE_GROUPS "Show all module sources in IDE projects" ON)


# include JUCE

add_subdirectory(deps/juce EXCLUDE_FROM_ALL )


# Extend JUCE with CLAP support from https://github.com/free-audio/clap-juce-extensions
add_subdirectory(deps/clap-juce-extensions EXCLUDE_FROM_ALL)



set (FormatsToBuild VST3 CLAP Standalone)

# On Mac, an AU version will be built too
if (APPLE)
    list (APPEND FormatsToBuild AU)
endif()

# If you are building a VST2 or AAX plugin, CMake needs to be told where to find these SDKs on your
# system. This setup should be done before calling `juce_add_plugin`.

#juce_set_vst2_sdk_path("../VST2_SDK")
#juce_set_aax_sdk_path("../AAX_SDK_2p3p2")


if (AAX_SDK_PATH)
    juce_set_aax_sdk_path (${AAX_SDK_PATH})
    
    if (APPLE OR (NOT ("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "Win32")))
      list (APPEND AaxFormatsToBuild AAX)
    endif()
endif()

#if (VST2_SDK_PATH)
#    juce_set_vst2_sdk_path (${VST2_SDK_PATH})
#    list (APPEND FormatsToBuild VST)
#endif()


set (MacPList "<plist version=\"1.0\">
<dict>
<key>CFBundleVersion</key>
<string>${BUILDVERSION}</string>
<key>CFBundleURLTypes</key>
<array>
     <dict>
         <key>CFBundleURLName</key>
         <string>com.sonosaurus.paulxstretch</string>
         <key>CFBundleURLSchemes</key>
         <array>
              <string>paulxstretch</string>
         </array>
      </dict>
   </array>
</dict>
</plist>")


# `juce_add_plugin` adds a static library target with the name passed as the first argument
# (AudioPluginExample here). This target is a normal CMake target, but has a lot of extra properties set
# up by default. As well as this shared code static library, this function adds targets for each of
# the formats specified by the FORMATS arguments. This function accepts many optional arguments.
# Check the readme at `docs/CMake API.md` in the JUCE repo for the full list.

function(sono_add_custom_plugin_target target_name product_name formats is_instrument no_fftw plugincode)

   if (is_instrument)
        set (vst3cats Instrument Network)
        set (vst2cat "kPlugCategSynth")
    else()
        set (vst3cats Fx Network)        
        set (vst2cat "kPlugCategEffect")
   endif()

    juce_add_plugin("${target_name}"
        IS_SYNTH "${is_instrument}"
        NEEDS_MIDI_INPUT TRUE
        NEEDS_MIDI_OUTPUT TRUE
        EDITOR_WANTS_KEYBOARD_FOCUS TRUE
        COMPANY_NAME "Sonosaurus"
        BUNDLE_ID "com.sonosaurus.paulxstretch"
        MICROPHONE_PERMISSION_ENABLED TRUE
        
        ICON_BIG "images/paulxstretch_icon_1024_rounded.png"
        ICON_SMALL "images/paulxstretch_icon_256_rounded.png"
        NEEDS_WEB_BROWSER FALSE
        VST2_CATEGORY "${vst2cat}"
        VST3_CATEGORIES "${vst3cats}"
        AAX_CATEGORY "AAX_EPlugInCategory_Effect"

        
        # mac settings
        HARDENED_RUNTIME_ENABLED TRUE
        HARDENED_RUNTIME_OPTIONS "com.apple.security.device.audio-input"
        PLIST_TO_MERGE "${MacPList}"
        AU_MAIN_TYPE "kAudioUnitType_MusicEffect"
        
        COPY_PLUGIN_AFTER_BUILD FALSE   # Set to TRUE to auto-install
        
        # other settings...
        PLUGIN_MANUFACTURER_CODE Sono
        PLUGIN_CODE ${plugincode}
        FORMATS ${formats}
        DESCRIPTION "PaulXStretch - Extreme Timestretching"
        PRODUCT_NAME "${product_name}")


    if ("CLAP" IN_LIST formats)
        # Configure the CLAP plugin
        clap_juce_extensions_plugin(TARGET "${target_name}"
                CLAP_ID "com.sonosaurus.paulstretch.clap"
                CLAP_FEATURES "audio-effect")
    endif()

    juce_generate_juce_header("${target_name}")


    set (HEADER_INCLUDES
     #   Source/PS_Source
     #   Source/WDL
    )

    set (LIB_PATHS  "")

    #set (PLAT_COMPILE_DEFS
    #    $<$<CONFIG:Debug>:LOGLEVEL=2>
    #    USE_CODEC_OPUS=1
    #    AOO_TIMEFILTER_CHECK=0
    #    AOO_STATIC)

    set(PlatSourceFiles
        Source/CrossPlatformUtils.h
      )


    set (FFTW_DEPLIBS  "")


    # platform specific stuff
    if (APPLE)
        list (APPEND HEADER_INCLUDES deps/mac/include)
        list (APPEND LIB_PATHS ${CMAKE_CURRENT_SOURCE_DIR}/deps/mac/lib)
        list (APPEND PlatSourceFiles  Source/CrossPlatformUtilsMac.mm)

        if (no_fftw)
            # use vDSP
            list (APPEND PLAT_COMPILE_DEFS
                    PS_USE_VDSP_FFT=1
            )
        else()
            list (APPEND FFTW_DEPLIBS fftw3f)
        endif()
    elseif (WIN32)
        list (APPEND HEADER_INCLUDES deps/windows ../asiosdk/common)
        list (APPEND PlatSourceFiles  Source/CrossPlatformUtilsWindows.cpp)

	    message (STATUS "Win generator platform is: ${CMAKE_VS_PLATFORM_NAME}" )
        if ("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "Win32")
            list (APPEND LIB_PATHS
                $<$<CONFIG:Debug>:${CMAKE_CURRENT_SOURCE_DIR}/deps/windows/Debug32>
                $<$<CONFIG:Release>:${CMAKE_CURRENT_SOURCE_DIR}/deps/windows/Release32>
            )
        else()
            list (APPEND LIB_PATHS
                $<$<CONFIG:Debug>:${CMAKE_CURRENT_SOURCE_DIR}/deps/windows/Debug>
                $<$<CONFIG:Release>:${CMAKE_CURRENT_SOURCE_DIR}/deps/windows/Release>
            )
        endif()

        list (APPEND PLAT_COMPILE_DEFS
            JUCE_MODAL_LOOPS_PERMITTED
            _USE_MATH_DEFINES
            WINVER=0x0601
            _WIN32_WINNT=0x0601
	    NOMINMAX)

        if (no_fftw)
            # use pffft
            list (APPEND PLAT_COMPILE_DEFS
                    PS_USE_PFFFT=1
            )
        else()
            list (APPEND FFTW_DEPLIBS
                fftw3f
                mingwex
                gcc
                mingw32
                msvcrt
            )
        endif()

    else()
        # Linux
        list (APPEND PlatSourceFiles  Source/CrossPlatformUtilsLinux.cpp)
	    list ( APPEND PLAT_COMPILE_DEFS
	        JUCE_USE_MP3AUDIOFORMAT=1 )

        if (no_fftw)
            # use pffft
            list (APPEND PLAT_COMPILE_DEFS
                    PS_USE_PFFFT=1
            )
        else()
            list (APPEND FFTW_DEPLIBS fftw3f)
        endif()
    endif()



    set(SourceFiles
        ${PlatSourceFiles}
        Source/CustomLookAndFeel.cpp
        Source/CustomLookAndFeel.h
        Source/CustomStandaloneFilterApp.cpp
        Source/CustomStandaloneFilterWindow.h
        Source/GenericItemChooser.cpp
        Source/GenericItemChooser.h
        Source/SonoChoiceButton.cpp
        Source/SonoChoiceButton.h
        Source/SonoTextButton.cpp
        Source/SonoTextButton.h
        Source/PluginEditor.cpp
        Source/PluginEditor.h
        Source/PluginProcessor.cpp
        Source/PluginProcessor.h
        Source/RenderSettingsComponent.cpp
        Source/RenderSettingsComponent.h
        Source/OptionsView.cpp
        Source/OptionsView.h
        Source/envelope_component.cpp
        Source/envelope_component.h
        Source/jcdp_envelope.h
        Source/ps3_BufferingAudioSource.cpp
        Source/ps3_BufferingAudioSource.h
        #Source/PS_Source/fftw3.h
        Source/PS_Source/PaulStretchControl.cpp
        Source/PS_Source/Stretch.h
        Source/PS_Source/version.h
        Source/PS_Source/Player.cpp
        Source/PS_Source/BinauralBeats.h
        Source/PS_Source/StretchSource.h
        Source/PS_Source/ProcessedStretch.cpp
        Source/PS_Source/Input
        Source/PS_Source/Input/AInputS.h
        Source/PS_Source/Input/InputS.h
        Source/PS_Source/BinauralBeats.cpp
        Source/PS_Source/ProcessedStretch.h
        Source/PS_Source/StretchSource.cpp
        Source/PS_Source/Player.h
        Source/PS_Source/globals.h
        Source/PS_Source/Stretch.cpp
        Source/PS_Source/FreeEdit.h
        Source/PS_Source/FreeEdit.cpp
        Source/PS_Source/PaulStretchControl.h
        Source/PS_Source/Input/AInputS.h
        Source/PS_Source/Input/InputS.h
        Source/WDL/resample.h
        Source/WDL/resample.cpp
        Source/WDL/wdltypes.h
        Source/WDL/denormal.h
        Source/WDL/heapbuf.h
        Source/pffft/pffft.h
        Source/pffft/pffft.c
            )

    target_sources("${target_name}" PRIVATE 
           ${SourceFiles} 
       )

    # No, we don't want our source buried in extra nested folders
    set_target_properties("${target_name}" PROPERTIES FOLDER "")

    # The source tree should uhhh, still look like the source tree, yo
    source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/Source PREFIX "" FILES ${SourceFiles})

    # Move the INTERFACE auto-created JUCE library stuff into its own folder
    source_group("JUCE Library Code" REGULAR_EXPRESSION "juce_")


    #target_include_directories("${target_name}"
    #        INTERFACE
    #            $<TARGET_PROPERTY:"${target_name}",INCLUDE_DIRECTORIES>)

    

    target_include_directories("${target_name}"
        PUBLIC
        ${HEADER_INCLUDES}
    )


    # Require at least C++17 to build `my_target`
    target_compile_features("${target_name}" PRIVATE cxx_std_17)
        

    # This cleans up the folder organization, especially on Xcode.
    # It tucks the Plugin varieties into a "Targets" folder and generate an Xcode Scheme manually
    # Xcode scheme generation is turned off globally to limit noise from other targets
    # The non-hacky way of doing this is via the global PREDEFINED_TARGETS_FOLDER propety
    # However that doesn't seem to be working in Xcode
    # Not all plugin types (au, vst) available on each build type (win, macos, linux)
    foreach(target ${formats} "All")
        if(TARGET ${target_name}_${target})
            set_target_properties(${target_name}_${target} PROPERTIES
                # Tuck the actual plugin targets into a folder where they won't bother us
                FOLDER "Targets"
            
                # MacOS only: Sets the default executable that Xcode will open on build
                # For this exact path to to work, manually build the AudioPluginHost.xcodeproj in the JUCE subdir 
                # XCODE_SCHEME_EXECUTABLE "${CMAKE_CURRENT_SOURCE_DIR}/deps/juce/extras/AudioPluginHost/Builds/MacOSX/build/Debug/AudioPluginHost.app"
            
                # Let us build the target in Xcode 
                XCODE_GENERATE_SCHEME ON)
        endif()
    endforeach()
    
    
    target_compile_definitions("${target_name}"
        PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0
        JUCE_USE_CUSTOM_PLUGIN_STANDALONE_APP=1
        JUCE_LOAD_CURL_SYMBOLS_LAZILY=1
        JUCE_USE_ANDROID_OBOE=1
        JUCE_USE_OBOE_STABILIZED_CALLBACK=1
        FF_AUDIO_ALLOW_ALLOCATIONS_IN_MEASURE_BLOCK=0
        PAULXSTRETCH_BUILD_VERSION="${VERSION}"
        ${PLAT_COMPILE_DEFS} )

if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
    target_compile_definitions("${target_name}"
            PUBLIC
            JUCE_ASIO=1
            JUCE_WASAPI=1
            JUCE_DIRECTSOUND=0
            JUCE_USE_WINDOWS_MEDIA_FORMAT=1
    )
elseif (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_compile_definitions("${target_name}"
            PUBLIC
            JUCE_JACK=1
            JUCE_ALSA=1
    )
endif()

    juce_add_binary_data("${target_name}_PSData" SOURCES
        images/freeze.svg
        images/loop_icon.svg
        images/passthru.svg
        images/passthru_enabled.svg
        images/pause_icon.svg
        images/play_icon.svg
        images/power.svg
        images/power_sel.svg
        images/record_input.svg
        images/record_input_active.svg
        images/record_output.svg
        images/record_output_active.svg
        images/skipback_icon.svg
    )

    set_target_properties(${target_name}_PSData PROPERTIES FOLDER "Targets")



    if (UNIX AND NOT APPLE)
       target_compile_options("${target_name}_PSData"
         PRIVATE
         -fPIC
       )

       find_library(FFTWF_LIB fftw3f)
       if (NOT FFTWF_LIB)
	       message(FATAL_ERROR "fftw3f library not found, please install libfftw3f develop package")
       endif()

       if (JUCE_LINUX_TARGET_ARCHITECTURE MATCHES "arm" )
          message(STATUS "ARM platform, adding -march=native")
          target_compile_options(${target_name} PUBLIC
           -march=native
          )
       endif()
     
       if (TARGET ${target_name}_Standalone)
           # make linux executable all lower case
           string(TOLOWER ${target_name} tmptargname)

           set_target_properties("${target_name}_Standalone"
	     PROPERTIES
	       OUTPUT_NAME ${tmptargname}
           )
       endif()

   endif()


    target_link_directories("${target_name}" INTERFACE
        ${LIB_PATHS}
    )

    target_link_libraries("${target_name}" 
        PRIVATE
            juce::juce_audio_utils
            juce::juce_dsp
            juce::juce_audio_plugin_client
            juce::juce_cryptography
            
            ${target_name}_PSData

            ${FFTW_DEPLIBS}

        PUBLIC
            juce::juce_recommended_config_flags
            juce::juce_recommended_lto_flags
        #   juce::juce_recommended_warning_flags
        )
            
endfunction()

# most of the targets
sono_add_custom_plugin_target(PaulXStretch PaulXStretch "${FormatsToBuild}" FALSE FALSE "Pxst")

if (AaxFormatsToBuild)
  # AAX builds without fftw
  sono_add_custom_plugin_target(PaulXStretchAAX PaulXStretch "${AaxFormatsToBuild}" FALSE TRUE "Pxst")
endif()

# Mobile targets
#sono_add_custom_plugin_target(PaulXStretch "AUv3 Standalone" FALSE "NBus")

# add VSTi target
# sono_add_custom_plugin_target(PaulXStretchInst "PaulXStretchInstrument" "VST3" TRUE  "IBus")

