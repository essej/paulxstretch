/*
 * This file allows our CLAP extension to load the objective
 * C extensions that JUCE uses to create a UI on mac in the
 * VST3 and VST implementations. Basically it provides the pair
 * of functions to attach our NSView to the parent window properly
 * from the juce editor. This code is maintained by the JUCE team
 * but we need to link it here also, so create this little stub
 * which (for this one file only) tells JUCE I'm a VST3 and makes
 * the objective C symbols available.
 */

#define JucePlugin_Build_VST3 1
#include "juce_audio_plugin_client/VST/juce_VST_Wrapper.mm"
