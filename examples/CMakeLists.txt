set(COMPANY_NAME "free-audio")
set(COMPANY_CODE "FrAu")
set(JUCE_FORMATS AU VST3 Standalone)

option(CLAP_WRAP_PROJUCER_PLUGIN "Wrap a CLAP plugin from a Projucer build" OFF)

option(CLAP_EXAMPLES_TREAT_WARNINGS_AS_ERRORS "Treat warnings as errors for the example plugin builds" OFF)
if(CLAP_EXAMPLES_TREAT_WARNINGS_AS_ERRORS)
    add_compile_options(
      $<$<CXX_COMPILER_ID:MSVC>:/WX>
      $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Werror>
    )
endif()

add_subdirectory(GainPlugin)
