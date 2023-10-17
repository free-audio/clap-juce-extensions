set(COMPANY_NAME "free-audio")
set(COMPANY_CODE "FrAu")
set(JUCE_FORMATS AU VST3 Standalone)

option(CLAP_WRAP_PROJUCER_PLUGIN "Wrap a CLAP plugin from a Projucer build" OFF)

option(CLAP_EXAMPLES_TREAT_WARNINGS_AS_ERRORS "Treat warnings as errors for the example plugin builds" OFF)
if(CLAP_EXAMPLES_TREAT_WARNINGS_AS_ERRORS)
    if(NOT APPLE AND NOT (CLAP_JUCE_VERSION STREQUAL "6.0.7" OR CLAP_JUCE_VERSION STREQUAL "6.1.6"))
        message(STATUS "Building CLAP example plugins with \"-Werror\"")
        add_compile_options(
            $<$<CXX_COMPILER_ID:MSVC>:/WX>
            $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Werror>
        )
    endif()
endif()

add_subdirectory(GainPlugin)
add_subdirectory(NoteNamesPlugin)
add_subdirectory(HostSpecificExtensionsPlugin)
