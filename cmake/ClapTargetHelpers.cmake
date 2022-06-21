function(clap_juce_extensions_plugin_internal)
    set(oneValueArgs TARGET TARGET_PATH PLUGIN_NAME IS_JUCER DO_COPY)
    set(multiValueArgs CLAP_ID CLAP_FEATURES CLAP_MANUAL_URL CLAP_SUPPORT_URL)
  
    cmake_parse_arguments(CJA "" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
    set(target ${CJA_TARGET})
    string(REPLACE " " "_" claptarget "${target}_CLAP")
  
    message(STATUS "Creating CLAP ${claptarget} from ${target}")
  
    if ("${CJA_CLAP_ID}" STREQUAL "")
        message(FATAL_ERROR "You must specify CLAP_ID to add a clap" )
    endif()
  
    if ("${CJA_CLAP_FEATURES}" STREQUAL "")
        message(WARNING "No CLAP_FEATURES were specified! Using \"instrument\" by default.")
        set(CJA_CLAP_FEATURES instrument)
     endif()

    # we need the list of features as comma separated quoted strings
    foreach(feature IN LISTS CJA_CLAP_FEATURES)
        list (APPEND CJA_CLAP_FEATURES_PARSED "\"${feature}\"")
    endforeach()
    list (JOIN CJA_CLAP_FEATURES_PARSED ", " CJA_CLAP_FEATURES_PARSED)

    get_property(SRC TARGET clap_juce_sources PROPERTY SOURCES)
    add_library(${claptarget} MODULE ${SRC})

    get_property(CLAP_CXX_STANDARD TARGET clap_juce_sources PROPERTY CXX_STANDARD)

    if(${CJA_IS_JUCER})
        set(base_folder "${CMAKE_CURRENT_BINARY_DIR}/${target}_artefacts")
        set(products_folder "${base_folder}/$<CONFIG>")
        set_target_properties(${claptarget} PROPERTIES
                CXX_STANDARD ${CLAP_CXX_STANDARD}
                ARCHIVE_OUTPUT_DIRECTORY "${products_folder}"
                LIBRARY_OUTPUT_DIRECTORY "${products_folder}"
                RUNTIME_OUTPUT_DIRECTORY "${products_folder}")
    else()
        set_target_properties(${claptarget} PROPERTIES
                CXX_STANDARD ${CLAP_CXX_STANDARD}
                ARCHIVE_OUTPUT_DIRECTORY "$<GENEX_EVAL:$<TARGET_PROPERTY:${target},ARCHIVE_OUTPUT_DIRECTORY>>/CLAP"
                LIBRARY_OUTPUT_DIRECTORY "$<GENEX_EVAL:$<TARGET_PROPERTY:${target},LIBRARY_OUTPUT_DIRECTORY>>/CLAP"
                RUNTIME_OUTPUT_DIRECTORY "$<GENEX_EVAL:$<TARGET_PROPERTY:${target},RUNTIME_OUTPUT_DIRECTORY>>/CLAP")
        target_include_directories(${claptarget} PRIVATE $<TARGET_PROPERTY:${target},INCLUDE_DIRECTORIES>)
    endif()

    get_target_property(products_folder ${claptarget} LIBRARY_OUTPUT_DIRECTORY)
    set(product_name "${CJA_PLUGIN_NAME}")

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

    target_compile_definitions(${claptarget} PRIVATE
            CLAP_ID="${CJA_CLAP_ID}"
            CLAP_FEATURES=${CJA_CLAP_FEATURES_PARSED}
            CLAP_MANUAL_URL="${CJA_CLAP_MANUAL_URL}"
            CLAP_SUPPORT_URL="${CJA_CLAP_SUPPORT_URL}")

    if(${CJA_IS_JUCER})
        # Since we're working with a pre-compiled plugin lib we can't build
        # the plugin with the extensions... however, we still need to compile
        # the extensions, since we'll get a linker error otherwise.
        target_link_libraries(${claptarget} PUBLIC clap_juce_extensions)
        target_link_libraries(${claptarget} PUBLIC ${CJA_TARGET_PATH})
    else()
        target_link_libraries(${target} PUBLIC clap_juce_extensions)
        target_link_libraries(${claptarget} PUBLIC ${target})
    endif()

    target_link_libraries(${claptarget} PUBLIC clap-core clap-helpers clap_juce_sources)
    set_property(TARGET ${claptarget} PROPERTY C_VISIBILITY_PRESET hidden)
    set_property(TARGET ${claptarget} PROPERTY VISIBILITY_INLINES_HIDDEN ON)

    set_target_properties(${claptarget} PROPERTIES
        POSITION_INDEPENDENT_CODE TRUE
        VISIBILITY_INLINES_HIDDEN TRUE
        C_VISBILITY_PRESET hidden
        CXX_VISIBILITY_PRESET hidden
    )

    if(${CJA_DO_COPY})
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

function(clap_juce_extensions_plugin)
    set(oneValueArgs TARGET)
    set(multiValueArgs CLAP_ID CLAP_FEATURES CLAP_MANUAL_URL CLAP_SUPPORT_URL)
    cmake_parse_arguments(CJA "" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    set(product_name $<TARGET_PROPERTY:${CJA_TARGET},JUCE_PRODUCT_NAME>)
    get_target_property(docopy "${CJA_TARGET}" JUCE_COPY_PLUGIN_AFTER_BUILD)

    clap_juce_extensions_plugin_internal(
        TARGET ${CJA_TARGET}
        PLUGIN_NAME "${product_name}"
        IS_JUCER FALSE
        DO_COPY ${docopy}
        CLAP_ID "${CJA_CLAP_ID}"
        CLAP_FEATURES "${CJA_CLAP_FEATURES}"
        CLAP_MANUAL_URL "${CJA_CLAP_MANUAL_URL}"
        CLAP_SUPPORT_URL "${CJA_CLAP_SUPPORT_URL}"
    )
endfunction()

# modified version of clap_juce_extensions_plugin
# for use with Projucer projects
function(clap_juce_extensions_plugin_jucer)
    set(oneValueArgs TARGET TARGET_PATH PLUGIN_NAME DO_COPY)
    set(multiValueArgs CLAP_ID CLAP_FEATURES CLAP_MANUAL_URL CLAP_SUPPORT_URL)
    cmake_parse_arguments(CJA "" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    clap_juce_extensions_plugin_internal(
        TARGET ${CJA_TARGET}
        TARGET_PATH "${CJA_TARGET_PATH}"
        PLUGIN_NAME "${CJA_PLUGIN_NAME}"
        IS_JUCER TRUE
        DO_COPY ${CJA_DO_COPY}
        CLAP_ID "${CJA_CLAP_ID}"
        CLAP_FEATURES "${CJA_CLAP_FEATURES}"
        CLAP_MANUAL_URL "${CJA_CLAP_MANUAL_URL}"
        CLAP_SUPPORT_URL "${CJA_CLAP_SUPPORT_URL}"
    )
endfunction()
