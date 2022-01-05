# CMAKE Support for out of tree clap plugin extensions to Juce 6
#
# To use these in your juce6 cmake project
# 1. In your CMAKE build chain, define CLAP_ROOT as the directory containing
#    both 'clap' and 'clap-helpers' from github.com/free-audio. These can be
#    submodules or out of tree
# 2. Include this cmake file in your build path
# 3. Create your juce plugin as normal with formats VST3 etc...
# 4. After that, add the following lines (or similar) to your cmake
#    clap_juce_extensions_plugin(TARGET my-target
#          CLAP_ID "com.my-cool-plugs.my-target")
# 5. Reload your CMAKe file and my-target_CLAP will be a buildable target

project(clap-juce-extensions VERSION 0.1 LANGUAGES C CXX)

set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 14)

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
  # Any Clang or any GCC
  add_compile_options(
          -Wno-multichar
          # Targetting Windows with GCC/Clang is experimental
          $<$<NOT:$<BOOL:${WIN32}>>:-Werror>

          # PE/COFF doesn't support visibility
          $<$<NOT:$<BOOL:${WIN32}>>:-fvisibility=hidden>
          # Inlines visibility is only relevant with C++
          $<$<AND:$<NOT:$<BOOL:${WIN32}>>,$<COMPILE_LANGUAGE:CXX>>:-fvisibility-inlines-hidden>
  )
endif()



if(NOT CLAP_ROOT)
  message(FATAL_ERROR "Please specify CLAP_ROOT, a directory containing `clap` and `clap-helpers`")
endif()

add_subdirectory(${CLAP_ROOT}/clap clapjuceext_clap)
add_subdirectory(${CLAP_ROOT}/clap-helpers clapjuceext_claphelpers)

add_library(clap_juce_extensions STATIC src/extensions/clap-juce-extensions.cpp)
target_include_directories(clap_juce_extensions PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_definitions(clap_juce_extensions PUBLIC
        HAS_CLAP_JUCE_EXTENSIONS=1)
target_link_libraries(clap_juce_extensions PUBLIC clap-core)

add_library(clap_juce_sources INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/src/wrapper/clap-juce-wrapper.cpp)
if (APPLE)
  target_sources(clap_juce_sources INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/src/wrapper/clap-juce-mac.mm)
endif()
target_include_directories(clap_juce_sources INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

function(clap_juce_extensions_plugin)
  set(oneValueArgs TARGET)
  set(multiValueArgs CLAP_ID CLAP_MANUAL_URL CLAP_SUPPORT_URL CLAP_FEATURES)

  cmake_parse_arguments(CJA "" "${oneValueArgs}"
          "${multiValueArgs}" ${ARGN} )
  set(target ${CJA_TARGET})
  set(claptarget ${target}_CLAP)

  message(STATUS "Creating CLAP ${claptarget} from ${target}")

  if ("${CJA_CLAP_ID}" STREQUAL "")
    message(FATAL_ERROR "You must specify CLAP_ID to add a clap" )
  endif()

  if ("${CJA_CLAP_FEATURES}" STREQUAL "")
    # message(WARNING "You did not specify features; setting to 'instrument'" )
    set(CJA_CLAP_FEATURES "instrument")
  endif()

  set(CJA_CLAP_FEATURES "instrument")

  set_target_properties(${claptarget} PROPERTIES
          ARCHIVE_OUTPUT_DIRECTORY "$<GENEX_EVAL:$<TARGET_PROPERTY:${target},ARCHIVE_OUTPUT_DIRECTORY>>/CLAP"
          LIBRARY_OUTPUT_DIRECTORY "$<GENEX_EVAL:$<TARGET_PROPERTY:${target},LIBRARY_OUTPUT_DIRECTORY>>/CLAP"
          RUNTIME_OUTPUT_DIRECTORY "$<GENEX_EVAL:$<TARGET_PROPERTY:${target},RUNTIME_OUTPUT_DIRECTORY>>/CLAP")

  get_target_property(products_folder ${claptarget} LIBRARY_OUTPUT_DIRECTORY)
  set(product_name $<TARGET_PROPERTY:${target},JUCE_PRODUCT_NAME>)

  if (${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
    set_target_properties(${claptarget} PROPERTIES
            BUNDLE True
            BUNDLE_EXTENSION clap
            PREFIX ""
            OUTPUT_NAME "${product_name}"
            MACOSX_BUNDLE TRUE
            )
  else()
    set_target_properties(${claptarget} PROPERTIES
            PREFIX ""
            SUFFIX ".clap"
            OUTPUT_NAME "${product_name}"
            )
  endif()

  target_include_directories(${claptarget} PRIVATE
          $<TARGET_PROPERTY:${target},INCLUDE_DIRECTORIES>)

  target_compile_definitions(${claptarget} PRIVATE
          CLAP_ID="${CJA_CLAP_ID}"
          CLAP_FEATURES="${CJA_CLAP_FEATURES}"
          CLAP_MANUAL_URL="${CJA_CLAP_MANUAL_URL}"
          CLAP_SUPPORT_URL="${CJA_CLAP_SUPPORT_URL}")

  target_link_libraries(${target} PUBLIC clap_juce_extensions)

  target_link_libraries(${claptarget} PUBLIC clap-core clap-helpers clap_juce_sources ${target})

  get_target_property(docopy "${target}" JUCE_COPY_PLUGIN_AFTER_BUILD)

  if(docopy)
    message(STATUS "Copy After Build" )
    if (${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
      add_custom_command(TARGET ${claptarget} POST_BUILD
              COMMAND ${CMAKE_COMMAND} -E echo "Installing ${products_folder}/${product_name}.clap to ~/Library/Audio/Plug-Ins/CLAP/"
              COMMAND ${CMAKE_COMMAND} -E make_directory "~/Library/Audio/Plug-Ins/CLAP"
              COMMAND ${CMAKE_COMMAND} -E copy_directory "${products_folder}/${product_name}.clap" "~/Library/Audio/Plug-Ins/CLAP/${product_name}.clap"
              )
    endif()
    if (${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
      add_custom_command(TARGET ${claptarget} POST_BUILD
              COMMAND ${CMAKE_COMMAND} -E echo "Installing ${products_folder}/${product_name}.clap"
              COMMAND ${CMAKE_COMMAND} -E make_directory "~/.clap"
              COMMAND ${CMAKE_COMMAND} -E copy "${products_folder}/${product_name}.clap" "~/.clap/"
              )
    endif()
  endif()
endfunction()
