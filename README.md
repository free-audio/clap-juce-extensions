# Experimental JUCE6/CMake Clap Plugin Support

This is a set of code which, combined with a JUCE6/CMake project, allows you to build a CLAP plugin. It is experimental
and missing many features currently, but it does allow us to build a few synths and have them work, include Surge.

By far the best solution for CLAP in JUCE would be full native support by the JUCE team. Until such a time as that
happens, this code may help you if you have a JUCE plugin and want to generate a CLAP.

This version is based off of CLAP 0.25 and generates plugins which work in BWS 4.3beta1.

## Requirements and Issues

Requirements:

* Your project must use Juce6 or Juce7 and CMake

Issues:

* We support plugin creation only, not hosting.
* We aim to have currently incomplete features set up as github issues before 1 Feb 2022
* Many bus and parameter features are unsupported still
* Synths using older deprecated JUCE APIs may not work

## The "Forkless" approach and using this software

There's a couple of ways we could have gone adding experimental JUCE support. The way the LV2 extensions to juce work
requires a forked JUCE which places LV2 support fully inside the JUCE ecosystem at the cost of maintaining a fork (and
not allowing folks with their own forks to easily use LV2). We instead chose an 'out-of-juce' approach which has the
following pros and cons

Pros:

* You can use any JUCE 6 or 7 / CMake method you want and don't need to use our branch
* We don't have to update our fork to pull latest Juce features; you don't have to use our fork and choices to build
  your plugin.

Cons:

* The CMake API is not consistent. Rather than add "CLAP" as a plugin type, you need a couple of extra lines of CMake to
  activate your CLAP
* In C++, the `wrapperType` API doesn't support CLAP. All CLAP plugins will define a `wrapperType` of
  `JuceWrapperType_Undefined`. We do provide a workaround for this below.

# Creating a CLAP in your JUCE 6 or 7 CMake project

We assume you already have a JUCE6/7 plugin which is generated with a CMake file, and you can run and build a VST3, AU,
or
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

3. Create your juce plugin as normal with formats VST3 etc...
4. After your `juce_plugin` code, add the following lines (or similar)
   to your cmake (a list of pre-defined CLAP
   features can be found [here](https://github.com/free-audio/clap/blob/main/include/clap/plugin.h#L27)):

```cmake
    clap_juce_extensions_plugin(TARGET my-target
        CLAP_ID "com.my-cool-plugs.my-target"
        CLAP_FEATURES instrument "virtual analog" gritty basses leads pads)
```

5. Reload your CMake file and my-target_CLAP will be a buildable target

## The Extensions API

There are a set of things which JUCE doesn't support which CLAP does. Rather than not support them in our
plugin, we've decided to create an extensions API. These are a set of classes which your AudioProcessor can
implement and, if it does, then the clap juce wrapper will call the associated functions.

The extension are in "include/clap-juce-extensions.h" and are documented there, but currently have
three classes

- `clap_juce_extensions::clap_properties`
    - if you subclass this your AudioProcessor will have a collection of members which give you extra clap info
    - Most usefully, you get an 'is_clap' member which is false if not a clap and true if it is, which works around
      the fact that our 'forkless' approach doesn't let us add a wrapperType to the juce API
- `clap_juce_extensions::clap_extensions`
    - these are a set of advnaced extensions which let you optionally interact more directly with the clap API
      and are mostly useful for advanced features like non-destructive modulation and note expression support
- `clap_juce_extensions::clap_param_extensions`
    - If your AudioProcessorParameter subclass implements this API, you can share extended clap information on
      a parameter by parameter basis

As an example, here's how to use `clap_properties` to work around `wrapperType` being `Undefined` in the forkless
CLAP approach

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