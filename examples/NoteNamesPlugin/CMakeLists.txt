if(CLAP_WRAP_PROJUCER_PLUGIN)
    return()
endif()

juce_add_plugin(NoteNamesPlugin
    COMPANY_NAME "${COMPANY_NAME}"
    PLUGIN_MANUFACTURER_CODE "${COMPANY_CODE}"
    PLUGIN_CODE Gplg
    FORMATS ${JUCE_FORMATS}
    PRODUCT_NAME "NoteNamesPlugin"
    IS_SYNTH TRUE
)

clap_juce_extensions_plugin(
    TARGET NoteNamesPlugin
    CLAP_ID "org.free-audio.NoteNamesPlugin"
    CLAP_FEATURES audio-effect utility
)

target_sources(NoteNamesPlugin PRIVATE
    NoteNamesPlugin.cpp
)

target_compile_definitions(NoteNamesPlugin PUBLIC
    JUCE_DISPLAY_SPLASH_SCREEN=1
    JUCE_REPORT_APP_USAGE=0
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_JACK=1
    JUCE_ALSA=1
    JUCE_MODAL_LOOPS_PERMITTED=1 # required for Linux FileChooser with JUCE 6.0.7
    JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(NoteNamesPlugin
    PRIVATE
        juce::juce_audio_utils
        juce::juce_audio_plugin_client
        clap_juce_extensions
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)
