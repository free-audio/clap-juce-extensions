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
    JUCE_MODAL_LOOPS_PERMITTED=0
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
