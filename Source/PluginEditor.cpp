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
	const auto& pars = processor.getParameters();
	for (int i=0;i<pars.size();++i)
	{
		m_parcomps.push_back(std::make_shared<ParameterComponent>(pars[i]));
		m_parcomps.back()->setBounds(1, i * 25, 598, 24);
		addAndMakeVisible(m_parcomps.back().get());
	}
	setSize (600, pars.size()*25);
}

PaulstretchpluginAudioProcessorEditor::~PaulstretchpluginAudioProcessorEditor()
{
}

//==============================================================================
void PaulstretchpluginAudioProcessorEditor::paint (Graphics& g)
{
	g.fillAll(Colours::darkgrey);
}

void PaulstretchpluginAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..
}
