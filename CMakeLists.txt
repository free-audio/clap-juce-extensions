project(clap-juce-extensions VERSION 0.99 LANGUAGES C CXX)

set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 17)

if(NOT CLAP_ROOT)
  message(FATAL_ERROR "Please specify CLAP_ROOT, a directory containing `clap` and `clap-helpers`")
endif()

add_subdirectory(${CLAP_ROOT}/clap clapjuceext_clap)
add_subdirectory(${CLAP_ROOT}/clap-helpers clapjuceext_claphelpers)

add_library(clap_juce_sources INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/src/clap_juce_wrapper.cpp)
target_include_directories(clap_juce_sources INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

function(clap_juce_extensions_plugin target)
  message(STATUS "Adding CLAP plugin extension to ${target}")

  set(claptarget ${target}_CLAP)
  get_property(SRC TARGET clap_juce_sources PROPERTY SOURCES)
  add_library(${claptarget} MODULE ${SRC})

  set_target_properties(${claptarget} PROPERTIES
          ARCHIVE_OUTPUT_DIRECTORY "$<GENEX_EVAL:$<TARGET_PROPERTY:${target},ARCHIVE_OUTPUT_DIRECTORY>>/Clap"
          LIBRARY_OUTPUT_DIRECTORY "$<GENEX_EVAL:$<TARGET_PROPERTY:${target},LIBRARY_OUTPUT_DIRECTORY>>/Clap"
          RUNTIME_OUTPUT_DIRECTORY "$<GENEX_EVAL:$<TARGET_PROPERTY:${target},RUNTIME_OUTPUT_DIRECTORY>>/Clap")

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

  target_link_libraries(${claptarget} PUBLIC clap-core clap-helpers clap_juce_sources ${target})

  get_target_property(docopy "${target}" JUCE_COPY_PLUGIN_AFTER_BUILD)

  if(docopy)
    message(STATUS "Copy After Build" )
    if (${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
      add_custom_command(TARGET ${claptarget} POST_BUILD
              COMMAND ${CMAKE_COMMAND} -E echo "Installing ${products_folder}/${product_name}.clap"
              COMMAND ${CMAKE_COMMAND} -E make_directory "~/Library/Audio/Plug-Ins/Clap"
              COMMAND ${CMAKE_COMMAND} -E copy_directory "${products_folder}/${product_name}.clap" "~/Library/Audio/Plug-Ins/Clap/"
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
