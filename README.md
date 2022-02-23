# Experimental JUCE6/CMake Clap Plugin Support

This is a set of code which, combined with a JUCE6/CMake project, allows you to build a CLAP plugin. It is experimental
and missing many features currently, but it does allow us to build a few synths and have them work, include Surge.

By far the best solution for CLAP in JUCE would be full native support by the JUCE team. Until such a time as that
happens, this code may help you if you have a JUCE plugin and want to generate a CLAP.

## Requirements and Issues

Requirements:

* Your project must use JUCE6 and CMake

Issues:

* We support plugin creation only, not hosting.
* We aim to have currently incomplete features set up as github issues before 1 Feb 2022
* Many bus and parameter features are unsupported still
* Synths using older deprecated JUCE APIs still don't work

## The "Forkless" approach and using this software

There's a couple of ways we could have gone adding experimental JUCE support. The way the LV2 extensions to juce work
requires a forked JUCE which places LV2 support fully inside the JUCE ecosystem at the cost of maintaining a fork (and
not allowing folks with their own forks to easily use LV2). We instead chose an 'out-of-juce' approach which has the
following pros and cons

As of Jan 26, 2022, we modified the forkless approach to make the extensions a consistent self contained git repo with
the rest of clap as a sub-module. If you built with these tools before, please read the changed instructions below.

Pros:

* You can use any JUCE 6 / CMake method you want and don't need to use our branch
* We don't have to update our fork to pull latest Juce features; you don't have to use our fork and choices to build
  your plugin.

Cons:

* The CMake API is not consistent. Rather than add "CLAP" as a plugin type, you need a couple of extra lines of CMake to
  activate your CLAP
* In C++, the `wrapperType` API doesn't support CLAP. All CLAP plugins will define a `wrapperType` of
  `JuceWrapperType_Undefined`. We do provide a workaround for this below.

# Creating a CLAP in your JUCE6 CMake project

We assume you already have a JUCE6 plugin which is generated with a CMake file, and you can run and build a VST3, AU, or
so on using CMake. If you are in this state, building a Clap is a simple exercise of checking out the clap code
somewhere in your dev environment, setting a few CMake variables, and adding a couple of lines to your CMake file. The
instructions are as follows:

1. Add `https://github.com/free-audio/clap-juce-extensions.git` as a submodule of your project, or otherwise make the
   source available to your cmake (CPM, side by side check out in CI, etc...).
2. Load the `clap-juce-extension` in your CMake after you have loaded juce. For instance you could do

```cmake
add_subdirectory(libs/JUCE) # this is however you load juce
add_subdirectory(libs/clap-juce-extensions EXCLUDE_FROM_ALL) 
```

In surge we clap extensions are a sub module side by side with juce.

3. Create your juce plugin as normal with formats VST3 etc...
4. After that `juce_plugin` code, add the following lines (or similar)
   to your cmake (a list of pre-defined CLAP
   features can be found [here](https://github.com/free-audio/clap/blob/main/include/clap/plugin.h#L27)):

```cmake
    clap_juce_extensions_plugin(TARGET my-target
          CLAP_ID "com.my-cool-plugs.my-target"
          CLAP_FEATURES equalizer audio_effect)
```

5. Reload your CMake file and my-target_CLAP will be a buildable target

## The one missing API from "Forkless"

As mentioned above `wrapperType` will be set to `Undefined` using this method. There's two things you can do about this

1. Live with it. It's probably OK! But if you do need to know your wrapper type
2. Use the extension mechanism:
  - `#include "clap-juce-extensions/clap-juce-extensions.h"`
  - Make your main plugin `juce::AudioProcessor` derive from `clap_juce_extensions::clap_properties`
  - Use the `is_clap` member variable to figure out the correct wrapper type.

Here's a minimal example:
```cpp
#include <JuceHeader.h>
#include "clap-juce-extensions/clap-juce-extensions.h"

class MyCoolPlugin : public juce::AudioProcessor,
                     public clap_juce_extensions::clap_properties
{
    String getWrapperTypeString()
    {
        if (wrapperType == wrapperType_Undefined && is_clap)
            return "Clap";
    
        return juce::AudioProcessor::getWrapperTypeDescription (wrapperType);
    }
    
    ...
};
```