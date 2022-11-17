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

    # We have to ignore some warnings on some platforms and with some versions of JUCE.
    # Let's remove these when we're no longer supporting some of the older version of JUCE.
    if(APPLE AND CLAP_JUCE_VERSION STREQUAL "6.0.7")
        add_compile_options(-Wno-deprecated-declarations)
        add_compile_options(-Wno-unused-but-set-variable)
    endif()
    if(APPLE AND CLAP_JUCE_VERSION STREQUAL "6.1.6")
        add_compile_options(-Wno-deprecated-copy-with-dtor)
        add_compile_options(-Wno-deprecated-copy)
    endif()
endif()

add_subdirectory(GainPlugin)
