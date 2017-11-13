/*
  ==============================================================================

    This file was auto-generated!

    It contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"


//==============================================================================
PaulstretchpluginAudioProcessorEditor::PaulstretchpluginAudioProcessorEditor (PaulstretchpluginAudioProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{
	addAndMakeVisible(&m_info_label);
	const auto& pars = processor.getParameters();
	for (int i=0;i<pars.size();++i)
	{
		m_parcomps.push_back(std::make_shared<ParameterComponent>(pars[i]));
		m_parcomps.back()->setBounds(1, i * 25, 598, 24);
		addAndMakeVisible(m_parcomps.back().get());
	}
	addAndMakeVisible(&m_rec_enable);
	m_rec_enable.setButtonText("Capture");
	m_rec_enable.addListener(this);
	setSize (600, pars.size()*25+30);
	startTimer(1, 100);
}

PaulstretchpluginAudioProcessorEditor::~PaulstretchpluginAudioProcessorEditor()
{
}

void PaulstretchpluginAudioProcessorEditor::buttonClicked(Button * but)
{
	if (but == &m_rec_enable)
	{
		processor.setRecordingEnabled(but->getToggleState());
	}
}

//==============================================================================
void PaulstretchpluginAudioProcessorEditor::paint (Graphics& g)
{
	g.fillAll(Colours::darkgrey);
}

void PaulstretchpluginAudioProcessorEditor::resized()
{
	m_rec_enable.setBounds(1, getHeight() - 25, 10, 24);
	m_rec_enable.changeWidthToFitText();
	m_info_label.setBounds(m_rec_enable.getRight() + 1, getHeight() - 25, 60, 24);
}

void PaulstretchpluginAudioProcessorEditor::timerCallback(int id)
{
	if (id == 1)
	{
		for (auto& e : m_parcomps)
			e->updateComponent();
		if (processor.isRecordingEnabled() != m_rec_enable.getToggleState())
			m_rec_enable.setToggleState(processor.isRecordingEnabled(), dontSendNotification);
		m_info_label.setText(String(processor.getRecordingPositionPercent()*100.0, 1),dontSendNotification);
	}
}
