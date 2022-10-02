# How To: Parameter Modulation

One of the most exciting features available to CLAP plugins is
[non-destructive parameter modulation](https://www.youtube.com/embed/B2mywWyI9es).

If you're using this repository to build your JUCE-based plugin as a CLAP,
you may be wondering how to get parameter modulation working with your plugin.
This document should be able to help you get started.

It is worth repeating that if JUCE implements CLAP support "natively"
in the future, it is unlikely that the approach outlined here would be
compatible with that implementation.

Note that the workflow below pre-supposes that your plugin uses the modern
JUCE parameter classes. If your plugin is using JUCE's "legacy" parameter
mechanisms, then the CLAP JUCE wrapper cannot support parameter modulation
(although regular parameter functionality will still work).

## Monophonic Modulation

For audio effects, monophonic synthesizers, and global parameters on
polyphonic synthesizers, monophonic parameter modulation can be enabled
as follows.

1. Link your plugin to the `clap_juce_extensions` header.
    - For a CMake project, this can be done by adding
      `target_link_libraries(MyPlugin PUBLIC clap_juce_extensions)`
      to your CMake configuration.
    - For a Projucer project, the user will need to add the
      following include paths to your Projucer configuration:
      - `path/to/clap-juce-extensions/include`
      - `path/to/clap-juce-extensions/clap-libs/clap/include`
      - `path/to/clap-juce-extensions/clap-libs/clap-helpers/include`

      You'll also need to add the following file to your Projucer
      source files:
      - `path/to/clap-juce-extensions/src/extensions/clap-juce-extensions.cpp`

2. Implement a custom parameter type, derived from
   `clap_juce_extensions::clap_juce_parameter_capabilities`.
   An example can be seen [here](../examples/GainPlugin/ModulatableFloatParameter.h).

3. Implement `supportsMonophonicModulation()` and 
   `applyMonophonicModulation()` for your custom parameter.
   The final parameter class should look something like this:

```cpp
#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include <clap-juce-extensions/clap-juce-extensions.h>

class ModulatableFloatParameter : public juce::AudioParameterFloat,
                                  public clap_juce_extensions::clap_juce_parameter_capabilities
{
public:
    ModulatableFloatParameter (/* args */) {} // implement your constructor

    bool supportsMonophonicModulation() override { return true; }

    void applyMonophonicModulation(double modulationValue) override
    {
        // do something with the modulation value here...
    }

    float getCurrentValue() const noexcept
    {
        // return parameter value with modulation applied...
    }
};
```

Any parameters in your plugin that should support non-destructuve modulation
should be derived from your custom parameter class. It's also important to
make sure that when accessing the parameter's value (for example, in your
`processBlock()` method), that you are calling `getCurrentValue()` (or the
equivalent in your code), rather than using the standard JUCE parameter APIs
for getting the parameter value, otherwise the processor will be using the
un-modulated value of the parameter. Notably, this constraint includes
`juce::AudioProcessorValueTreeState::getRawParameterValue()`.

4. Add `CLAP_PROCESS_EVENTS_RESOLUTION_SAMPLES` to your CLAP CMake arguments.
```cmake
# For CMake plugins:
clap_juce_extensions_plugin(TARGET my-target
    CLAP_ID "com.my-cool-plugs.my-target"
    CLAP_FEATURES instrument "virtual analog" gritty basses leads pads
    CLAP_PROCESS_EVENTS_RESOLUTION_SAMPLES 64)

# For Projucer plugins:
create_jucer_clap_target(
    TARGET MyPlugin
    PLUGIN_NAME "My Plugin"
    BINARY_NAME "MyPlugin"
    MANUFACTURER_NAME "My Company"
    MANUFACTURER_CODE Manu
    PLUGIN_CODE Plg1
    VERSION_STRING "1.0.0"
    CLAP_ID "org.mycompany.myplugin"
    CLAP_FEATURES instrument synthesizer
    CLAP_PROCESS_EVENTS_RESOLUTION_SAMPLES 64
)
```
While not strictly necessary, defining `CLAP_PROCESS_EVENTS_RESOLUTION_SAMPLES`
will allow the plugin to respond to modulation events sent from the host at a
finer resolution than the host's block size. Bitwig Studio sends modulation
events at a resolution of 64 samples, but you may want to use a different resolution
size, for example, if your plugin already has a modulation system which uses a
different sample resolution for modulation.

And that's it! Now your plugin should support monophonic modulation!

## Polyphonic Modulation

If your plugin is a polyphonic synthesizer or MIDI effect, you may also want to
implement polyphonic modulation for some parameters. It should be noted that
any parameter which supports polyphonic modulation is also expected to support
monophonic modulation, so make sure to complete the steps from the previous
section before following along here.

1. Implement `supportsPolyphonicModulation()` and 
   `applyPolyphonicModulation()` for your custom parameter type.
```cpp
class PolyModulatableFloatParameter : public ModulatableFloatParameter
{
public:
    PolyModulatableFloatParameter (/* args */) {} // implement your constructor

    bool supportsPolyphonicModulation() override { return true; }

    void applyPolyphonicModulation(int32_t note_id, int16_t port_index,
                                   int16_t channel, int16_t key,
                                   double amount) override
    {
        // apply the modulation here
    }

    float getCurrentValuePoly(int32_t note_id, int16_t port_index,
                              int16_t channel, int16_t key) const noexcept
    {
        // return parameter value with modulation applied
        // for this note, channel, etc ...
    }
};
```

2. Next we need our plugin to keep track of note IDs so we can know
   which modulation events should apply to which notes. This can be
   done by implementing custom event handlers for incoming note events.
```cpp
class MyPlugin : public juce::AudioProcessor,
                 public clap_juce_extensions::clap_juce_audio_processor_capabilities
{
public:
    // All your other code here...

    bool supportsDirectEvent(uint16_t space_id, uint16_t type) override
    {
        if (space_id != CLAP_CORE_EVENT_SPACE_ID)
            return false; // only handle events in the core namespace

        // do custom handling for note events
        return type == CLAP_EVENT_NOTE_ON
            && type == CLAP_EVENT_NOTE_OFF;
    }

    void handleDirectEvent(const clap_event_header_t *evt, int sampleOffset) override
    {
        if (evt->space_id != CLAP_CORE_EVENT_SPACE_ID)
            return;

        switch (evt->type)
        {
        case CLAP_EVENT_NOTE_ON:
        {
            auto nevt = reinterpret_cast<const clap_event_note *>(evt);
            const auto note_id = nevt->note_id;
            // start the note here...
        }
        break;
        case CLAP_EVENT_NOTE_OFF:
        {
            auto nevt = reinterpret_cast<const clap_event_note *>(evt);
            const auto note_id = nevt->note_id;
            // end the note here...
        }
        break;
        };
    }
};
```

3. Finally, we need to tell the host when each note is ending, by adding note
   end events to the output event queue:
```cpp
class MyPlugin : public juce::AudioProcessor,
                 public clap_juce_extensions::clap_juce_audio_processor_capabilities
{
public:
    // All your other code here...

    bool supportsOutboundEvents() override { return true; }
    void addOutboundEventsToQueue(const clap_output_events *out_events,
                                  const juce::MidiBuffer &midiBuffer, int sampleOffset) override
    {
        // Assuming the plugin has implemented some container `notesThatEndedDuringLastBlock`
        // to hold information for all the notes that ended during the previous `processBlock()`
        for (auto& noteEndEvent : notesThatEndedDuringLastBlock)
        {
            auto evt = clap_event_note();
            evt.header.size = sizeof(clap_event_note);
            evt.header.type = (uint16_t)CLAP_EVENT_NOTE_END;
            evt.header.space_id = CLAP_CORE_EVENT_SPACE_ID;
            evt.header.flags = 0;

            // The way the CLAP/JUCE wrapper is able to accomplish sample-accurate
            // parameter automation/modulation is by splitting up the incoming CLAP
            // audio block into multiple JUCE `processBlock()` calls. So when adding
            // events to the output event queue, we need to take the note end time
            // relative to the last `processBlock()` call and add the sample offset
            // to get the correct event time relative to the start of the CLAP block.
            evt.header.time = uint32_t(noteEndEvent.noteEndTime + sampleOffset);

            // some of these values may be zero, for example
            // your synth might not have a concept of "note ports".
            evt.port_index = noteEndEvent.notePort;
            evt.channel = noteEndEvent.channel;
            evt.key = noteEndEvent.key;
            evt.note_id = noteEndEvent.noteID;
            evt.velocity = noteEndEvent.velocity;

            out_events->try_push(out_events, reinterpret_cast<const clap_event_header *>(&evt));
        }
    }
};
```

For plugins that produce MIDI, extra care needs to be taken during this step.
Any MIDI events in the `juce::MidiBuffer` which is passed to
`addOutboundEventsToQueue()` will also need to be added to the output event
queue, however, since all the events in the queue need to be ordered sequentially, 
the implementer may need to "interleave" the note end events with the MIDI events
in order to make sure all the events are in the correct order.

## Troubleshooting

If you run into difficulties when trying to implement parameter modulation in your
plugin, please create a GitHub Issue in this repo. If the wrapper API for supporting
parameter modulation appears to be incomplete, Pull Requests for improving the API
are welcome!
