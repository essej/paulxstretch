#pragma once

#include <juce_audio_utils/juce_audio_utils.h>

JUCE_BEGIN_IGNORE_WARNINGS_GCC_LIKE("-Wunused-parameter", "-Wextra-semi", "-Wnon-virtual-dtor")
#include <clap-juce-extensions/clap-juce-extensions.h>
JUCE_END_IGNORE_WARNINGS_GCC_LIKE

class ModulatableFloatParameter : public juce::AudioParameterFloat,
                                  public clap_juce_extensions::clap_juce_parameter_capabilities
{
  public:
    ModulatableFloatParameter(const juce::String &parameterID, const juce::String &parameterName,
                              const juce::NormalisableRange<float> &valueRange,
                              float defaultFloatValue,
                              const std::function<juce::String(float)> &valueToTextFunction,
                              std::function<float(const juce::String &)> &&textToValueFunction)
#if JUCE_VERSION < 0x070000
        : juce::AudioParameterFloat(
              parameterID, parameterName, valueRange, defaultFloatValue, juce::String(),
              AudioProcessorParameter::genericParameter,
              valueToTextFunction == nullptr
                  ? std::function<juce::String(float v, int)>()
                  : [valueToTextFunction](float v, int) { return valueToTextFunction(v); },
              std::move(textToValueFunction)),
#else
        : juce::AudioParameterFloat(
              parameterID, parameterName, valueRange, defaultFloatValue,
              juce::AudioParameterFloatAttributes()
                  .withStringFromValueFunction(
                      [valueToTextFunction](float v, int) { return valueToTextFunction(v); })
                  .withValueFromStringFunction(std::move(textToValueFunction))),
#endif
          unsnappedDefault(valueRange.convertTo0to1(defaultFloatValue)),
          normalisableRange(valueRange)
    {
    }

    float getDefaultValue() const override { return unsnappedDefault; }

    bool supportsMonophonicModulation() override { return true; }

    void applyMonophonicModulation(double modulationValue) override
    {
        modulationAmount = (float)modulationValue;
    }

    float getCurrentValue() const noexcept
    {
        return normalisableRange.convertFrom0to1(
            juce::jlimit(0.0f, 1.0f, normalisableRange.convertTo0to1(get()) + modulationAmount));
    }

  private:
    const float unsnappedDefault;
    const juce::NormalisableRange<float> normalisableRange;

    float modulationAmount = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModulatableFloatParameter)
};
