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

class WaveformComponent : public Component, public ChangeListener, public Timer
{
public:
	WaveformComponent(AudioFormatManager* afm);
	~WaveformComponent();
	void changeListenerCallback(ChangeBroadcaster* cb) override;
	void paint(Graphics& g) override;
	void setAudioFile(File f);
	void setAudioBuffer(AudioBuffer<float>* buf, int samplerate, int len);
	void timerCallback() override;
	std::function<double()> CursorPosCallback;
	std::function<void(double)> SeekCallback;
	std::function<void(Range<double>, int)> TimeSelectionChangedCallback;
	void mouseDown(const MouseEvent& e) override;
	void mouseUp(const MouseEvent& e) override;
	void mouseDrag(const MouseEvent& e) override;
	void mouseMove(const MouseEvent& e) override;
	Range<double> getTimeSelection()
	{
		if (m_time_sel_start >= 0.0 && m_time_sel_end>m_time_sel_start + 0.001)
			return { m_time_sel_start, m_time_sel_end };
		return { 0.0, 1.0 };
	}
	void setTimeSelection(Range<double> rng)
	{
		if (rng.isEmpty())
			rng = { -1.0,1.0 };
		m_time_sel_start = rng.getStart();
		m_time_sel_end = rng.getEnd();
		repaint();
	}
	void setFileCachedRange(std::pair<Range<double>, Range<double>> rng);
	void setTimerEnabled(bool b);
	void setViewRange(Range<double> rng);
	Value ShowFileCacheRange;
private:
	AudioThumbnailCache m_thumbcache;

	std::unique_ptr<AudioThumbnail> m_thumb;
	Range<double> m_view_range{ 0.0,1.0 };
	int m_time_sel_drag_target = 0;
	double m_time_sel_start = -1.0;
	double m_time_sel_end = -1.0;
	double m_drag_time_start = 0.0;
	bool m_mousedown = false;
	bool m_didseek = false;
	bool m_didchangetimeselection = false;
	int m_topmargin = 0;
	int getTimeSelectionEdge(int x, int y);
	std::pair<Range<double>, Range<double>> m_file_cached;
	File m_curfile;
	Image m_waveimage;
	OpenGLContext m_ogl;
	bool m_use_opengl = false;
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
	void setAudioFile(File f);
	void setAudioBuffer(AudioBuffer<float>* buf, int samplerate, int len);
private:
    PaulstretchpluginAudioProcessor& processor;
	std::vector<std::shared_ptr<ParameterComponent>> m_parcomps;
	ToggleButton m_rec_enable;
	Label m_info_label;
	WaveformComponent m_wavecomponent;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PaulstretchpluginAudioProcessorEditor)
};
