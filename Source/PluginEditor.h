/*
  ==============================================================================

    This file was auto-generated!

    It contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "PluginProcessor.h"
#include <memory>
#include <vector>

class ParameterComponent : public Component,
	public Slider::Listener
{
public:
	ParameterComponent(AudioProcessorParameter* par) : m_par(par)
	{
		addAndMakeVisible(&m_label);
		m_label.setText(par->getName(50),dontSendNotification);
		AudioParameterFloat* floatpar = dynamic_cast<AudioParameterFloat*>(par);
		if (floatpar)
		{
			m_slider = std::make_unique<Slider>();
			m_slider->setRange(floatpar->range.start, floatpar->range.end, floatpar->range.interval);
			m_slider->setValue(*floatpar, dontSendNotification);
			m_slider->addListener(this);
			addAndMakeVisible(m_slider.get());
		}
		AudioParameterChoice* choicepar = dynamic_cast<AudioParameterChoice*>(par);
		if (choicepar)
		{

		}
		AudioParameterBool* boolpar = dynamic_cast<AudioParameterBool*>(par);
		if (boolpar)
		{

		}
	}
	void resized() override
	{
		m_label.setBounds(0, 0, 200, 24);
		if (m_slider)
			m_slider->setBounds(m_label.getRight() + 1, 0, getWidth() - 2 - m_label.getWidth(), 24);
	}
	void sliderValueChanged(Slider* slid) override
	{
		AudioParameterFloat* floatpar = dynamic_cast<AudioParameterFloat*>(m_par);
		*floatpar = slid->getValue();
	}
	void updateComponent()
	{
		AudioParameterFloat* floatpar = dynamic_cast<AudioParameterFloat*>(m_par);
		if (m_slider != nullptr && (float)m_slider->getValue() != *floatpar)
		{
			m_slider->setValue(*floatpar, dontSendNotification);
		}
	}
private:
	Label m_label;
	AudioProcessorParameter* m_par = nullptr;
	std::unique_ptr<Slider> m_slider;
	std::unique_ptr<ComboBox> m_combobox;
	std::unique_ptr<ToggleButton> m_togglebut;
};

class PaulstretchpluginAudioProcessorEditor  : public AudioProcessorEditor, 
	public MultiTimer, public Button::Listener
{
public:
    PaulstretchpluginAudioProcessorEditor (PaulstretchpluginAudioProcessor&);
    ~PaulstretchpluginAudioProcessorEditor();
	void buttonClicked(Button* but) override;
    //==============================================================================
    void paint (Graphics&) override;
    void resized() override;
	void timerCallback(int id) override;
private:
    PaulstretchpluginAudioProcessor& processor;
	std::vector<std::shared_ptr<ParameterComponent>> m_parcomps;
	ToggleButton m_rec_enable;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PaulstretchpluginAudioProcessorEditor)
};
