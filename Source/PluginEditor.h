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
#include "envelope_component.h"

class zoom_scrollbar : public Component
{
public:
	enum hot_area
	{
		ha_none,
		ha_left_edge,
		ha_right_edge,
		ha_handle
	};
	void mouseDown(const MouseEvent& e) override;
	void mouseMove(const MouseEvent& e) override;
	void mouseDrag(const MouseEvent& e) override;
	void mouseEnter(const MouseEvent &event) override;
	void mouseExit(const MouseEvent &event) override;
	void paint(Graphics &g) override;
	std::function<void(Range<double>)> RangeChanged;
	Range<double> get_range() const { return m_therange; }
	void setRange(Range<double> rng, bool docallback);
private:
	Range<double> m_therange{ 0.0,1.0 };

	hot_area m_hot_area = ha_none;
	hot_area get_hot_area(int x, int y);
	int m_drag_start_x = 0;
};


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
	void setHighLighted(bool b);
	int m_group_id = -1;
private:
	Label m_label;
	AudioProcessorParameter* m_par = nullptr;
	std::unique_ptr<MySlider> m_slider;
	std::unique_ptr<ComboBox> m_combobox;
	std::unique_ptr<ToggleButton> m_togglebut;
	bool m_notify_only_on_release = false;
	bool m_dragging = false;
	Colour m_labeldefcolor;
};

class PerfMeterComponent : public Component, public Timer
{
public:
	PerfMeterComponent(PaulstretchpluginAudioProcessor* p);
	void paint(Graphics& g) override;
	void mouseDown(const MouseEvent& ev) override;
	void timerCallback() override;
	PaulstretchpluginAudioProcessor* m_proc = nullptr;
private:
    ColourGradient m_gradient;
};

class MyThumbCache : public AudioThumbnailCache
{
public:
	MyThumbCache() : AudioThumbnailCache(200) { /*Logger::writeToLog("Constructed AudioThumbNailCache");*/ }
	~MyThumbCache() { /*Logger::writeToLog("Destructed AudioThumbNailCache");*/ }
};

class WaveformComponent : public Component, public ChangeListener, public Timer
{
public:
	WaveformComponent(AudioFormatManager* afm, AudioThumbnail* thumb);
	~WaveformComponent();
	void changeListenerCallback(ChangeBroadcaster* cb) override;
	void paint(Graphics& g) override;
	void timerCallback() override;
	std::function<double()> CursorPosCallback;
	std::function<void(double)> SeekCallback;
	std::function<void(Range<double>, int)> TimeSelectionChangedCallback;
	std::function<File()> GetFileCallback;
	std::function<void(Range<double>)> ViewRangeChangedCallback;
	void mouseDown(const MouseEvent& e) override;
	void mouseUp(const MouseEvent& e) override;
	void mouseDrag(const MouseEvent& e) override;
	void mouseMove(const MouseEvent& e) override;
	void mouseDoubleClick(const MouseEvent& e) override;
	void mouseWheelMove(const MouseEvent& e, const MouseWheelDetails& wd) override;
	Range<double> getTimeSelection();
	void setTimeSelection(Range<double> rng);
	void setFileCachedRange(std::pair<Range<double>, Range<double>> rng);
	void setTimerEnabled(bool b);
	void setViewRange(Range<double> rng);
	Value ShowFileCacheRange;
	void setRecordingPosition(double pos) { m_rec_pos = pos; }
	int m_image_init_count = 0;
	int m_image_update_count = 0;
private:
	AudioThumbnail* m_thumbnail = nullptr;
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
	bool m_image_dirty = false;
	Image m_waveimage;
#ifdef JUCE_MODULE_AVAILABLE_juce_opengl
	OpenGLContext m_ogl;
	bool m_use_opengl = false;
#endif
	double m_rec_pos = 0.0;
	bool m_lock_timesel_set = false;
    bool m_using_audio_buffer = false;
	bool m_is_at_selection_drag_area = false;
	bool m_is_dragging_selection = false;
	void updateCachedImage();
	double viewXToNormalized(double xcor)
	{
		return jmap<double>(xcor, 0, getWidth(), m_view_range.getStart(), m_view_range.getEnd());
	}
	template<typename T>
	inline T normalizedToViewX(double norm)
	{
		return static_cast<T>(jmap<double>(norm, m_view_range.getStart(), m_view_range.getEnd(), 0, getWidth()));
	}
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
	std::function<void(int)> ModuleSelectedCallback;
	std::function<void(void)> ModuleOrderOrEnabledChangedCallback;
	const std::vector <SpectrumProcess>& getOrder() const { return m_order; }
	void setModuleSelected(int id);
	void moveModule(int old_id, int new_id);
private:
	StretchAudioSource * m_src = nullptr;
	bool m_did_drag = false;
	int m_cur_index = -1;
	int m_drag_x = 0;
    std::vector<SpectrumProcess> m_order;
	void drawBox(Graphics& g, int index, int x, int y, int w, int h);
};

class RatioMixerEditor : public Component, public Timer
{
public:
	RatioMixerEditor(int numratios);
	void resized() override;
	std::function<void(int, double)> OnRatioChanged;
	std::function<void(int, double)> OnRatioLevelChanged;
	std::function<double(int which, int index)> GetParameterValue;
	void timerCallback() override;
	void paint(Graphics& g) override;
private:
	uptrvec<Slider> m_ratio_sliders;
	uptrvec<Slider> m_ratio_level_sliders;
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
	
	
	bool isInterestedInFileDrag(const StringArray &files) override;
	void filesDropped(const StringArray &files, int x, int y) override;

	WaveformComponent m_wavecomponent;
	void chooseFile();
	void showRenderDialog();
private:
    PaulstretchpluginAudioProcessor& processor;
	uptrvec<ParameterComponent> m_parcomps;
	//SpectralVisualizer m_specvis;
	PerfMeterComponent m_perfmeter;
	TextButton m_import_button;
	TextButton m_settings_button;
	TextButton m_render_button;
	TextButton m_rewind_button;
	Label m_info_label;
	SpectralChainEditor m_spec_order_ed;
	
	void showSettingsMenu();
    String m_last_err;
	zoom_scrollbar m_zs;
	RatioMixerEditor m_ratiomixeditor{ 8 };
	EnvelopeComponent m_free_filter_component;
	TabbedComponent m_wavefilter_tab;
	Component* m_wave_container=nullptr;
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PaulstretchpluginAudioProcessorEditor)
};

