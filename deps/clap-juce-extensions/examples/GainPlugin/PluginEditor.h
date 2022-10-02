#pragma once

#include "GainPlugin.h"

class PluginEditor : public juce::AudioProcessorEditor,
                     private juce::AudioProcessorValueTreeState::Listener
{
  public:
    explicit PluginEditor(GainPlugin &plugin);
    ~PluginEditor() override;

    void resized() override;
    void paint(juce::Graphics &g) override;

  private:
    void parameterChanged(const juce::String &parameterID, float newValue) override;

    GainPlugin &plugin;

    juce::Slider gainSlider;
    std::unique_ptr<juce::SliderParameterAttachment> sliderAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginEditor)
};
