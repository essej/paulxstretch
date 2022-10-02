set(COMPANY_NAME "free-audio")
set(COMPANY_CODE "FrAu")
set(JUCE_FORMATS AU VST3 Standalone)

option(CLAP_WRAP_PROJUCER_PLUGIN "Wrap a CLAP plugin from a Projucer build" OFF)

option(CLAP_EXAMPLES_TREAT_WARNINGS_AS_ERRORS "Treat warnings as errors for the example plugin builds" OFF)
if(CLAP_EXAMPLES_TREAT_WARNINGS_AS_ERRORS)
    message(STATUS "Building CLAP example plugins with \"-Werror\"")
    add_compile_options(
        $<$<CXX_COMPILER_ID:MSVC>:/WX>
        $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Werror>
    )

    # JUCE 6.0.7 gets a deprecated declaration in its CoreAudio
    # code. We can remove this once we're no longer building JUCE
    # 6.0.7 in our CI.
    if(APPLE AND CLAP_JUCE_VERSION STREQUAL "6.0.7")
        add_compile_options(-Wno-deprecated-declarations)
    endif()
endif()

add_subdirectory(GainPlugin)
