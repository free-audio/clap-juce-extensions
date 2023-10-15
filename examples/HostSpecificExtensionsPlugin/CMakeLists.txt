if(CLAP_WRAP_PROJUCER_PLUGIN)
    return()
endif()

if(NOT DEFINED REAPER_SDK_PATH)
    message(STATUS "REAPER SDK path not supplied, skipping configuration for host-specific extensions plugin.")
    return()
endif()

if (NOT EXISTS "${REAPER_SDK_PATH}/sdk/reaper_plugin.h")
    message(WARNING "REAPER SDK: reaper_plugin.h not found! (Looking at: ${REAPER_SDK_PATH}/sdk/reaper_plugin.h)")
endif()

message(STATUS "Configuring host-specific extensions plugin with REAPER SDK: ${REAPER_SDK_PATH}")

juce_add_plugin(HostSpecificExtensionsPlugin
    COMPANY_NAME "${COMPANY_NAME}"
    PLUGIN_MANUFACTURER_CODE "${COMPANY_CODE}"
    PLUGIN_CODE Hsep
    FORMATS ${JUCE_FORMATS}
    PRODUCT_NAME "Host-Specific Extensions Tester"
)

clap_juce_extensions_plugin(
    TARGET HostSpecificExtensionsPlugin
    CLAP_ID "org.free-audio.HostSpecificExtensionsPlugin"
    CLAP_FEATURES audio-effect utility
    CLAP_PROCESS_EVENTS_RESOLUTION_SAMPLES 64
)

target_sources(HostSpecificExtensionsPlugin PRIVATE
    HostSpecificExtensionsPlugin.cpp
    PluginEditor.cpp
)

target_compile_definitions(HostSpecificExtensionsPlugin PUBLIC
    JUCE_DISPLAY_SPLASH_SCREEN=1
    JUCE_REPORT_APP_USAGE=0
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_JACK=1
    JUCE_ALSA=1
    JUCE_MODAL_LOOPS_PERMITTED=1 # required for Linux FileChooser with JUCE 6.0.7
    JUCE_VST3_CAN_REPLACE_VST2=0
)

target_include_directories(HostSpecificExtensionsPlugin
    PRIVATE
        "${REAPER_SDK_PATH}/sdk"
)

target_link_libraries(HostSpecificExtensionsPlugin
    PRIVATE
        juce::juce_audio_utils
        juce::juce_audio_plugin_client
        clap_juce_extensions
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)
