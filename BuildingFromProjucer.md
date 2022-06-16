# Building From Projucer

While `clap-juce-extensions` is designed to be used with a plugin project
that uses JUCE + CMake, it is also possible to use this wrapper to build
a CLAP plugin from a JUCE project using the Projucer, although this requires
a few extra steps.

1. Build your projucer-based plugin.
2. Create `CMakeLists.txt` file in the same directory as your `.jucer` file. Here's an example CMakeLists.txt:

```cmake
cmake_minimum_required(VERSION 3.15)
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.12" CACHE STRING "Minimum OS X deployment target")
project(MyPlugin VERSION 1.0.0)

set(PATH_TO_JUCE path/to/JUCE)
set(PATH_TO_CLAP_EXTENSIONS path/to/clap-juce-extensions)

# define the exporter types used in your Projucer configuration
if(APPLE)
    set(JUCER_GENERATOR "Xcode")
elseif(WIN32)
    set(JUCER_GENERATOR "VisualStudio2019")
else() # Linux
    set(JUCER_GENERATOR "LinuxMakefile")
endif()

include(${PATH_TO_CLAP_EXTENSIONS}/cmake/JucerClap.cmake)
create_jucer_clap_target(
    TARGET MyPlugin # "Binary Name" in the Projucer
    PLUGIN_NAME "My Plugin"
    MANUFACTURER_NAME "My Company"
    MANUFACTURER_CODE Manu
    PLUGIN_CODE Plg1
    VERSION_STRING "1.0.0"
    CLAP_ID "org.mycompany.myplugin"
    CLAP_FEATURES instrument synthesizer
    CLAP_MANUAL_URL "https://www.mycompany.com"
    CLAP_SUPPORT_URL "https://www.mycompany.com"
    EDITOR_NEEDS_KEYBOARD_FOCUS FALSE
)
```

3. Build the CLAP plugin using CMake. This step can be done manually,
   as part of an automated build script, or potentially even as a
   post-build step triggered from the Projucer:
```bash
cmake -Bbuild-clap -G<generator> -DCMAKE_BUILD_TYPE=<Debug|Release>
cmake --build build-clap --config <Debug|Release>
```
The resulting builds will be located in `build-clap/MyPlugin_artefacts`.
