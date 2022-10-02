#pragma once

#include <juce_dsp/juce_dsp.h>
#include "ModulatableFloatParameter.h"

class ModulatableFloatParameter;
class GainPlugin : public juce::AudioProcessor,
                   public clap_juce_extensions::clap_juce_audio_processor_capabilities,
                   protected clap_juce_extensions::clap_properties
{
  public:
    GainPlugin();

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameters();

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }

    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return juce::String(); }
    void changeProgramName(int, const juce::String &) override {}

    bool isBusesLayoutSupported(const juce::AudioProcessor::BusesLayout &layouts) const override;
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    void processBlock(juce::AudioBuffer<float> &, juce::MidiBuffer &) override;
    void processBlock(juce::AudioBuffer<double> &, juce::MidiBuffer &) override {}

    bool hasEditor() const override { return true; }
    juce::AudioProcessorEditor *createEditor() override;

    void getStateInformation(juce::MemoryBlock &data) override;
    void setStateInformation(const void *data, int sizeInBytes) override;

    juce::String getPluginTypeString() const;
    auto *getGainParameter() { return gainDBParameter; }
    auto &getValueTreeState() { return vts; }

  private:
    ModulatableFloatParameter *gainDBParameter = nullptr;

    juce::AudioProcessorValueTreeState vts;

    juce::dsp::Gain<float> gain;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GainPlugin)
};
