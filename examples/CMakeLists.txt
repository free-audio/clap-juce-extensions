set(COMPANY_NAME "free-audio")
set(COMPANY_CODE "FrAu")
set(JUCE_FORMATS AU VST3 Standalone)

option(CLAP_WRAP_PROJUCER_PLUGIN "Wrap a CLAP plugin from a Projucer build" OFF)

add_subdirectory(GainPlugin)
