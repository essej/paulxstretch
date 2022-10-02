if(CLAP_WRAP_PROJUCER_PLUGIN)
    set(PATH_TO_JUCE "${JUCE_SOURCE_DIR}")
    set(PATH_TO_CLAP_EXTENSIONS ${CMAKE_CURRENT_SOURCE_DIR}/../..)

    if(APPLE)
        set(JUCER_GENERATOR "Xcode")
    elseif(WIN32)
        set(JUCER_GENERATOR "VisualStudio2019")
    else() # Linux
        set(JUCER_GENERATOR "LinuxMakefile")
    endif()

    include(${PATH_TO_CLAP_EXTENSIONS}/cmake/JucerClap.cmake)
    create_jucer_clap_target(
        TARGET "GainPlugin"
        PLUGIN_NAME "GainPlugin"
        BINARY_NAME "MyGreatGainPlugin"
        PLUGIN_CODE "Gplg"
        MANUFACTURER_NAME "${COMPANY_NAME}"
        MANUFACTURER_CODE "${COMPANY_CODE}"
        VERSION_STRING "${CMAKE_PROJECT_VERSION}"
        CLAP_ID "org.free-audio.GainPlugin"
        CLAP_FEATURES audio-effect utility
        CLAP_PROCESS_EVENTS_RESOLUTION_SAMPLES 64
    )

    return()
endif()

juce_add_plugin(GainPlugin
    COMPANY_NAME "${COMPANY_NAME}"
    PLUGIN_MANUFACTURER_CODE "${COMPANY_CODE}"
    PLUGIN_CODE Gplg
    FORMATS ${JUCE_FORMATS}
    PRODUCT_NAME "GainPlugin"
)

clap_juce_extensions_plugin(
    TARGET GainPlugin
    CLAP_ID "org.free-audio.GainPlugin"
    CLAP_FEATURES audio-effect utility
    CLAP_PROCESS_EVENTS_RESOLUTION_SAMPLES 64
)

target_sources(GainPlugin PRIVATE
    GainPlugin.cpp
    PluginEditor.cpp
)

target_compile_definitions(GainPlugin PUBLIC
    JUCE_DISPLAY_SPLASH_SCREEN=1
    JUCE_REPORT_APP_USAGE=0
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_JACK=1
    JUCE_ALSA=1
    JUCE_MODAL_LOOPS_PERMITTED=1 # required for Linux FileChooser with JUCE 6.0.7
    JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(GainPlugin
    PRIVATE
        juce::juce_audio_utils
        juce::juce_audio_plugin_client
        juce::juce_dsp
        clap_juce_extensions
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)
