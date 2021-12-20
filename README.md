# JUCE6/CMake Clap Support

This is a set of code which, combined with a JUCE6/CMake project, allows you to build a (buggy, feature incomplete, work
in progress) clap.

To use these in your juce6 cmake project:

1. In your CMAKE build chain, define CLAP_ROOT as the directory containing both 'clap' and 'clap-helpers' from
   github.com/free-audio. These can be submodules or out of tree
2. Include the cmake file from this project in your main CMakeLists.txt with
   `add_subdirectory(path-to-clap-juce-extensions EXCLUDE_FROM_ALL)` or some such
3. Create your juce plugin as normal with formats VST3 etc...
4. After that, add the following lines (or similar) to your cmake

```
    clap_juce_extensions_plugin(TARGET my-target
          CLAP_ID "com.my-cool-plugs.my-target")
```

5. Reload your CMake file and my-target_CLAP will be a buildable target

This code is code I moved from my juce fork so we could run against unforked or differently forked juces. That original
fork worked mac/win/lin but right nwo this different strategy is tested mac only. Give me until christmas :)