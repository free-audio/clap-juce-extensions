# modified version of clap_juce_extensions_plugin
# for use with Projucer projects
function(clap_juce_extensions_plugin_jucer)
    set(oneValueArgs TARGET TARGET_PATH PLUGIN_NAME)
    set(multiValueArgs CLAP_ID CLAP_FEATURES CLAP_MANUAL_URL CLAP_SUPPORT_URL)

    cmake_parse_arguments(CJA "" "${oneValueArgs}"
            "${multiValueArgs}" ${ARGN} )
    set(target ${CJA_TARGET})
    set(claptarget ${target}_CLAP)

    message(STATUS "Creating CLAP ${claptarget} from ${target}")

    if ("${CJA_PLUGIN_NAME}" STREQUAL "")
        message(STATUS "Explicit plugin name not set! Using ${target}")
        set(CJA_CLAP_FEATURES "${target}")
    endif()

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

    set(base_folder "${CMAKE_CURRENT_BINARY_DIR}/${target}_artefacts")
    set(products_folder "${base_folder}/$<CONFIG>")

    set_target_properties(${claptarget} PROPERTIES
            CXX_STANDARD 14
            ARCHIVE_OUTPUT_DIRECTORY "${products_folder}"
            LIBRARY_OUTPUT_DIRECTORY "${products_folder}"
            RUNTIME_OUTPUT_DIRECTORY "${products_folder}")

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

    # The real wrapper includes the plugin libs include directories
    # here... we'll have to just do that manually later on.

    target_compile_definitions(${claptarget} PRIVATE
            CLAP_ID="${CJA_CLAP_ID}"
            CLAP_FEATURES=${CJA_CLAP_FEATURES_PARSED}
            CLAP_MANUAL_URL="${CJA_CLAP_MANUAL_URL}"
            CLAP_SUPPORT_URL="${CJA_CLAP_SUPPORT_URL}")

    # Since we're working with a pre-compiled plugin lib we can't build
    # the plugin with the extensions... however, we still need to compile
    # the extensions, since we'll get a linker error otherwise.
    target_link_libraries(${claptarget} PUBLIC clap_juce_extensions)

    target_link_libraries(${claptarget} PUBLIC clap-core clap-helpers clap_juce_sources ${CJA_TARGET_PATH})
    set_property(TARGET ${claptarget} PROPERTY C_VISIBILITY_PRESET hidden)
    set_property(TARGET ${claptarget} PROPERTY VISIBILITY_INLINES_HIDDEN ON)

    set_target_properties(${claptarget} PROPERTIES
            POSITION_INDEPENDENT_CODE TRUE
            VISIBILITY_INLINES_HIDDEN TRUE
            C_VISBILITY_PRESET hidden
            CXX_VISIBILITY_PRESET hidden
            )

    # The real wrapper does an optional copy step here... let's skip that for now.
endfunction()

# use this function to create a CLAP from a jucer project
function(create_jucer_clap_target)
    set(oneValueArgs TARGET PLUGIN_NAME MANUFACTURER_NAME MANUFACTURER_URL VERSION_STRING MANUFACTURER_CODE PLUGIN_CODE EDITOR_NEEDS_KEYBOARD_FOCUS)
    set(multiValueArgs CLAP_ID CLAP_FEATURES CLAP_MANUAL_URL CLAP_SUPPORT_URL)

    cmake_parse_arguments(CJA "" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if ("${CMAKE_BUILD_TYPE}" STREQUAL "")
        message(WARNING "CMAKE_BUILD_TYPE not set... using Release by default")
        set(CMAKE_BUILD_TYPE "Release")
    endif()

    if("${JUCER_GENERATOR}" STREQUAL "VisualStudio2019")
        find_library(PLUGIN_LIBRARY_PATH ${CJA_TARGET} "Builds/VisualStudio2019/x64/${CMAKE_BUILD_TYPE}/Shared Code")
    elseif("${JUCER_GENERATOR}" STREQUAL "VisualStudio2017")
        find_library(PLUGIN_LIBRARY_PATH ${CJA_TARGET} "Builds/VisualStudio2017/x64/${CMAKE_BUILD_TYPE}/Shared Code")
    elseif("${JUCER_GENERATOR}" STREQUAL "VisualStudio2015")
        find_library(PLUGIN_LIBRARY_PATH ${CJA_TARGET} "Builds/VisualStudio2015/x64/${CMAKE_BUILD_TYPE}/Shared Code")
    elseif("${JUCER_GENERATOR}" STREQUAL "Xcode")
        find_library(PLUGIN_LIBRARY_PATH ${CJA_TARGET} "Builds/MacOSX/build/${CMAKE_BUILD_TYPE}")
    elseif("${JUCER_GENERATOR}" STREQUAL "LinuxMakefile")
        # for some reason Projucer makes a lib called "PluginName.a", but find_library needs "libPluginName.a"
        set(LINUX_LIB_PATH "${CMAKE_CURRENT_SOURCE_DIR}/Builds/LinuxMakefile/build")
        configure_file("${LINUX_LIB_PATH}/${CJA_TARGET}.a" "${LINUX_LIB_PATH}/lib${CJA_TARGET}.a" COPYONLY)
        find_library(PLUGIN_LIBRARY_PATH ${CJA_TARGET} "${LINUX_LIB_PATH}")
    elseif("${JUCER_GENERATOR}" STREQUAL "")
        message(FATAL_ERROR "JUCER_GENERATOR variable must be set!")
    else()
        message(FATAL_ERROR "Unknown Generator!")
    endif()

    message(STATUS "Plugin SharedCode library path: ${PLUGIN_LIBRARY_PATH}")

    add_subdirectory(${PATH_TO_JUCE} clap_juce_juce)
    add_subdirectory(${PATH_TO_CLAP_EXTENSIONS} clap_juce_clapext EXCLUDE_FROM_ALL)

    clap_juce_extensions_plugin_jucer(
        TARGET ${CJA_TARGET}
        TARGET_PATH ${PLUGIN_LIBRARY_PATH}
        PLUGIN_NAME "${CJA_PLUGIN_NAME}"
        CLAP_ID "${CJA_CLAP_ID}"
        CLAP_FEATURES "${CJA_CLAP_FEATURES}"
    )

    set(clap_target "${CJA_TARGET}_CLAP")
    target_include_directories(${clap_target}
        PUBLIC
            ${PATH_TO_JUCE}/modules
            JuceLibraryCode
            Source
    )

    target_compile_definitions(${clap_target}
        PRIVATE
            JUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1
            JucePlugin_Name="${CJA_PLUGIN_NAME}"
            JucePlugin_Manufacturer="${CJA_MANUFACTURER_NAME}"
            JucePlugin_ManufacturerWebsite="${CJA_MANUFACTURER_URL}"
            JucePlugin_VersionString="${CJA_VERSION_STRING}"
            JucePlugin_Desc=""
    )

    if(APPLE)
        if("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
            target_compile_definitions(${clap_target} PRIVATE DEBUG=1)
        else()
            target_compile_definitions(${clap_target} PRIVATE NDEBUG=1)
        endif()

        if("${CJA_CLAP_FEATURES}" MATCHES "^instrument.*")
            message(STATUS "Detected plugin category: instrument")
            target_compile_definitions(${clap_target} PRIVATE JucePlugin_IsSynth=1)
            target_compile_definitions(${clap_target} PRIVATE JucePlugin_ProducesMidiOutput=0)
            target_compile_definitions(${clap_target} PRIVATE JucePlugin_WantsMidiInput=1)
        elseif("${CJA_CLAP_FEATURES}" MATCHES "^audio-effect.*")
            message(STATUS "Detected plugin category: audio-effect")
            target_compile_definitions(${clap_target} PRIVATE JucePlugin_IsSynth=0)
            target_compile_definitions(${clap_target} PRIVATE JucePlugin_ProducesMidiOutput=0)
            target_compile_definitions(${clap_target} PRIVATE JucePlugin_WantsMidiInput=0)
        elseif("${CJA_CLAP_FEATURES}" MATCHES "^note-effect.*")
            message(STATUS "Detected plugin category: note-effect")
            target_compile_definitions(${clap_target} PRIVATE JucePlugin_IsSynth=0)
            target_compile_definitions(${clap_target} PRIVATE JucePlugin_ProducesMidiOutput=1)
            target_compile_definitions(${clap_target} PRIVATE JucePlugin_WantsMidiInput=1)
        else()
            message(FATAL_ERROR "No plugin category detected!")
        endif()
        
        if("${CJA_MANUFACTURER_CODE}" STREQUAL "")
            message(WARNING "Manufacturer code not set! Using \"Manu\"")
            set(CJA_MANUFACTURER_CODE "Manu")
        endif()
        target_compile_definitions(${clap_target} PRIVATE JucePlugin_ManufacturerCode=${CJA_MANUFACTURER_CODE})

        if("${CJA_PLUGIN_CODE}" STREQUAL "")
            message(WARNING "Plugin code not set! Using \"Xyz5\"")
            set(CJA_PLUGIN_CODE "Xyz5")
        endif()
        target_compile_definitions(${clap_target} PRIVATE JucePlugin_PluginCode=${CJA_PLUGIN_CODE})

        if(${CJA_EDITOR_NEEDS_KEYBOARD_FOCUS})
            target_compile_definitions(${clap_target} PRIVATE JucePlugin_EditorRequiresKeyboardFocus=1)
        else()
            target_compile_definitions(${clap_target} PRIVATE JucePlugin_EditorRequiresKeyboardFocus=0)
        endif()

        _juce_link_frameworks("${clap_target}" PRIVATE AppKit Cocoa WebKit OpenGL CoreAudioKit CoreAudio CoreMidi CoreVideo CoreImage Quartz Accelerate AudioToolbox IOKit)
    endif()
endfunction()
