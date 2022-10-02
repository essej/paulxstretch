#include "PluginEditor.h"

PluginEditor::PluginEditor(GainPlugin &plug) : juce::AudioProcessorEditor(plug), plugin(plug)
{
    setSize(300, 300);

    addAndMakeVisible(gainSlider);
    gainSlider.setSliderStyle(juce::Slider::SliderStyle::RotaryHorizontalVerticalDrag);
    gainSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 100, 20);

    auto *gainParameter = plugin.getGainParameter();
    sliderAttachment =
        std::make_unique<juce::SliderParameterAttachment>(*gainParameter, gainSlider, nullptr);

    plugin.getValueTreeState().addParameterListener(gainParameter->paramID, this);
}

PluginEditor::~PluginEditor()
{
    auto *gainParameter = plugin.getGainParameter();
    plugin.getValueTreeState().removeParameterListener(gainParameter->paramID, this);
}

void PluginEditor::resized()
{
    gainSlider.setBounds(juce::Rectangle<int>{200, 200}.withCentre(getLocalBounds().getCentre()));
}

void PluginEditor::paint(juce::Graphics &g)
{
    g.fillAll(juce::Colours::grey);

    g.setColour(juce::Colours::black);
    g.setFont(25.0f);
    const auto titleBounds = getLocalBounds().removeFromTop(30);
    const auto titleText = "Gain Plugin " + plugin.getPluginTypeString();
    g.drawFittedText(titleText, titleBounds, juce::Justification::centred, 1);
}

void PluginEditor::parameterChanged(const juce::String &, float)
{
    // visual feedback so we know the parameter listeners are getting called:
    struct FlashComponent : Component
    {
        void paint(juce::Graphics &g) override { g.fillAll(juce::Colours::red); }
    } flashComp;

    addAndMakeVisible(flashComp);
    flashComp.setBounds(juce::Rectangle<int>{getWidth() - 10, 0, 10, 10});

    auto &animator = juce::Desktop::getInstance().getAnimator();
    animator.fadeOut(&flashComp, 100);
}
