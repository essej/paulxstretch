# JUCE6 and 7 Unofficial CLAP Plugin Support

This is a set of code which, combined with a JUCE 6 or JUCE 7 plugin project, allows you to build a CLAP plugin. It
is licensed under the MIT license, and can be used for both open and closed source projects.

We are labeling it 'unofficial' for four reasons

1. It is not supported by the JUCE team,
2. There are some JUCE features which we have not translated to CLAP yet,
3. It presents a set of completely optional extensions which break JUCE abstractions to allow extended CLAP feature
   support and
4. It does not support JUCE-based CLAP hosting

Despite those caveats, the basic use of this library has allowed a wide variety
of synths and effects to generate a CLAP from their JUCE program, including Surge, B-Step,
Monique, several ChowDSP plugins, Dexed and more.

By far the best solution for CLAP in JUCE would be full native support by the JUCE team. Until such a time as that
happens (and it may never happen), this code may help you if you have a JUCE plugin and want to generate a CLAP. We are
happy to merge changes and answer questions as you try to use it. Please feel free to raise github issues in this repo.

This version is based off of CLAP 1.0 and generates plugins which work in BWS 4.3beta5 and later, as well as
other CLAP 1.0 DAWs such as MultitrackStudio.

## Basics: Using these extensions to build a CLAP

### CMake

Given a starting point of a JUCE plugin using CMake which can build a VST3, AU, Standalone and so forth with
`juce_plugin`, building a CLAP is a simple exercise of checking out this CLAP extension code
somewhere in your dev environment, setting a few CMake variables, and adding a couple of lines to your CMake file.

The instructions are as follows:

1. Add `https://github.com/free-audio/clap-juce-extensions.git` as a submodule of your project, or otherwise make the
   source available to your cmake (CPM, side by side check out in CI, etc...).
2. Load the `clap-juce-extension` in your CMake after you have loaded JUCE. For instance, you could do

```cmake
add_subdirectory(libs/JUCE) # this is however you load juce
add_subdirectory(libs/clap-juce-extensions EXCLUDE_FROM_ALL) 
```

3. Create your JUCE plugin as normal with flags and formats using the `juce_plugin` CMake function
4. After your `juce_plugin` code, add the following lines (or similar)
   to your CMake (a list of pre-defined CLAP
   features can be found [here](https://github.com/free-audio/clap/blob/main/include/clap/plugin-features.h)):

```cmake
    clap_juce_extensions_plugin(TARGET my-target
        CLAP_ID "com.my-cool-plugs.my-target"
        CLAP_FEATURES instrument "virtual analog" gritty basses leads pads)
```

5. Reload your CMake file and you will have a new target `my-target_CLAP` which will build a CLAP and leave
   it side-by-side with your AU, Standalone, VST3, and so forth. Load that CLAP into a DAW and give it a whirl!

### Projucer

Given a starting point of a JUCE plugin using the Projucer, it is possible to build a CLAP plugin by adding
a small CMake configuration alongside the Projucer build setup.

1. Build your Projucer-based plugin.
2. Create `CMakeLists.txt` file in the same directory as your `.jucer` file. Here's an example CMakeLists.txt:

```cmake
cmake_minimum_required(VERSION 3.15)

# Make sure to set the same MacOS deployment target as you have set in the Projucer
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.12" CACHE STRING "Minimum OS X deployment target")

# If the Projucer is using "static runtime" for Visual Studio:
# set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>" CACHE STRING "Runtime")
# set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Release>:Release>" CACHE STRING "Runtime")

# If running into issues when Xcode tries to codesign the CLAP plugin, you may want to add these lines:
# set(CMAKE_XCODE_ATTRIBUTE_CODE_SIGNING_REQUIRED "NO")
# set(CMAKE_XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED "NO")

project(MyPlugin VERSION 1.0.0)

set(PATH_TO_JUCE path/to/JUCE)
set(PATH_TO_CLAP_EXTENSIONS path/to/clap-juce-extensions)

# define the exporter types used in your Projucer configuration
if (APPLE)
    set(JUCER_GENERATOR "Xcode")
elseif (WIN32)
    set(JUCER_GENERATOR "VisualStudio2019")
else () # Linux
    set(JUCER_GENERATOR "LinuxMakefile")
endif ()

include(${PATH_TO_CLAP_EXTENSIONS}/cmake/JucerClap.cmake)
create_jucer_clap_target(
        TARGET MyPlugin # "Binary Name" in the Projucer
        PLUGIN_NAME "My Plugin"
        BINARY_NAME "MyPlugin" # Name of the resulting plugin binary
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

If you would like to use the [CLAP extensions API](#the-extensions-api), the necessary source
files must be added to the plugin's Projucer configuration.

### Arguments to CLAP CMake functions

In addition to `CLAP_ID` and `CLAP_FEATURES` described above the following arguments
are available

* `CLAP_MANUAL_URL` and `CLAP_SUPPORT_URL` generate the urls in your description
* `CLAP_MISBHEAVIOUR_HANDLER_LEVEL` can be set to `Terminate` or `Ignore` (default
  is `Ignore`) to choose your behaviour for a misbehaving host.
* `CLAP_CHECKING_LEVEL` can be set to `None`, `Minimal`, or `Maximal` (default is
  `Minimal`) to choose the level of sanity checks enabled for the plugin.
* `CLAP_PROCESS_EVENTS_RESOLUTION_SAMPLES` can be set to any integer value to choose the
  resolution (in samples) used by the wrapper for doing sample-accurate event processing.
  Setting the value to `0` (the default value) will turn off sample-accurate event processing.
* `CLAP_ALWAYS_SPLIT_BLOCK` can be set to `1` (on), or `0` (off, default), to tell the
  wrapper to _always_ attempt to split incoming audio buffers into chunks of size
  `CLAP_PROCESS_EVENTS_RESOLUTION_SAMPLES`, regardless of any input events being
  sent from the host. Note that if the block size provided by the host is not an
  even multiple of `CLAP_PROCESS_EVENTS_RESOLUTION_SAMPLES`, the plugin may be
  required to process a chunk smaller than the chosen resolution.
* `CLAP_USE_JUCE_PARAMETER_RANGES` can be set to `ALL`, `DISCRETE` or `OFF` (default) to
  tell the wrapper to use JUCE's parameter ranges for all parameters, discrete parameters only,
  or no parameters. When not using JUCE's parameter ranges, the plugin will communicate with
  the host using 0-1 parameter ranges for the given parameter,

## Risks of using this library

Using this library is, of course, not without risks. There could easily be bugs we haven't found and there are
APIs we don't cover. We are happy to discuss, investigate, and work to fix any of those.

The biggest risk, though, involves JUCE team providing official support in a way which is fundamentally incompatible
with these wrappers. As of this writing, the JUCE team has not committed to supporting CLAP in JUCE 7 or any future
version, although they are aware of the project. But if the JUCE team did provide official future support, it is not
clear that your CLAP plugin which resulted from their official support would work in the same way as the plugin
generated by this library would.

There are a couple of mitigants to that risk.

Most importantly, in the three critical places a DAW interacts with a plugin - CLAP ID, parameter IDs, and state
streaming -
we have endeavoured to write in as JUCE-natural a way as possible.

1. The CLAP ID is just a CMake parameter, as we expect it would be in an official build.
2. The parameter IDs we use uses the [internal JUCE hashing mechanism to generate
   our `uint32_t`](https://github.com/free-audio/clap-juce-extensions/blob/85bc0d56dc784a5f1271602db46f0748954b180e/src/wrapper/clap-juce-wrapper.cpp#L198)
   just like
   the [current VST3 wrapper does](https://github.com/juce-framework/JUCE/blob/2f980209cc4091a4490bb1bafc5d530f16834e58/modules/juce_audio_plugin_client/VST3/juce_VST3_Wrapper.cpp#L585).
3. Our stream implementation
   transparently [calls `AudioProcessor::setStateInformation` and `AudioProcessor::getStateInformation`](https://github.com/free-audio/clap-juce-extensions/blob/85bc0d56dc784a5f1271602db46f0748954b180e/src/wrapper/clap-juce-wrapper.cpp#L930)
   with no intervening
   modification of the stream.

While there is no guarantee that an official JUCE implementation, if it were to exist, would make these choices,
it seems quite natural that it would, and in that case, your plugin would continue to work.

If, however, you use the extensions detailed below - which allows features outside of JUCE like note expressions,
sample accurate automation, and polyphonic and non-destructive modulation - there is very little assurance we can
give you that an official JUCE implementation, if it were to exist, would work with your code without modification,
or even that it would support those features at all. That would leave such a synth (of which Surge is the primary
example today) relying on these wrappers still.

## Major Missing API points

1. We have not tested any JUCE version earlier than 6.0.7, and plugins which use deprecated APIs may not work
2. The [`AudioProcessor::WrapperType`](https://docs.juce.com/master/classAudioProcessor.html#a2e1b21b8831ac529965abffc96223dcf)
   API doesn't support CLAP. All CLAP plugins will define a `wrapperType` of `wrapperType_Undefined`. We do provide
   a workaround for using our extensions mechanism, below.
3. Several parameter features - including discrete (stepped) parameters - don't translate from JUCE to CLAP
   in our adapter (although they are supported in the CLAP API of course). We would love a test plugin to help us
   resolve this.

## The `clap_juce_extensions` API for extended CLAP capabilities in JUCE

There are a set of things which JUCE doesn't support which CLAP does. Rather than not support them in our
plugins, we've decided to create an specific extensions API which allow you to decorate JUCE
classes with extended capabilities. These are a set of classes which your AudioProcessor can
implement and, if it does, then the CLAP JUCE wrapper will call the associated functions.

The extension are in "include/clap-juce-extensions.h" and are documented there, but currently have
three classes

- `clap_juce_extensions::clap_properties`
    - if you subclass this your AudioProcessor will have a collection of members which give you extra CLAP info
    - Most usefully, you get an `is_clap` member which is false if not a CLAP and true if it is, which works around
      the fact that our 'forkless' approach doesn't let us add a `AudioProcessor::WrapperType` to the JUCE API
- `clap_juce_extensions::clap_juce_audio_processor_capabilities`
    - these are a set of advanced extensions which let you optionally interact more directly with the CLAP API
      and are mostly useful for advanced features like non-destructive modulation and note expression support
- `clap_juce_extensions::clap_juce_parameter_capabilities`
    - If your AudioProcessorParameter subclass implements this API, you can share extended CLAP information on
      a parameter by parameter basis

As an example, here's how to use `clap_properties` to work around `AudioProcessor::WrapperType` being `Undefined` in the
forkless
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
            return "CLAP";
    
        return juce::AudioProcessor::getWrapperTypeDescription (wrapperType);
    }
    
    ...
};
```

If you are interested in using these extensions, please consult the documentation in the
[clap-juce-extensions header.](https://github.com/free-audio/clap-juce-extensions/blob/main/include/clap-juce-extensions/clap-juce-extensions.h)
The [Surge XT Synthesizer](https://github.com/surge-synthesizer/surge) is a worked example of using many of these.
We are also happy to discuss them - reach out in the issues here or in a shared discord server.

## Technical Detail: The "Forkless" approach

There's a couple of ways we could have gone adding experimental JUCE support. The way the LV2 extensions to JUCE work
requires a forked JUCE which places LV2 support fully inside the JUCE ecosystem at the cost of maintaining a fork (and
not allowing folks with their own forks to easily use LV2). We instead chose an 'out-of-JUCE' approach which has the
following pros and cons

Pros:

* You can use any JUCE 6 or 7 / CMake method you want and don't need to use our branch.
* We don't have to update our fork to pull latest JUCE features; you don't have to use our fork and choices to build
  your plugin.

Cons:

* The CMake API is not consistent. Rather than add "CLAP" as a plugin type, you need a couple of extra lines of CMake to
  activate your CLAP.
* We cannot support the `AudioProcessor::WrapperType` API, as discussed above.
