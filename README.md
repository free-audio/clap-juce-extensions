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
* We aim to have currently incomplete features set up as github issues before Mid-January
* Many bus and parameter features are unsupported still
* Synths using older deprecated JUCE APIs still don't work

## The "Forkless" approach and using this software

There's a couple of ways we could have gone adding experimental JUCE support. The way the LV2 extensions to juce work
requires a forked JUCE which places LV2 support fully inside the JUCE ecosystem at the cost of maintaining a fork (and
not allowing folks with their own forks to easily use LV2). We instead chose an 'out-of-juce' approach which has the
following pros and cons

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

1. In a directory of your choosing, clone https://github.com/free-audio/clap https://github.com/free-audio/clap-helpers
   and
   https://github.com/free-audio/clap-juce-extensions
3. Load the `clap-juce-extension` in your CMake after you have loaded juce. For instance you could do

```
add_subdirectory(libs/JUCE) # this is however you load juce
add_subdirectory(/usr/Me/dev/CLAP/clap-juce-extensions EXCLUDE_FROM_ALL)` 
```

In Surge we chose to make this juce extensions location a variable which triggers the build

3. Create your juce plugin as normal with formats VST3 etc... 4After that `juce_plugin` code, add the following lines (
   or similar) to your cmake

```
    clap_juce_extensions_plugin(TARGET my-target
          CLAP_ID "com.my-cool-plugs.my-target")
```

5. In your CMAKE build chain, define CLAP_ROOT as the directory containing both 'clap' and 'clap-helpers'. For instance
   if the directory you picked was `/usr/Me/dev/CLAP_STUFF` you would add to your first
   CMake `-DCLAP_ROOT=/usr/Me/dev/CLAP_STUFF`
6. Reload your CMake file and my-target_CLAP will be a buildable target

## The one missing API from "Forkless"

As mentioned above `wrapperType` will be set to `Undefined` using this method. There's two things you can do about this

1. Live with it. It's probably OK! But if you do need to know your wrapper type
2. Use the extension mechanism (to be documented)