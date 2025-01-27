// SPDX-License-Identifier: GPLv3-or-later WITH Appstore-exception
// Copyright (C) 2017 Xenakios
// Copyright (C) 2020 Jesse Chappell

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "PluginProcessor.h"
#include <memory>
#include <vector>
#include <map>
#include "envelope_component.h"
#include "CustomLookAndFeel.h"

class OptionsView;

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
    void mouseDoubleClick (const MouseEvent&) override;
    void mouseWheelMove (const MouseEvent& e, const MouseWheelDetails& wd) override;

	void paint(Graphics &g) override;
	std::function<void(Range<double>)> RangeChanged;
	Range<double> get_range() const { return m_therange; }
	void setRange(Range<double> rng, bool docallback);
private:
	Range<double> m_therange{ 0.0,1.0 };

	hot_area m_hot_area = ha_none;
	hot_area get_hot_area(int x, int y);
	int m_drag_start_x = 0;
    int m_handle_off_x = 0;
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
public Slider::Listener, public Button::Listener, public ComboBox::Listener
{
public:
	ParameterComponent(AudioProcessorParameter* par, bool notifyOnlyOnRelease, bool useDrawableToggle=false);
	void resized() override;
	void sliderValueChanged(Slider* slid) override;
	void sliderDragStarted(Slider* slid) override;
	void sliderDragEnded(Slider* slid) override;
	void buttonClicked(Button* but) override;
    void comboBoxChanged (ComboBox* comboBoxThatHasChanged);
	void updateComponent();
	void setHighLighted(bool b);
	int m_group_id = -1;
	Slider* getSlider() const { return m_slider.get(); }

    DrawableButton* getDrawableButton() const { return m_drawtogglebut.get(); }
    ToggleButton* getToggleButton() const { return m_togglebut.get(); }
    ComboBox* getComboBox() const { return m_combobox.get(); }

private:
	Label m_label;
	AudioProcessorParameter* m_par = nullptr;
	std::unique_ptr<MySlider> m_slider;
	std::unique_ptr<ComboBox> m_combobox;
	std::unique_ptr<ToggleButton> m_togglebut;
    std::unique_ptr<DrawableButton> m_drawtogglebut;
	bool m_notify_only_on_release = false;
	bool m_dragging = false;
	Colour m_labeldefcolor;
};

class ParameterGroupComponent : public Component
{
public:
    ParameterGroupComponent(const String & name, int groupid, PaulstretchpluginAudioProcessor* proc, bool showtoggle=true);

    void resized() override;
    void paint(Graphics &g) override;

    //void addParameterComponent(std::unique_ptr<ParameterComponent> pcomp);
    void addParameterComponent(ParameterComponent * pcomp);
    void replaceParameterComponent(ParameterComponent * oldcomp, ParameterComponent * newcomp);

    void updateParameterComponents();

    void setBackgroundColor(Colour col) { m_bgcolor = col; }
    Colour getBackgroundColor() const { return m_bgcolor; }

    void setSelectedBackgroundColor(Colour col) { m_selbgcolor = col; }
    Colour getSelectedBBackgroundColor() const { return m_selbgcolor; }

    void setToggleEnabled(bool flag){ if (m_enableButton)  m_enableButton->setToggleState(flag, dontSendNotification); }
    bool getToggleEnabled() const { if (m_enableButton) return m_enableButton->getToggleState(); return false; }

    String name;
    int groupId = -1;
    bool allowDisableFade = true;

    int getMinimumHeight(int forWidth);

    std::function<void(void)> EnabledChangedCallback;

private:
    int doLayout(juce::Rectangle<int> bounds); // returns min height

    //uptrvec<ParameterComponent> m_parcomps;
    std::vector<ParameterComponent*> m_parcomps;
    std::unique_ptr<Label> m_namelabel;
    std::unique_ptr<DrawableButton> m_enableButton;
    //std::unique_ptr<ToggleButton> m_enableButton;

    CriticalSection* m_cs = nullptr;
    PaulstretchpluginAudioProcessor* m_proc = nullptr;
    int m_slidwidth = 400;

    Colour m_bgcolor;
    Colour m_selbgcolor;

    int m_minHeight = 0;
    int m_lastForWidth = -1;
    int m_lastCompSize = 0;
};


class PerfMeterComponent : public Component, public Timer
{
public:
	PerfMeterComponent(PaulstretchpluginAudioProcessor* p);
	void paint(Graphics& g) override;
	void mouseDown(const MouseEvent& ev) override;
	void timerCallback() override;
	PaulstretchpluginAudioProcessor* m_proc = nullptr;

    bool enabled = true;
private:
    ColourGradient m_gradient;
};

class MyThumbCache : public AudioThumbnailCache
{
public:
	MyThumbCache() : AudioThumbnailCache(200) 
	{ 
		// The default priority of 2 is a bit too low in some cases, it seems...
		//TODO: It's unclear if the thread inside the ThumbCache can have its priority manipulated in JUCE8...
		//getTimeSliceThread().setPriority(Thread::Priority::high);
	}
	~MyThumbCache() {}
};

class WaveformComponent : public Component, public ChangeListener, public Timer
{
public:
	WaveformComponent(AudioFormatManager* afm, AudioThumbnail* thumb, StretchAudioSource* sas);
	~WaveformComponent();
	void changeListenerCallback(ChangeBroadcaster* cb) override;
	void paint(Graphics& g) override;
	void timerCallback() override;
	std::function<double()> CursorPosCallback;
	std::function<void(double)> SeekCallback;
	std::function<void(Range<double>, int)> TimeSelectionChangedCallback;
	std::function<URL()> GetFileCallback;
	std::function<void(Range<double>)> ViewRangeChangedCallback;
	void mouseDown(const MouseEvent& e) override;
	void mouseUp(const MouseEvent& e) override;
	void mouseDrag(const MouseEvent& e) override;
	void mouseMove(const MouseEvent& e) override;
	void mouseDoubleClick(const MouseEvent& e) override;
	void mouseWheelMove(const MouseEvent& e, const MouseWheelDetails& wd) override;
    void mouseMagnify (const MouseEvent& event, float scaleFactor) override;
	void setAudioInfo(double sr, double seekpos, int fftsize);
	Range<double> getTimeSelection();
	void setTimeSelection(Range<double> rng);
	void setFileCachedRange(std::pair<Range<double>, Range<double>> rng);
	void setTimerEnabled(bool b);
	void setViewRange(Range<double> rng);
	String m_infotext;
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
    bool m_timedrag_started = false;
	int getTimeSelectionEdge(int x, int y);
	std::pair<Range<double>, Range<double>> m_file_cached;
	bool m_image_dirty = false;
	double m_sr = 0.0;
	int m_fft_size = 0;
	double m_last_startpos = 0.0;
	int64_t m_last_source_pos = -1;
	double m_last_source_pos_update_time = 0.0;
	Image m_waveimage;
	StretchAudioSource* m_sas = nullptr;
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
    SpectralChainEditor();
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
    int m_downoffset_x = 0;
    std::vector<SpectrumProcess> m_order;
    std::unique_ptr<Drawable> m_enabledImage;
    std::unique_ptr<Drawable> m_disabledImage;
    Colour m_bgcolor;
    Colour m_selbgcolor;
    Colour m_dragbgcolor;

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
    void setSlidersSnap(bool flag);
private:
	uptrvec<Slider> m_ratio_sliders;
	uptrvec<Slider> m_ratio_level_sliders;
    uptrvec<Label> m_labels;
};

class FreeFilterComponent : public Component
{
public:
	FreeFilterComponent(PaulstretchpluginAudioProcessor* proc);
	void resized() override;
	EnvelopeComponent* getEnvelopeComponent() { return &m_env; }
	void paint(Graphics &g) override;
	void updateParameterComponents();
    void setSlidersSnap(bool flag);
private:
	EnvelopeComponent m_env;
	uptrvec<ParameterComponent> m_parcomps;
    std::unique_ptr<Viewport> m_viewport;
    Component m_container;
    CriticalSection* m_cs = nullptr;
	PaulstretchpluginAudioProcessor* m_proc = nullptr;
	int m_slidwidth = 350;
};

class MyTabComponent : public TabbedComponent
{
public:
	MyTabComponent(int& curtab) : TabbedComponent(TabbedButtonBar::TabsAtTop), m_cur_tab(curtab) {}
	void currentTabChanged(int newCurrentTabIndex, const String&) override
	{
		//m_cur_tab = newCurrentTabIndex;
		
	}
private:
	int& m_cur_tab;
	
};

class SimpleFFTComponent : public Component,
	private Timer
{
public:
	SimpleFFTComponent() :

		forwardFFT(fftOrder),
		spectrogramImage(Image::RGB, 512, 512, true)
	{
		setOpaque(true);
		startTimerHz(60);
		
	}

	~SimpleFFTComponent()
	{
		
	}

	void addAudioBlock(const AudioBuffer<float>& bufferToFill) 
	{
		if (bufferToFill.getNumChannels() > 0)
		{
			const auto* channelData = bufferToFill.getReadPointer(0);

			for (auto i = 0; i < bufferToFill.getNumSamples(); ++i)
				pushNextSampleIntoFifo(channelData[i]);
		}
	}

	void resized() override
	{
		spectrogramImage = Image(Image::RGB, getWidth(), getHeight(), true);
	}

	void paint(Graphics& g) override
	{
		g.fillAll(Colours::black);

		g.setOpacity(1.0f);
		g.drawImage(spectrogramImage, getLocalBounds().toFloat());
	}

	void timerCallback() override
	{
		if (nextFFTBlockReady)
		{
			drawNextLineOfSpectrogram();
			nextFFTBlockReady = false;
			repaint();
		}
	}

	void pushNextSampleIntoFifo(float sample) noexcept
	{
		// if the fifo contains enough data, set a flag to say
		// that the next line should now be rendered..
		if (fifoIndex == fftSize)
		{
			if (!nextFFTBlockReady)
			{
				zeromem(fftData, sizeof(fftData));
				memcpy(fftData, fifo, sizeof(fifo));
				nextFFTBlockReady = true;
			}

			fifoIndex = 0;
		}

		fifo[fifoIndex++] = sample;
	}

	void drawNextLineOfSpectrogram()
	{
		auto rightHandEdge = spectrogramImage.getWidth() - 1;
		auto imageHeight = spectrogramImage.getHeight();

		// first, shuffle our image leftwards by 1 pixel..
		spectrogramImage.moveImageSection(0, 0, 1, 0, rightHandEdge, imageHeight);

		// then render our FFT data..
		forwardFFT.performFrequencyOnlyForwardTransform(fftData);

		// find the range of values produced, so we can scale our rendering to
		// show up the detail clearly
		auto maxLevel = FloatVectorOperations::findMinAndMax(fftData, fftSize / 2);

		for (auto y = 1; y < imageHeight; ++y)
		{
			auto skewedProportionY = 1.0f - std::exp(std::log(y / (float)imageHeight) * 0.2f);
			auto fftDataIndex = jlimit(0, fftSize / 2, (int)(skewedProportionY * fftSize / 2));
			auto level = jmap(fftData[fftDataIndex], 0.0f, jmax(maxLevel.getEnd(), 1e-5f), 0.0f, 1.0f);

			spectrogramImage.setPixelAt(rightHandEdge, y, Colour::fromHSV(level, 1.0f, level, 1.0f));
		}
	}

	enum
	{
		fftOrder = 10,
		fftSize = 1 << fftOrder
	};

private:
	dsp::FFT forwardFFT;
	Image spectrogramImage;

	float fifo[fftSize];
	float fftData[8 * fftSize];
	int fifoIndex = 0;
	bool nextFFTBlockReady = false;

	
};


class AudioFilePreviewComponent : public FilePreviewComponent
{
public:
	AudioFilePreviewComponent(PaulstretchpluginAudioProcessor* p) : m_proc(p)
	{
		addAndMakeVisible(m_playbut);
		m_playbut.setButtonText("Play");
		m_playbut.onClick = [this]()
		{

		};
		setSize(100, 30);
	}
	void selectedFileChanged(const File &newSelectedFile) override
	{
		ScopedLock locker(m_proc->getCriticalSection());
		m_reader = unique_from_raw(m_proc->m_afm->createReaderFor(newSelectedFile));
		m_playpos = 0;
	}
	void resized() override
	{
		m_playbut.setBounds(0, 0, getWidth(), getHeight());
	}
	void togglePlay()
	{

	}
	void processBlock(double sr, AudioBuffer<float>& buf);
private:
	TextButton m_playbut;
	PaulstretchpluginAudioProcessor* m_proc = nullptr;
	std::unique_ptr<AudioFormatReader> m_reader;
	int64 m_playpos = 0;
};

class MyFileBrowserComponent : public Component, public FileBrowserListener
{
public:
	MyFileBrowserComponent(PaulstretchpluginAudioProcessor& p);
	~MyFileBrowserComponent();
	void resized() override;
	void paint(Graphics& g) override;
	std::function<void(int, File)> OnAction;
	void selectionChanged() override;

	/** Callback when the user clicks on a file in the browser. */
	void fileClicked(const File& file, const MouseEvent& e) override;

	/** Callback when the user double-clicks on a file in the browser. */
	void fileDoubleClicked(const File& file) override;

	/** Callback when the browser's root folder changes. */
	void browserRootChanged(const File& newRoot) override;
private:
	std::unique_ptr<FileBrowserComponent> m_fbcomp;
	WildcardFileFilter m_filefilter;
	PaulstretchpluginAudioProcessor& m_proc;
	LookAndFeel_V3 m_filebwlookandfeel;
};

class PaulstretchpluginAudioProcessorEditor  : public AudioProcessorEditor, 
	public MultiTimer, public FileDragAndDropTarget, public DragAndDropContainer, public ComponentListener

{
public:
    PaulstretchpluginAudioProcessorEditor (PaulstretchpluginAudioProcessor&);
    ~PaulstretchpluginAudioProcessorEditor();
	/*
	void parentHierarchyChanged() override {
		if (JUCEApplicationBase::isStandaloneApp()) {
			auto* comp = getTopLevelComponent();

			if (auto window = dynamic_cast<DocumentWindow*>(comp)) {
				window->setUsingNativeTitleBar(true);
				window->setResizable(true, false);
				window->setTitleBarButtonsRequired(DocumentWindow::TitleBarButtons::allButtons, false);
				window->setSize(800, 600);
			}
		}
	}
	*/
	void paint (Graphics&) override;
    void resized() override;
	void timerCallback(int id) override;
	
	bool isInterestedInFileDrag(const StringArray &files) override;
	void filesDropped(const StringArray &files, int x, int y) override;

	bool keyPressed(const KeyPress& press) override;

    void componentParentHierarchyChanged (Component& component) override;

    
	WaveformComponent m_wavecomponent;
	
	void showRenderDialog();
	//SimpleFFTComponent m_sonogram;
	String m_last_err;

    void urlOpened(const URL& url);

    std::function<AudioDeviceManager*()> getAudioDeviceManager;
    std::function<void(Component*,Component*)> showAudioSettingsDialog;

private:

    bool isSpectrumProcGroupEnabled(int groupid);
    void setSpectrumProcGroupEnabled(int groupid, bool enabled);

    void updateAllSliders();

    void toggleOutputRecording();

    void showPopTip(const String & message, int timeoutMs, Component * target, int maxwidth=100);

    
    CustomLookAndFeel m_lookandfeel;

	PaulstretchpluginAudioProcessor& processor;
	uptrvec<ParameterComponent> m_parcomps;
    std::map<int, std::unique_ptr<ParameterGroupComponent> > m_pargroups;
    std::unique_ptr<ParameterGroupComponent> m_posgroup;
    std::unique_ptr<ParameterGroupComponent> m_stretchgroup;
    std::unique_ptr<ParameterGroupComponent> m_binauralgroup;

    std::unique_ptr<Viewport> m_groupviewport;
    std::unique_ptr<Component> m_groupcontainer;


	//SpectralVisualizer m_specvis;
	PerfMeterComponent m_perfmeter;
	TextButton m_import_button;
	TextButton m_settings_button;
	TextButton m_render_button;
    std::unique_ptr<DrawableButton> m_rewind_button;
	Label m_info_label;
	SpectralChainEditor m_spec_order_ed;
    double m_lastspec_select_time = 0.0;
    int m_lastspec_select_group = -1;
    bool m_shortMode = false;
    File m_lastRecordedFile;
    std::unique_ptr<Label> m_fileRecordingLabel;
    std::unique_ptr<DrawableButton> m_recordingButton;

    bool settingsWasShownOnDown = false;
    uint32 settingsClosedTimestamp = 0;
    WeakReference<Component> settingsCalloutBox;
    std::unique_ptr<OptionsView> m_optionsView;

    
	zoom_scrollbar m_zs;
	RatioMixerEditor m_ratiomixeditor{ 8 };
	FreeFilterComponent m_free_filter_component;
	
	MyTabComponent m_wavefilter_tab;
	Component* m_wave_container=nullptr;
    void showAudioSetup();
    void showSettings(bool flag);
	void toggleFileBrowser();
	std::vector<int> m_capturelens{ 2,5,10,30,60,120 };
	
	std::unique_ptr<MyFileBrowserComponent> m_filechooser;
    std::unique_ptr<FileChooser> fileChooser;
	WildcardFileFilter m_filefilter;

    class CustomTooltipWindow : public TooltipWindow
    {
    public:
        CustomTooltipWindow(PaulstretchpluginAudioProcessorEditor * parent_, Component * viewparent) : TooltipWindow(viewparent), parent(parent_) {}
        virtual ~CustomTooltipWindow() {
            if (parent) {
                // reset our smart pointer without a delete! someone else is deleting it
                parent->tooltipWindow.release();
            }
        }

        /*
        String getTipFor (Component& c) override
        {
            if (parent->popTip && parent->popTip->isShowing()) {
                return {};
            }
            return TooltipWindow::getTipFor(c);
        }
         */

        PaulstretchpluginAudioProcessorEditor * parent;
    };

    std::unique_ptr<CustomTooltipWindow> tooltipWindow;

    // keep this down here, so it gets destroyed early
    std::unique_ptr<BubbleMessageComponent> popTip;

    
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PaulstretchpluginAudioProcessorEditor)
};

