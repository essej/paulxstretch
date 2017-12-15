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

class SpectralVisualizer : public Component
{
public:
	SpectralVisualizer();
	void setState(const ProcessParameters& pars, int nfreqs, double samplerate);
	void paint(Graphics& g) override;
	
private:
	Image m_img;
	std::vector<REALTYPE> m_insamples,m_freqs1, m_freqs2, m_freqs3;
	std::unique_ptr<FFT> m_fft;
	int m_nfreqs = 0;
	double m_elapsed = 0.0;
};

inline void attachCallback(Button& button, std::function<void()> callback)
{
	struct ButtonCallback : public Button::Listener,
		private ComponentListener
	{
		ButtonCallback(Button& b, std::function<void()> f) : target(b), fn(f)
		{
			target.addListener(this);
			target.addComponentListener(this);
		}

		~ButtonCallback()
		{
			target.removeListener(this);
		}

		void componentBeingDeleted(Component&) override { delete this; }
		void buttonClicked(Button*) override { fn(); }

		Button& target;
		std::function<void()> fn;

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ButtonCallback)
	};

	new ButtonCallback(button, callback);
}

class MySlider : public Slider
{
public:
	MySlider() {}
	MySlider(NormalisableRange<float>* range) : m_range(range)
	{
	}
	double proportionOfLengthToValue(double x) override
	{
		if (m_range)
			return m_range->convertFrom0to1(x);
		return Slider::proportionOfLengthToValue(x);
	}
	double valueToProportionOfLength(double x) override
	{
		if (m_range)
			return m_range->convertTo0to1(x);
		return Slider::valueToProportionOfLength(x);
	}
private:
	NormalisableRange<float>* m_range = nullptr;
};

class ParameterComponent : public Component,
	public Slider::Listener, public Button::Listener
{
public:
	ParameterComponent(AudioProcessorParameter* par, bool notifyOnlyOnRelease) : m_par(par)
	{
		addAndMakeVisible(&m_label);
		m_label.setText(par->getName(50),dontSendNotification);
		AudioParameterFloat* floatpar = dynamic_cast<AudioParameterFloat*>(par);
		if (floatpar)
		{
			m_slider = std::make_unique<MySlider>(&floatpar->range);
			m_notify_only_on_release = notifyOnlyOnRelease;
			m_slider->setRange(floatpar->range.start, floatpar->range.end, floatpar->range.interval);
			m_slider->setValue(*floatpar, dontSendNotification);
			m_slider->addListener(this);
			addAndMakeVisible(m_slider.get());
		}
		AudioParameterInt* intpar = dynamic_cast<AudioParameterInt*>(par);
		if (intpar)
		{
			m_slider = std::make_unique<MySlider>();
			m_notify_only_on_release = notifyOnlyOnRelease;
			m_slider->setRange(intpar->getRange().getStart(), intpar->getRange().getEnd(), 1.0);
			m_slider->setValue(*intpar, dontSendNotification);
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
			m_togglebut = std::make_unique<ToggleButton>();
			m_togglebut->setToggleState(*boolpar, dontSendNotification);
			m_togglebut->addListener(this);
			addAndMakeVisible(m_togglebut.get());
		}
	}
	void resized() override
	{
		m_label.setBounds(0, 0, 200, 24);
		if (m_slider)
			m_slider->setBounds(m_label.getRight() + 1, 0, getWidth() - 2 - m_label.getWidth(), 24);
		if (m_togglebut)
			m_togglebut->setBounds(m_label.getRight() + 1, 0, getWidth() - 2 - m_label.getWidth(), 24);
	}
	void sliderValueChanged(Slider* slid) override
	{
		if (m_notify_only_on_release == true)
			return;
		AudioParameterFloat* floatpar = dynamic_cast<AudioParameterFloat*>(m_par);
		if (floatpar!=nullptr)
			*floatpar = slid->getValue();
		AudioParameterInt* intpar = dynamic_cast<AudioParameterInt*>(m_par);
		if (intpar != nullptr)
			*intpar = slid->getValue();
	}
	void sliderDragStarted(Slider* slid) override
	{
		m_dragging = true;
	}
	void sliderDragEnded(Slider* slid) override
	{
		m_dragging = false;
		if (m_notify_only_on_release == false)
			return;
		AudioParameterFloat* floatpar = dynamic_cast<AudioParameterFloat*>(m_par);
		if (floatpar!=nullptr)
			*floatpar = slid->getValue();
		AudioParameterInt* intpar = dynamic_cast<AudioParameterInt*>(m_par);
		if (intpar != nullptr)
			*intpar = slid->getValue();
	}
	void buttonClicked(Button* but) override
	{
		AudioParameterBool* boolpar = dynamic_cast<AudioParameterBool*>(m_par);
		if (m_togglebut != nullptr && m_togglebut->getToggleState() != *boolpar)
		{
			*boolpar = m_togglebut->getToggleState();
		}
	}
	void updateComponent()
	{
		AudioParameterFloat* floatpar = dynamic_cast<AudioParameterFloat*>(m_par);
		if (floatpar!=nullptr && m_slider != nullptr && m_dragging == false && (float)m_slider->getValue() != *floatpar)
		{
			m_slider->setValue(*floatpar, dontSendNotification);
		}
		AudioParameterInt* intpar = dynamic_cast<AudioParameterInt*>(m_par);
		if (intpar != nullptr && m_slider != nullptr && m_dragging == false && (int)m_slider->getValue() != *intpar)
		{
			m_slider->setValue(*intpar, dontSendNotification);
		}
		AudioParameterBool* boolpar = dynamic_cast<AudioParameterBool*>(m_par);
		if (m_togglebut != nullptr && m_togglebut->getToggleState() != *boolpar)
		{
			m_togglebut->setToggleState(*boolpar, dontSendNotification);
		}
	}
private:
	Label m_label;
	AudioProcessorParameter* m_par = nullptr;
	std::unique_ptr<MySlider> m_slider;
	std::unique_ptr<ComboBox> m_combobox;
	std::unique_ptr<ToggleButton> m_togglebut;
	bool m_notify_only_on_release = false;
	bool m_dragging = false;
};

class MyThumbCache : public AudioThumbnailCache
{
public:
	MyThumbCache() : AudioThumbnailCache(100) { /*Logger::writeToLog("Constructed AudioThumbNailCache");*/ }
	~MyThumbCache() { /*Logger::writeToLog("Destructed AudioThumbNailCache");*/ }
};

class WaveformComponent : public Component, public ChangeListener, public Timer
{
public:
	WaveformComponent(AudioFormatManager* afm);
	~WaveformComponent();
	void changeListenerCallback(ChangeBroadcaster* cb) override;
	void paint(Graphics& g) override;
	void setAudioFile(File f);
	const File& getAudioFile() const { return m_curfile; }
    bool isUsingAudioBuffer() const { return m_using_audio_buffer; }
    void setAudioBuffer(AudioBuffer<float>* buf, int samplerate, int len);
	void beginAddingAudioBlocks(int channels, int samplerate, int totalllen);
	void addAudioBlock(AudioBuffer<float>& buf, int samplerate, int pos);
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
		if (m_lock_timesel_set == true)
			return;
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
	void setRecordingPosition(double pos) { m_rec_pos = pos; }
private:
	SharedResourcePointer<MyThumbCache> m_thumbcache;

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
	double m_rec_pos = 0.0;
	bool m_lock_timesel_set = false;
    bool m_using_audio_buffer = false;
};

class SpectralChainEditor : public Component
{
public:
	SpectralChainEditor() {}
	void paint(Graphics& g) override;
	void setSource(StretchAudioSource* src)
	{
		m_src = src;
		m_order = m_src->getSpectrumProcessOrder();
		repaint();
	}
	void mouseDown(const MouseEvent& ev) override;
	void mouseDrag(const MouseEvent& ev) override;
	void mouseUp(const MouseEvent& ev) override;
private:
	StretchAudioSource * m_src = nullptr;
	bool m_did_drag = false;
	int m_cur_index = -1;
	int m_drag_x = 0;
	std::vector<int> m_order;
	void drawBox(Graphics& g, int index, int x, int y, int w, int h);
};

class MyDynamicObject : public DynamicObject
{
public:
	bool hasMethod(const Identifier& methodName) const override
	{
		if (methodName == Identifier("setLabelBounds") ||
			methodName == Identifier("setComponentBounds"))
			return true;
		return false;
	}
	var invokeMethod(Identifier methodName,
		const var::NativeFunctionArgs& args) override
	{
		return var();
	}
};

class PaulstretchpluginAudioProcessorEditor  : public AudioProcessorEditor, 
	public MultiTimer
{
public:
    PaulstretchpluginAudioProcessorEditor (PaulstretchpluginAudioProcessor&);
    ~PaulstretchpluginAudioProcessorEditor();
	void paint (Graphics&) override;
    void resized() override;
	void timerCallback(int id) override;
	void setAudioFile(File f);
	void setAudioBuffer(AudioBuffer<float>* buf, int samplerate, int len);
	void beginAddingAudioBlocks(int channels, int samplerate, int totalllen);
	void addAudioBlock(AudioBuffer<float>& buf, int samplerate, int pos);
	WaveformComponent m_wavecomponent;
private:
    PaulstretchpluginAudioProcessor& processor;
	std::vector<std::shared_ptr<ParameterComponent>> m_parcomps;
	SpectralVisualizer m_specvis;
	
	TextButton m_import_button;
	Label m_info_label;
	SpectralChainEditor m_spec_order_ed;
	void chooseFile();
    String m_last_err;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PaulstretchpluginAudioProcessorEditor)
};

