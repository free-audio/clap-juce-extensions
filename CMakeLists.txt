# CMAKE Support for out of tree clap plugin extensions to Juce 6
#
# To use these in your juce6 cmake project
# 1. Include this cmake file in your build path
# 2. Create your juce plugin as normal with formats VST3 etc...
# 3. After that, add the following lines (or similar) to your cmake
#    clap_juce_extensions_plugin(TARGET my-target
#          CLAP_ID "com.my-cool-plugs.my-target")
# 4. Reload your CMAKe file and my-target_CLAP will be a buildable target

cmake_minimum_required (VERSION 3.15 FATAL_ERROR)

project(clap-juce-extensions VERSION 0.1 LANGUAGES C CXX)

if (CMAKE_CXX_STANDARD)
  set(CLAP_CXX_STANDARD ${CMAKE_CXX_STANDARD})
else()
  set(CLAP_CXX_STANDARD 14)
endif()
if (${CLAP_CXX_STANDARD} LESS 14)
  set(CLAP_CXX_STANDARD 14)
endif()

message( STATUS "Building CLAP with CLAP_CXX_STANDARD=${CLAP_CXX_STANDARD}")
set(CMAKE_CXX_EXTENSIONS OFF)

add_subdirectory(clap-libs/clap clapjuceext_clap EXCLUDE_FROM_ALL)
add_subdirectory(clap-libs/clap-helpers clapjuceext_claphelpers EXCLUDE_FROM_ALL)

add_library(clap_juce_extensions STATIC src/extensions/clap-juce-extensions.cpp)
set_property(TARGET clap_juce_extensions PROPERTY CXX_STANDARD ${CLAP_CXX_STANDARD})
target_include_directories(clap_juce_extensions PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_definitions(clap_juce_extensions PUBLIC
        HAS_CLAP_JUCE_EXTENSIONS=1)
target_link_libraries(clap_juce_extensions PUBLIC clap-core clap-helpers)

set_target_properties(clap_juce_extensions PROPERTIES
    POSITION_INDEPENDENT_CODE TRUE
    VISIBILITY_INLINES_HIDDEN TRUE
    C_VISBILITY_PRESET hidden
    CXX_VISIBILITY_PRESET hidden
)

add_library(clap_juce_sources INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/src/wrapper/clap-juce-wrapper.cpp)
set_property(TARGET clap_juce_sources PROPERTY CXX_STANDARD ${CLAP_CXX_STANDARD})
if (APPLE)
  target_sources(clap_juce_sources INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/src/wrapper/clap-juce-mac.mm)
endif()
target_include_directories(clap_juce_sources INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

include(cmake/ClapTargetHelpers.cmake)
