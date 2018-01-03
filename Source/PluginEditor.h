/*
Copyright (C) 2006-2011 Nasca Octavian Paul
Author: Nasca Octavian Paul

Copyright (C) 2017 Xenakios

This program is free software; you can redistribute it and/or modify
it under the terms of version 2 of the GNU General Public License
as published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License (version 2) for more details.

You should have received a copy of the GNU General Public License (version 2)
along with this program; if not, write to the Free Software Foundation,
Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
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

class MySlider : public Slider
{
public:
	MySlider() {}
	MySlider(NormalisableRange<float>* range);
	double proportionOfLengthToValue(double x) override;
	double valueToProportionOfLength(double x) override;
private:
	NormalisableRange<float>* m_range = nullptr;
};

class ParameterComponent : public Component,
	public Slider::Listener, public Button::Listener
{
public:
	ParameterComponent(AudioProcessorParameter* par, bool notifyOnlyOnRelease);
	void resized() override;
	void sliderValueChanged(Slider* slid) override;
	void sliderDragStarted(Slider* slid) override;
	void sliderDragEnded(Slider* slid) override;
	void buttonClicked(Button* but) override;
	void updateComponent();
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
	Range<double> getTimeSelection();
	void setTimeSelection(Range<double> rng);
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
	void setSource(StretchAudioSource* src);
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

class ParamLayoutInfo
{
public:
	ParamLayoutInfo() {}
	ParamLayoutInfo(int c, int x, int y, int w, int h) :
		m_comp(c), m_col(x), m_row(y), m_w(w), m_h(h) {}
	int m_comp = 0;
	int m_col = 0;
	int m_row = 0;
	int m_w = 1;
	int m_h = 1;
};

class PaulstretchpluginAudioProcessorEditor  : public AudioProcessorEditor, 
	public MultiTimer, public FileDragAndDropTarget, public DragAndDropContainer
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
	bool isInterestedInFileDrag(const StringArray &files) override;
	void filesDropped(const StringArray &files, int x, int y) override;

	WaveformComponent m_wavecomponent;
private:
    PaulstretchpluginAudioProcessor& processor;
	std::vector<std::shared_ptr<ParameterComponent>> m_parcomps;
	//SpectralVisualizer m_specvis;
	
	TextButton m_import_button;
	TextButton m_settings_button;
	Label m_info_label;
	SpectralChainEditor m_spec_order_ed;
	void chooseFile();
	void showSettingsMenu();
    String m_last_err;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PaulstretchpluginAudioProcessorEditor)
};

