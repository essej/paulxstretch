/*

Copyright (C) 2017 Xenakios

This program is free software; you can redistribute it and/or modify
it under the terms of version 3 of the GNU General Public License
as published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License (version 3) for more details.

www.gnu.org/licenses

*/


#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <array>
#include "RenderSettingsComponent.h"

#include "CrossPlatformUtils.h"

static void handleSettingsMenuModalCallback(int choice, PaulstretchpluginAudioProcessorEditor* ed)
{
	ed->executeModalMenuAction(0,choice);
}

enum ParameterGroupIds
{
    HarmonicsGroup = 0,
    TonalNoiseGroup = 1,
    FrequencyShiftGroup = 2,
    PitchShiftGroup = 3,
    RatiosGroup = 4,
    FrequencySpreadGroup = 5,
    FilterGroup = 6,
    FreeFilterGroup = 7,
    CompressGroup = 8
};

//==============================================================================
PaulstretchpluginAudioProcessorEditor::PaulstretchpluginAudioProcessorEditor(PaulstretchpluginAudioProcessor& p)
	: AudioProcessorEditor(&p),
	m_wavecomponent(p.m_afm,p.m_thumb.get(), p.getStretchSource()),
	processor(p), m_perfmeter(&p),
    m_free_filter_component(&p),
    m_wavefilter_tab(p.m_cur_tab_index),
	m_filefilter(p.m_afm->getWildcardForAllFormats(),String(),String())
{
    LookAndFeel::setDefaultLookAndFeel(&m_lookandfeel);
    setLookAndFeel(&m_lookandfeel);
	
	
	setWantsKeyboardFocus(true);
	m_wave_container = new Component;
    m_free_filter_component.getEnvelopeComponent()->set_envelope(processor.m_free_filter_envelope);
	m_free_filter_component.getEnvelopeComponent()->XFromNormalized = [this](double x) 
	{ 
		//return jmap<double>(pow(x, 3.0), 0.0, 1.0, 30.0, processor.getSampleRateChecked()/2.0);
		return 30.0*pow(1.05946309436, x*115.0);
	};
	m_free_filter_component.getEnvelopeComponent()->YFromNormalized = [this](double x)
	{
		return jmap<double>(x, 0.0, 1.0, -48.0, 12.0);
	};
    int tabdepth = 24;

#if JUCE_IOS
    tabdepth = 36;
#endif
    m_wavefilter_tab.setTabBarDepth(tabdepth);
    m_wavefilter_tab.getTabbedButtonBar().setMinimumTabScaleFactor(0.25f);

    addAndMakeVisible(&m_perfmeter);
	
	addAndMakeVisible(&m_import_button);
#if JUCE_IOS
	m_import_button.setButtonText("Load Audio...");
#else
    m_import_button.setButtonText("Show browser");
#endif
	m_import_button.onClick = [this]()
	{ 
		toggleFileBrowser();
	};
	
	addAndMakeVisible(&m_settings_button);
	m_settings_button.setButtonText("Settings...");
	m_settings_button.onClick = [this]() { showSettingsMenu(); };
	
	if (JUCEApplicationBase::isStandaloneApp())
	{
		addAndMakeVisible(&m_render_button);
		m_render_button.setButtonText("Render...");
		m_render_button.onClick = [this]() { showRenderDialog(); };
	}

    // jlc
    m_rewind_button = std::make_unique<DrawableButton>("rewind", DrawableButton::ButtonStyle::ImageFitted);
    std::unique_ptr<Drawable> rewimg(Drawable::createFromImageData(BinaryData::skipback_icon_svg, BinaryData::skipback_icon_svgSize));
    m_rewind_button->setImages(rewimg.get());
    m_rewind_button->setColour(TextButton::buttonColourId, Colours::transparentBlack);
    m_rewind_button->setColour(TextButton::buttonOnColourId, Colours::transparentBlack);
    m_rewind_button->setColour(DrawableButton::backgroundColourId, Colours::transparentBlack);
    m_rewind_button->setColour(DrawableButton::backgroundOnColourId, Colours::transparentBlack);

	addAndMakeVisible(m_rewind_button.get());
	m_rewind_button->setTitle("Return to start");
    m_rewind_button->setTooltip("Return to start");
	m_rewind_button->onClick = [this]()
	{
		*processor.getBoolParameter(cpi_rewind) = true;
		//processor.getStretchSource()->seekPercent(processor.getStretchSource()->getPlayRange().getStart());
	};

	addAndMakeVisible(&m_info_label);
	m_info_label.setJustificationType(Justification::centredRight);
    m_info_label.setFont(14.0f);

	m_wavecomponent.GetFileCallback = [this]() { return processor.getAudioFile(); };
	
    const auto& pars = processor.getParameters();
	for (int i=0;i<pars.size();++i)
	{
		AudioProcessorParameterWithID* parid = dynamic_cast<AudioProcessorParameterWithID*>(pars[i]);
		jassert(parid);
		bool notifyonlyonrelease = false;
		if (parid->paramID.startsWith("fftsize") || parid->paramID.startsWith("numoutchans") 
			|| parid->paramID.startsWith("numinchans"))
				notifyonlyonrelease = true;

        bool usedrawable = false;
        if (i == cpi_pause_enabled || i == cpi_looping_enabled || i == cpi_capture_trigger || i == cpi_freeze || i == cpi_passthrough)
            usedrawable = true;


        int group_id = -1;
		if (i == cpi_harmonicsbw || i == cpi_harmonicsfreq || i == cpi_harmonicsgauss || i == cpi_numharmonics)
			group_id = HarmonicsGroup;
		if (i == cpi_octavesm2 || i == cpi_octavesm1 || i == cpi_octaves0 || i == cpi_octaves1 || i == cpi_octaves15 ||
			i == cpi_octaves2 || i==cpi_octaves_extra1 || i==cpi_octaves_extra2)
			group_id = -2; // -2 for not included in the main parameters page
		if (i >= cpi_octaves_ratio0 && i <= cpi_octaves_ratio7)
			group_id = -2;
		if ((i >= cpi_enable_spec_module0 && i <= cpi_enable_spec_module8))
			group_id = -2;
		if (i == cpi_tonalvsnoisebw || i == cpi_tonalvsnoisepreserve)
			group_id = TonalNoiseGroup;
		if (i == cpi_filter_low || i == cpi_filter_high)
			group_id = FilterGroup;
		if (i == cpi_compress)
			group_id = CompressGroup;
		if (i == cpi_spreadamount)
			group_id = FrequencySpreadGroup;
		if (i == cpi_frequencyshift)
			group_id = FrequencyShiftGroup;
		if (i == cpi_pitchshift)
			group_id = PitchShiftGroup;
		if (i == cpi_freefilter_scaley || i == cpi_freefilter_shiftx || i == cpi_freefilter_shifty ||
			i == cpi_freefilter_tilty || i == cpi_freefilter_randomy_amount || i == cpi_freefilter_randomy_numbands
			|| i == cpi_freefilter_randomy_rate)
			group_id = -2;
		if (group_id >= -1)
		{
			m_parcomps.emplace_back(std::make_unique<ParameterComponent>(pars[i], notifyonlyonrelease, usedrawable));
			m_parcomps.back()->m_group_id = group_id;

            if (group_id == -1) // only add ones that aren't in groups
                addAndMakeVisible(m_parcomps.back().get());
		}
		else
		{
			m_parcomps.push_back(nullptr);
		}
	}


    m_parcomps[cpi_num_inchans]->getSlider()->setSliderStyle(Slider::SliderStyle::IncDecButtons);
    m_parcomps[cpi_num_inchans]->getSlider()->setTextBoxStyle(Slider::TextEntryBoxPosition::TextBoxLeft, false, 30, 34);
    m_parcomps[cpi_num_outchans]->getSlider()->setSliderStyle(Slider::SliderStyle::IncDecButtons);
    m_parcomps[cpi_num_outchans]->getSlider()->setTextBoxStyle(Slider::TextEntryBoxPosition::TextBoxLeft, false, 30, 34);

    removeChildComponent(m_parcomps[cpi_bypass_stretch].get());


    if (auto * pausebut = m_parcomps[cpi_pause_enabled]->getDrawableButton()) {
        std::unique_ptr<Drawable> playimg(Drawable::createFromImageData(BinaryData::play_icon_svg, BinaryData::play_icon_svgSize));
        std::unique_ptr<Drawable> pauseimg(Drawable::createFromImageData(BinaryData::pause_icon_svg, BinaryData::pause_icon_svgSize));
        pausebut->setImages(pauseimg.get(), nullptr, nullptr, nullptr, playimg.get());
        pausebut->setColour(DrawableButton::backgroundColourId, Colour::fromFloatRGBA(0.1f, 0.5f, 0.1f, 0.55f));
    }

    if (auto * loopbut = m_parcomps[cpi_looping_enabled]->getDrawableButton()) {
        std::unique_ptr<Drawable> loopimg(Drawable::createFromImageData(BinaryData::loop_icon_svg, BinaryData::loop_icon_svgSize));
        loopbut->setImages(loopimg.get());
        loopbut->setColour(DrawableButton::backgroundOnColourId, Colour::fromFloatRGBA(0.6, 0.4, 0.6, 0.4));
    }

    if (auto * recbut = m_parcomps[cpi_capture_trigger]->getDrawableButton()) {
        std::unique_ptr<Drawable> reconimg(Drawable::createFromImageData(BinaryData::record_active_svg, BinaryData::record_active_svgSize));
        std::unique_ptr<Drawable> recoffimg(Drawable::createFromImageData(BinaryData::record_svg, BinaryData::record_svgSize));
        recbut->setImages(recoffimg.get(), nullptr, nullptr, nullptr, reconimg.get());
        recbut->setColour(DrawableButton::backgroundOnColourId, Colour::fromFloatRGBA(0.6, 0.3, 0.3, 0.5));
    }

    if (auto * freezebut = m_parcomps[cpi_freeze]->getDrawableButton()) {
        std::unique_ptr<Drawable> img(Drawable::createFromImageData(BinaryData::freeze_svg, BinaryData::freeze_svgSize));
        freezebut->setImages(img.get());
        freezebut->setColour(DrawableButton::backgroundOnColourId, Colour::fromFloatRGBA(0.2, 0.5, 0.7, 0.55));
    }

    if (auto * thrubut = m_parcomps[cpi_passthrough]->getDrawableButton()) {
        std::unique_ptr<Drawable> img(Drawable::createFromImageData(BinaryData::passthru_svg, BinaryData::passthru_svgSize));
        std::unique_ptr<Drawable> imgon(Drawable::createFromImageData(BinaryData::passthru_enabled_svg, BinaryData::passthru_enabled_svgSize));
        thrubut->setImages(img.get(), nullptr, nullptr, nullptr, imgon.get());
        thrubut->setColour(DrawableButton::backgroundOnColourId, Colour::fromFloatRGBA(0.5, 0.3, 0.0, 0.55));
    }


#if JUCE_IOS
    // just don't include chan counts on ios for now
    removeChildComponent(m_parcomps[cpi_num_inchans].get());
    removeChildComponent(m_parcomps[cpi_num_outchans].get());
#endif


    m_groupviewport = std::make_unique<Viewport>();
    m_groupcontainer = std::make_unique<Component>();
    m_groupviewport->setViewedComponent(m_groupcontainer.get(), false);

    addAndMakeVisible(m_groupviewport.get());

    m_stretchgroup = std::make_unique<ParameterGroupComponent>("", -1, &processor, true);
    m_stretchgroup->setBackgroundColor(Colour(0xff332244));
    m_stretchgroup->setToggleEnabled( ! *processor.getBoolParameter(cpi_bypass_stretch));
    if (*processor.getBoolParameter(cpi_bypass_stretch)) {
        m_stretchgroup->addParameterComponent(m_parcomps[cpi_dryplayrate].get());
        removeChildComponent(m_parcomps[cpi_stretchamount].get());
    } else {
        m_stretchgroup->addParameterComponent(m_parcomps[cpi_stretchamount].get());
        removeChildComponent(m_parcomps[cpi_dryplayrate].get());
    }
    m_stretchgroup->addParameterComponent(m_parcomps[cpi_fftsize].get());
    m_stretchgroup->EnabledChangedCallback = [this]() {
        toggleBool(processor.getBoolParameter(cpi_bypass_stretch));
        m_stretchgroup->setToggleEnabled( ! *processor.getBoolParameter(cpi_bypass_stretch));
        // jlc
    };

    addAndMakeVisible(m_stretchgroup.get());


    m_posgroup = std::make_unique<ParameterGroupComponent>("", -1, &processor, false);
    m_posgroup->addParameterComponent(m_parcomps[cpi_loopxfadelen].get());
    m_posgroup->addParameterComponent(m_parcomps[cpi_onsetdetection].get());
    m_posgroup->addParameterComponent(m_parcomps[cpi_soundstart].get());
    m_posgroup->addParameterComponent(m_parcomps[cpi_soundend].get());

    m_groupcontainer->addAndMakeVisible(m_posgroup.get());


    auto harmgroup = std::make_unique<ParameterGroupComponent>("", HarmonicsGroup, &processor);
    harmgroup->addParameterComponent(m_parcomps[cpi_numharmonics].get());
    harmgroup->addParameterComponent(m_parcomps[cpi_harmonicsfreq].get());
    harmgroup->addParameterComponent(m_parcomps[cpi_harmonicsbw].get());
    harmgroup->addParameterComponent(m_parcomps[cpi_harmonicsgauss].get());
    harmgroup->EnabledChangedCallback = [this]() {
        processor.setDirty();
    };

    m_groupcontainer->addAndMakeVisible(harmgroup.get());
    m_pargroups.insert( {HarmonicsGroup, std::move(harmgroup) });

    auto tonegroup = std::make_unique<ParameterGroupComponent>("", TonalNoiseGroup, &processor);
    tonegroup->addParameterComponent(m_parcomps[cpi_tonalvsnoisebw].get());
    tonegroup->addParameterComponent(m_parcomps[cpi_tonalvsnoisepreserve].get());
    tonegroup->EnabledChangedCallback = [this]() {
        processor.setDirty();
    };
    m_groupcontainer->addAndMakeVisible(tonegroup.get());
    m_pargroups.insert( {TonalNoiseGroup, std::move(tonegroup) });

    auto fsgroup = std::make_unique<ParameterGroupComponent>("", FrequencyShiftGroup, &processor);
    fsgroup->addParameterComponent(m_parcomps[cpi_frequencyshift].get());
    fsgroup->EnabledChangedCallback = [this]() {
        processor.setDirty();
    };
    m_groupcontainer->addAndMakeVisible(fsgroup.get());
    m_pargroups.insert( {FrequencyShiftGroup, std::move(fsgroup) });

    auto psgroup = std::make_unique<ParameterGroupComponent>("", PitchShiftGroup, &processor);
    psgroup->addParameterComponent(m_parcomps[cpi_pitchshift].get());
    psgroup->EnabledChangedCallback = [this]() {
        processor.setDirty();
    };
    m_groupcontainer->addAndMakeVisible(psgroup.get());
    m_pargroups.insert( {PitchShiftGroup, std::move(psgroup) });

    auto spreadgroup = std::make_unique<ParameterGroupComponent>("", FrequencySpreadGroup, &processor);
    spreadgroup->addParameterComponent(m_parcomps[cpi_spreadamount].get());
    spreadgroup->EnabledChangedCallback = [this]() {
        processor.setDirty();
    };
    m_groupcontainer->addAndMakeVisible(spreadgroup.get());
    m_pargroups.insert( {FrequencySpreadGroup, std::move(spreadgroup) });

    auto filtgroup = std::make_unique<ParameterGroupComponent>("", FilterGroup, &processor);
    filtgroup->addParameterComponent(m_parcomps[cpi_filter_low].get());
    filtgroup->addParameterComponent(m_parcomps[cpi_filter_high].get());
    filtgroup->EnabledChangedCallback = [this]() {
        processor.setDirty();
    };
    m_groupcontainer->addAndMakeVisible(filtgroup.get());
    m_pargroups.insert( {FilterGroup, std::move(filtgroup) });

    auto compgroup = std::make_unique<ParameterGroupComponent>("", CompressGroup, &processor);
    compgroup->addParameterComponent(m_parcomps[cpi_compress].get());
    compgroup->EnabledChangedCallback = [this]() {
        processor.setDirty();
    };
    m_groupcontainer->addAndMakeVisible(compgroup.get());
    m_pargroups.insert( {CompressGroup, std::move(compgroup) });


	//m_parcomps[cpi_dryplayrate]->getSlider()->setSkewFactorFromMidPoint(1.0);
	//addAndMakeVisible(&m_specvis);
	m_wave_container->addAndMakeVisible(&m_zs);
	m_zs.RangeChanged = [this](Range<double> r)
	{
		m_wavecomponent.setViewRange(r);
		processor.m_wave_view_range = r;
	};
	m_zs.setRange(processor.m_wave_view_range, true);
	
	m_wavecomponent.ViewRangeChangedCallback = [this](Range<double> rng)
	{
		m_zs.setRange(rng, false);
	};
	m_wavecomponent.TimeSelectionChangedCallback = [this](Range<double> range, int which)
	{
		*processor.getFloatParameter(cpi_soundstart) = range.getStart();
		*processor.getFloatParameter(cpi_soundend) = range.getEnd();
	};
	m_wavecomponent.CursorPosCallback = [this]()
	{
		return processor.getStretchSource()->getInfilePositionPercent();
	};
	m_wavecomponent.SeekCallback = [this](double pos)
	{
		if (processor.getStretchSource()->getPlayRange().contains(pos))
			processor.getStretchSource()->seekPercent(pos);
	};
	
	m_spec_order_ed.setSource(processor.getStretchSource());
	addAndMakeVisible(&m_spec_order_ed);


	m_spec_order_ed.ModuleSelectedCallback = [this](int id)
	{
        auto nowtime = Time::getMillisecondCounterHiRes() * 1e-3;

        if (m_lastspec_select_group == id && nowtime < m_lastspec_select_time + 0.5) {
            // double click toggles enabled
            setSpectrumProcGroupEnabled(id, !isSpectrumProcGroupEnabled(id));
        }
        m_lastspec_select_group = id;
        m_lastspec_select_time = nowtime;

        if (id == FreeFilterGroup) {
            if (isSpectrumProcGroupEnabled(id) && !m_shortMode) {
                m_wavefilter_tab.setCurrentTabIndex(2);
            }
        } else if (id == RatiosGroup) {
            if (isSpectrumProcGroupEnabled(id) && !m_shortMode) {
                m_wavefilter_tab.setCurrentTabIndex(1);
            }
        }

		for (int i = 0; i < m_parcomps.size(); ++i)
		{
			if (m_parcomps[i] != nullptr)
			{
                if (m_parcomps[i]->m_group_id == id)
					m_parcomps[i]->setHighLighted(true);
				else
					m_parcomps[i]->setHighLighted(false);
			}
		}
	};
	m_spec_order_ed.ModuleOrderOrEnabledChangedCallback = [this]()
	{
		processor.setDirty();
	};
	
	m_ratiomixeditor.GetParameterValue = [this](int which, int index)
	{
		if (which == 0)
			return (double)*processor.getFloatParameter((int)cpi_octaves_ratio0 + index);
		if (which == 1)
		{
			if (index == 0)
				return (double)*processor.getFloatParameter(cpi_octavesm2);
			if (index == 1)
				return (double)*processor.getFloatParameter(cpi_octavesm1);
			if (index == 2)
				return (double)*processor.getFloatParameter(cpi_octaves0);
			if (index == 3)
				return (double)*processor.getFloatParameter(cpi_octaves1);
			if (index == 4)
				return (double)*processor.getFloatParameter(cpi_octaves15);
			if (index == 5)
				return (double)*processor.getFloatParameter(cpi_octaves2);
			if (index == 6)
				return (double)*processor.getFloatParameter(cpi_octaves_extra1);
			if (index == 7)
				return (double)*processor.getFloatParameter(cpi_octaves_extra2);
		}
			
		return 0.0;
	};
	m_ratiomixeditor.OnRatioLevelChanged = [this](int index, double val)
	{
		if (index == 0)
			*processor.getFloatParameter(cpi_octavesm2) = val;
		if (index == 1)
			*processor.getFloatParameter(cpi_octavesm1) = val;
		if (index == 2)
			*processor.getFloatParameter(cpi_octaves0) = val;
		if (index == 3)
			*processor.getFloatParameter(cpi_octaves1) = val;
		if (index == 4)
			*processor.getFloatParameter(cpi_octaves15) = val;
		if (index == 5)
			*processor.getFloatParameter(cpi_octaves2) = val;
		if (index == 6)
			*processor.getFloatParameter(cpi_octaves_extra1) = val;
		if (index == 7)
			*processor.getFloatParameter(cpi_octaves_extra2) = val;
	};
	m_ratiomixeditor.OnRatioChanged = [this](int index, double val)
	{
		*processor.getFloatParameter((int)cpi_octaves_ratio0 + index) = val;
	};
	m_wave_container->addAndMakeVisible(&m_wavecomponent);

    auto tabbgcol = Colour(0xff555555);

	m_wavefilter_tab.addTab("Waveform", tabbgcol, m_wave_container, true);
	m_wavefilter_tab.addTab("Ratio mixer", tabbgcol, &m_ratiomixeditor, false);
	m_wavefilter_tab.addTab("Free filter", tabbgcol, &m_free_filter_component, false);
	//m_wavefilter_tab.addTab("Spectrum", Colours::white, &m_sonogram, false);

	addAndMakeVisible(&m_wavefilter_tab);

    auto defbounds = processor.getLastPluginBounds();

    setSize (defbounds.getWidth(), defbounds.getHeight());

    startTimer(1, 100);
	startTimer(2, 1000);
	startTimer(3, 200);
	m_wavecomponent.startTimer(100);


    setResizeLimits(320, 430, 40000, 4000);

    setResizable(true, !JUCEApplicationBase::isStandaloneApp());

#if JUCE_MAC
    disableAppNap();
#endif
}

PaulstretchpluginAudioProcessorEditor::~PaulstretchpluginAudioProcessorEditor()
{
    LookAndFeel::setDefaultLookAndFeel(nullptr);
    setLookAndFeel(nullptr);
	//Logger::writeToLog("PaulX Editor destroyed");
}

bool PaulstretchpluginAudioProcessorEditor::isSpectrumProcGroupEnabled(int groupid)
{
    auto order = processor.getStretchSource()->getSpectrumProcessOrder();
    for (int i=0; i < order.size(); ++i) {
        if (order[i].m_index == groupid) {
            return order[i].m_enabled->get();
        }
    }
    return false;
}

void PaulstretchpluginAudioProcessorEditor::setSpectrumProcGroupEnabled(int groupid, bool enabled)
{
    auto order = processor.getStretchSource()->getSpectrumProcessOrder();
    for (int i=0; i < order.size(); ++i) {
        if (order[i].m_index == groupid) {
            *(order[i].m_enabled) = enabled; //->setValueNotifyingHost(enabled ? 1.0f : 0.0f);
            return;
        }
    }
    return;
}



void PaulstretchpluginAudioProcessorEditor::showRenderDialog()
{
	auto contentraw =  new RenderSettingsComponent(&processor);

    int prefw = jmin(contentraw->getPreferredWidth(), getWidth() - 20);
    int prefh = jmin(contentraw->getPreferredHeight(), getHeight() - 10);
	contentraw->setSize(prefw, prefh);
	std::unique_ptr<Component> content(contentraw);
	auto & cb = CallOutBox::launchAsynchronously(std::move(content), m_render_button.getBounds(), this);
}

void PaulstretchpluginAudioProcessorEditor::showAudioSetup()
{
    if (showAudioSettingsDialog) {
        showAudioSettingsDialog(&m_settings_button, this);
    }
}

void PaulstretchpluginAudioProcessorEditor::executeModalMenuAction(int menuid, int r)
{
	if (r >= 200 && r < 210)
	{
		int caplen = m_capturelens[r - 200];
		*processor.getFloatParameter(cpi_max_capture_len) = (float)caplen;
	}
	else if (r == 1)
	{
		toggleBool(processor.m_play_when_host_plays);
	}
	else if (r == 2)
	{
		toggleBool(processor.m_capture_when_host_plays);
	}
	else if (r == 8)
	{
		toggleBool(processor.m_mute_while_capturing);
	}
    else if (r == 10)
    {
        toggleBool(processor.m_mute_processed_while_capturing);
    }
	else if (r == 4)
	{
		processor.resetParameters();
	}
	else if (r == 5)
	{
		toggleBool(processor.m_load_file_with_state);
	}
	else if (r == 9)
	{
		toggleBool(processor.m_save_captured_audio);
	}
	else if (r == 3)
	{
		showAbout();
	}
    else if (r == 11)
    {
        showAudioSetup();
    }

	else if (r == 6)
	{
		ValueTree tree = processor.getStateTree(true, true);
		MemoryBlock destData;
		MemoryOutputStream stream(destData, true);
		tree.writeToStream(stream);
		String txt = Base64::toBase64(destData.getData(), destData.getSize());
		SystemClipboard::copyTextToClipboard(txt);
	}
	else if (r == 7)
	{
		toggleBool(processor.m_show_technical_info);
		processor.m_propsfile->m_props_file->setValue("showtechnicalinfo", processor.m_show_technical_info);
	}
}



void PaulstretchpluginAudioProcessorEditor::paint (Graphics& g)
{
	g.fillAll(Colour(0xff404040));
}

void PaulstretchpluginAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds();

    bounds.reduce(4, 0);
    //bounds.removeFromRight(4);

    int w = bounds.getWidth();
    int rowheight = 24;
    int togglerowheight = 24;
    int buttonrowheight = 32;
    int minw = w / 3;
    int toggleminw = 90;
    int minh = 32;
    int buttw = 60;
    int buttminw = 36;
    int minitemw = 260;
    int margin = 1;

#if JUCE_IOS
    togglerowheight = 32;
    rowheight = 36;
    buttonrowheight = 40;
    minh = 40;
#endif

    DBG("Resized: " << getWidth() << " " << getHeight());

    processor.setLastPluginBounds(getLocalBounds());

    FlexBox mainbox;
    mainbox.flexDirection = FlexBox::Direction::column;


    FlexBox topbox;
    topbox.flexDirection = FlexBox::Direction::row;
    topbox.flexWrap = FlexBox::Wrap::wrap;
    topbox.alignContent = FlexBox::AlignContent::flexStart;

    topbox.items.add(FlexItem(buttw, buttonrowheight, m_import_button).withMargin(1).withFlex(1).withMaxWidth(130));
    topbox.items.add(FlexItem(buttw, buttonrowheight, m_settings_button).withMargin(1).withFlex(1).withMaxWidth(130));
    if (JUCEApplicationBase::isStandaloneApp())
    {
        topbox.items.add(FlexItem(buttw, buttonrowheight, m_render_button).withMargin(1).withFlex(1).withMaxWidth(130));
    }
    topbox.items.add(FlexItem(4, 4));
    topbox.items.add(FlexItem(80, buttonrowheight, m_perfmeter).withMargin(1).withFlex(1).withMaxWidth(110).withMaxHeight(24).withAlignSelf(FlexItem::AlignSelf::center));
    topbox.items.add(FlexItem(140, 26, m_info_label).withMargin(1).withFlex(2));

    topbox.performLayout(Rectangle<int>(0,0,w - 2*margin,400)); // test run to calculate actual used height
    int topboxh = topbox.items.getLast().currentBounds.getBottom() + + topbox.items.getLast().margin.bottom;


    FlexBox togglesbox;
    togglesbox.flexDirection = FlexBox::Direction::row;
    togglesbox.flexWrap = FlexBox::Wrap::wrap;
    togglesbox.alignContent = FlexBox::AlignContent::flexStart;

    togglesbox.items.add(FlexItem(buttminw, buttonrowheight, *m_parcomps[cpi_capture_trigger]).withMargin(margin).withFlex(1).withMaxWidth(65));
    togglesbox.items.add(FlexItem(3, buttonrowheight).withFlex(0.1));

    togglesbox.items.add(FlexItem(buttminw, buttonrowheight, *m_rewind_button).withMargin(1).withFlex(1).withMaxWidth(65));
    togglesbox.items.add(FlexItem(0, buttonrowheight).withFlex(0.1).withMaxWidth(10));
    togglesbox.items.add(FlexItem(buttminw, buttonrowheight, *m_parcomps[cpi_pause_enabled]).withMargin(margin).withFlex(1).withMaxWidth(65));
    togglesbox.items.add(FlexItem(0, buttonrowheight).withFlex(0.1).withMaxWidth(10));
    togglesbox.items.add(FlexItem(buttminw, buttonrowheight, *m_parcomps[cpi_looping_enabled]).withMargin(margin).withFlex(1).withMaxWidth(65));
    togglesbox.items.add(FlexItem(0, buttonrowheight).withFlex(0.1).withMaxWidth(10));
    togglesbox.items.add(FlexItem(buttminw, buttonrowheight, *m_parcomps[cpi_freeze]).withMargin(margin).withFlex(1).withMaxWidth(65));

    togglesbox.items.add(FlexItem(3, buttonrowheight).withFlex(0.1));

    //togglesbox.items.add(FlexItem(toggleminw, togglerowheight, *m_parcomps[cpi_bypass_stretch]).withMargin(margin).withFlex(1).withMaxWidth(150));
    togglesbox.items.add(FlexItem(buttminw + 15, buttonrowheight, *m_parcomps[cpi_passthrough]).withMargin(margin).withFlex(1.5).withMaxWidth(85));

    togglesbox.items.add(FlexItem(2, buttonrowheight));

    togglesbox.performLayout(Rectangle<int>(0,0,w - 2*margin,400)); // test run to calculate actual used height
    int toggleh = togglesbox.items.getLast().currentBounds.getBottom() + togglesbox.items.getLast().margin.bottom;
    DBG("toggle h: " << toggleh);

    FlexBox volbox;
    volbox.flexDirection = FlexBox::Direction::row;
    volbox.flexWrap = FlexBox::Wrap::wrap;
    volbox.alignContent = FlexBox::AlignContent::flexStart;
    volbox.items.add(FlexItem(minitemw*0.75f, rowheight, *m_parcomps[cpi_main_volume]).withMargin(margin).withFlex(1));


#if !JUCE_IOS
    FlexBox inoutbox;
    int inoutminw = 140;
    int inoutmaxw = 200;

    inoutbox.flexDirection = FlexBox::Direction::row;
    inoutbox.items.add(FlexItem(inoutminw, rowheight, *m_parcomps[cpi_num_inchans]).withMargin(margin).withFlex(0.5).withMaxWidth(inoutmaxw));
    inoutbox.items.add(FlexItem(inoutminw, rowheight, *m_parcomps[cpi_num_outchans]).withMargin(margin).withFlex(0.5).withMaxWidth(inoutmaxw));

    volbox.items.add(FlexItem(2*inoutminw, rowheight, inoutbox).withMargin(margin).withFlex(1.5).withMaxWidth(2*inoutmaxw + 10));
#endif

    volbox.performLayout(Rectangle<int>(0,0,w - 2*margin,400)); // test run to calculate actual used height
    int volh = volbox.items.getLast().currentBounds.getBottom() + volbox.items.getLast().margin.bottom;
    int stretchH = m_stretchgroup->getMinimumHeight(w - 2*margin);
    stretchH = jmax(stretchH, (int) (minh*1.1f));


    FlexBox groupsbox;
    groupsbox.flexDirection = FlexBox::Direction::column;

    int scrollw = m_groupviewport->getScrollBarThickness() ;

    int gheight = 0;
    int groupmargin = 1;
    int groupw = w - 2*groupmargin - scrollw;
    // groups


    minh = m_pargroups[HarmonicsGroup]->getMinimumHeight(groupw);
    groupsbox.items.add(FlexItem(minw, minh, *m_pargroups[HarmonicsGroup]).withMargin(groupmargin));
    gheight += minh + 2*groupmargin;

    minh = m_pargroups[TonalNoiseGroup]->getMinimumHeight(groupw);
    groupsbox.items.add(FlexItem(minw, minh, *m_pargroups[TonalNoiseGroup]).withMargin(groupmargin));
    gheight += minh + 2*groupmargin;

    FlexBox shiftbox;
    FlexBox scompbox;

    if (w >= 700) {
        shiftbox.flexDirection = FlexBox::Direction::row;
        minh = m_pargroups[FrequencyShiftGroup]->getMinimumHeight(minw);
        shiftbox.items.add(FlexItem(minw, minh, *m_pargroups[FrequencyShiftGroup]).withFlex(1));
        shiftbox.items.add(FlexItem(4, minh));
        shiftbox.items.add(FlexItem(minw, minh, *m_pargroups[PitchShiftGroup]).withFlex(1));
        groupsbox.items.add(FlexItem(minw, minh, shiftbox).withMargin(groupmargin));
        gheight += minh + 2*groupmargin;

        scompbox.flexDirection = FlexBox::Direction::row;
        minh = m_pargroups[FrequencySpreadGroup]->getMinimumHeight(minw);
        scompbox.items.add(FlexItem(minw, minh, *m_pargroups[FrequencySpreadGroup]).withFlex(1));
        scompbox.items.add(FlexItem(4, minh));
        scompbox.items.add(FlexItem(minw, minh, *m_pargroups[CompressGroup]).withFlex(1));
        groupsbox.items.add(FlexItem(minw, minh, scompbox).withMargin(groupmargin));
        gheight += minh + 2*groupmargin;
    } else {
        minh = m_pargroups[FrequencyShiftGroup]->getMinimumHeight(groupw);
        groupsbox.items.add(FlexItem(minw, minh, *m_pargroups[FrequencyShiftGroup]).withMargin(groupmargin));
        gheight += minh + 2*groupmargin;

        minh = m_pargroups[PitchShiftGroup]->getMinimumHeight(groupw);
        groupsbox.items.add(FlexItem(minw, minh, *m_pargroups[PitchShiftGroup]).withMargin(groupmargin));
        gheight += minh + 2*groupmargin;

        minh = m_pargroups[FrequencySpreadGroup]->getMinimumHeight(groupw);
        groupsbox.items.add(FlexItem(minw, minh, *m_pargroups[FrequencySpreadGroup]).withMargin(groupmargin));
        gheight += minh + 2*groupmargin;

        minh = m_pargroups[CompressGroup]->getMinimumHeight(groupw);
        groupsbox.items.add(FlexItem(minw, minh, *m_pargroups[CompressGroup]).withMargin(groupmargin));
        gheight += minh + 2*groupmargin;
    }

    minh = m_pargroups[FilterGroup]->getMinimumHeight(groupw);
    groupsbox.items.add(FlexItem(minw, minh, *m_pargroups[FilterGroup]).withMargin(groupmargin));
    gheight += minh + 2*groupmargin;

    minh = m_posgroup->getMinimumHeight(groupw);
    groupsbox.items.add(FlexItem(minw, minh, *m_posgroup).withMargin(groupmargin));
    gheight += minh + 2*groupmargin;



    /*
    for (const auto & group : m_pargroups) {
        int minheight = group.second->getMinimumHeight(w);
        groupsbox.items.add(FlexItem(minw, minheight, *(group.second)).withMargin(2));
        gheight += minheight + 4;
    }
     */

    DBG("group tot height: " << gheight);

    int useh = gheight;
    int vpminh = jmin(useh, 140);
    int tabminh = 200;
    int orderminh = 38;

#if JUCE_IOS
    tabminh = 234;
    orderminh = 44;
#endif


    mainbox.items.add(FlexItem(6, 2));

    mainbox.items.add(FlexItem(minw, topboxh, topbox).withMargin(margin).withFlex(0));

    mainbox.items.add(FlexItem(minw, toggleh, togglesbox).withMargin(margin).withFlex(0));


    auto reparentIfNecessary = [] (Component * comp, Component *newparent) {
        if (comp->getParentComponent() != newparent)
            newparent->addAndMakeVisible(comp);
    };

    std::function<void(FlexBox & box, Component *newparent)> reparentItemsIfNecessary;
    reparentItemsIfNecessary = [&reparentItemsIfNecessary,&reparentIfNecessary] (FlexBox & box, Component *newparent) {
        for (auto & item : box.items ) {
            if (item.associatedFlexBox) {
                reparentItemsIfNecessary(*item.associatedFlexBox, newparent);
            }
            else if (item.associatedComponent) {
                reparentIfNecessary(item.associatedComponent, newparent);
            }
        }
    };


    int totminh = vpminh + orderminh + tabminh + topboxh + toggleh + volh + stretchH + 18;

    int shortthresh = vpminh + orderminh + tabminh + topboxh + toggleh;

    if (getHeight() < totminh) {
        // not enough vertical space, put the top items in the scrollable viewport
        // may have to reparent them
        reparentIfNecessary(m_stretchgroup.get(), m_groupcontainer.get());
        //reparentItemsIfNecessary(togglesbox, m_groupcontainer.get());
        reparentItemsIfNecessary(volbox, m_groupcontainer.get());

        groupsbox.items.insert(0, FlexItem(minw, stretchH, *m_stretchgroup).withMargin(groupmargin).withFlex(0));
        groupsbox.items.insert(0, FlexItem(minw, volh, volbox).withMargin(groupmargin).withFlex(0));
        //groupsbox.items.insert(0, FlexItem(minw, toggleh, togglesbox).withMargin(groupmargin).withFlex(0));

        useh += /*toggleh + */ volh + stretchH + 6*groupmargin;

        if (getHeight() < shortthresh) {
            // really not much space, put group scroll in a new tab
            if (m_wavefilter_tab.getNumTabs() <= 3) {
                m_wavefilter_tab.addTab("Controls", Colour(0xff555555), m_groupviewport.get(), false);
                m_wavefilter_tab.setCurrentTabIndex(3);
            }

            reparentIfNecessary(&m_spec_order_ed, m_groupcontainer.get());

            groupsbox.items.add(FlexItem(minw, orderminh, m_spec_order_ed).withMargin(2));

            useh += orderminh + 4;

            m_shortMode = true;
        } else {
            reparentIfNecessary(&m_spec_order_ed, this);

            if (m_wavefilter_tab.getNumTabs() > 3) {
                // bring it back
                m_wavefilter_tab.removeTab(3);
                m_wavefilter_tab.setCurrentTabIndex(0);
                addAndMakeVisible(m_groupviewport.get());
            }
            m_shortMode = false;
        }


    } else {
        m_shortMode = false;

        if (m_wavefilter_tab.getNumTabs() > 3) {
            // bring it back
            m_wavefilter_tab.removeTab(3);
            m_wavefilter_tab.setCurrentTabIndex(0);
            addAndMakeVisible(m_groupviewport.get());
        }

        // may have to reparent them
        reparentIfNecessary(m_groupviewport.get(), this);
        reparentIfNecessary(&m_spec_order_ed, this);
        reparentIfNecessary(m_stretchgroup.get(), this);
        //reparentItemsIfNecessary(togglesbox, this);
        reparentItemsIfNecessary(volbox, this);


        //mainbox.items.add(FlexItem(minw, toggleh, togglesbox).withMargin(margin).withFlex(0));
        mainbox.items.add(FlexItem(minw, volh, volbox).withMargin(margin).withFlex(0));
        mainbox.items.add(FlexItem(minw, stretchH, *m_stretchgroup).withMargin(margin).withFlex(0));
    }

    mainbox.items.add(FlexItem(6, 3));


    if (!m_shortMode) {
        mainbox.items.add(FlexItem(w, vpminh, *m_groupviewport).withMargin(0).withFlex(1).withMaxHeight(useh + 4));

        mainbox.items.add(FlexItem(6, 2));

        mainbox.items.add(FlexItem(w-4, orderminh, m_spec_order_ed).withMargin(2).withFlex(0.1).withMaxHeight(60));
        mainbox.items.add(FlexItem(6, 2));
    }


    mainbox.items.add(FlexItem(w, tabminh, m_wavefilter_tab).withMargin(0).withFlex(0.1));

    mainbox.items.add(FlexItem(6, 4));

    mainbox.performLayout(bounds);

    if ( m_shortMode) {
        auto totgroupw = jmax(260, m_groupviewport->getWidth()) - scrollw;
        auto groupsbounds = Rectangle<int>(0, 0, totgroupw, useh);
        auto layoutbounds = groupsbounds.translated(2, 0).withWidth(totgroupw - 3);
        m_groupcontainer->setBounds(groupsbounds);
        groupsbox.performLayout(layoutbounds);
    }
    else {
        auto totgroupw = jmax(260, w) - scrollw;
        auto groupsbounds = Rectangle<int>(0, 0, totgroupw, useh);
        m_groupcontainer->setBounds(groupsbounds);
        groupsbox.performLayout(groupsbounds);
    }

    int zscrollh = 18;
#if JUCE_IOS
    zscrollh = 28;
#endif

    m_wavecomponent.setBounds(m_wave_container->getX(), 0, m_wave_container->getWidth(),
		m_wave_container->getHeight()-zscrollh-1);



	m_zs.setBounds(m_wave_container->getX(), m_wavecomponent.getBottom(), m_wave_container->getWidth(), zscrollh);
	//m_wavecomponent.setBounds(1, m_spec_order_ed.getBottom()+1, getWidth()-2, remain_h/5*4);

    if (m_shortMode) {
        m_groupcontainer->repaint();
    }

}

void PaulstretchpluginAudioProcessorEditor::timerCallback(int id)
{
	if (id == 1)
	{
        if (!tooltipWindow && getParentComponent()) {
            Component* dw = this;
            if (dw) {
                tooltipWindow = std::make_unique<CustomTooltipWindow>(this, dw);
            }
        }

		for (int i = 0; i < m_parcomps.size(); ++i)
		{
			if (m_parcomps[i]!=nullptr)
				m_parcomps[i]->updateComponent();
		}
		m_free_filter_component.updateParameterComponents();
		if (processor.isRecordingEnabled())
		{
			m_wavecomponent.setRecordingPosition(processor.getRecordingPositionPercent());
		} else
			m_wavecomponent.setRecordingPosition(-1.0);
		m_wavecomponent.setAudioInfo(processor.getSampleRateChecked(), processor.getStretchSource()->getLastSeekPos(),
			processor.getStretchSource()->getFFTSize());
		String infotext; 
		String waveinfotext;
		if (processor.m_show_technical_info)
		{
			double sr = processor.getStretchSource()->getInfileSamplerate();
			if (sr>0.0)
				waveinfotext += String(processor.getStretchSource()->getDiskReadSampleCount()/sr) + " seconds read from disk\n";
			waveinfotext += String(processor.m_prepare_count)+" prepareToPlay calls\n";
			waveinfotext += String(processor.getStretchSource()->m_param_change_count)+" parameter changes handled\n";
			waveinfotext += String(m_wavecomponent.m_image_init_count) + " waveform image inits\n" 
				+ String(m_wavecomponent.m_image_update_count) + " waveform image updates\n";
			m_wavecomponent.m_infotext = waveinfotext;
		}
		else
			m_wavecomponent.m_infotext = {};
		infotext += m_last_err + " [FFT size " +
			String(processor.getStretchSource()->getFFTSize())+"]";
		double outlen = processor.getStretchSource()->getOutputDurationSecondsForRange(processor.getStretchSource()->getPlayRange(), 
			processor.getStretchSource()->getFFTSize());
		infotext += " [Output length " + secondsToString2(outlen)+"]";
		if (processor.m_abnormal_output_samples > 0)
			infotext += " " + String(processor.m_abnormal_output_samples) + " invalid sample values";
		if (processor.isNonRealtime())
			infotext += " (offline rendering)";
		if (processor.m_playposinfo.isPlaying)
			infotext += " "+String(processor.m_playposinfo.timeInSeconds,1);
		if (processor.m_offline_render_state >= 0 && processor.m_offline_render_state <= 100)
			infotext += String(processor.m_offline_render_state)+"%";
		if (processor.m_capture_save_state == 1)
			infotext += "Saving captured audio...";
		m_info_label.setText(infotext, dontSendNotification);

        for (auto & group : m_pargroups) {
            group.second->updateParameterComponents();
        }

        m_stretchgroup->setToggleEnabled(!*processor.getBoolParameter(cpi_bypass_stretch));

        if (AudioParameterBool* enablepar = dynamic_cast<AudioParameterBool*>(processor.getBoolParameter(cpi_pause_enabled))) {
            m_perfmeter.enabled = !enablepar->get();
        }

	}
	if (id == 2)
	{
		m_wavecomponent.setTimeSelection(processor.getTimeSelection());
		if (processor.m_state_dirty)
		{
			//m_spec_order_ed.setSource(processor.getStretchSource());
			processor.m_state_dirty = false;
		}
	}
	if (id == 3)
	{
		processor.m_free_filter_envelope->updateMinMaxValues();
		m_free_filter_component.repaint();
		m_spec_order_ed.repaint();

        if (*processor.getBoolParameter(cpi_bypass_stretch)) {
            m_stretchgroup->replaceParameterComponent(m_parcomps[cpi_stretchamount].get(), m_parcomps[cpi_dryplayrate].get());
        } else {
            m_stretchgroup->replaceParameterComponent(m_parcomps[cpi_dryplayrate].get(), m_parcomps[cpi_stretchamount].get());
        }

		//m_parcomps[cpi_dryplayrate]->setVisible(*processor.getBoolParameter(cpi_bypass_stretch));
        //m_parcomps[cpi_stretchamount]->setVisible(!*processor.getBoolParameter(cpi_bypass_stretch));

        //if (m_wavefilter_tab.getCurrentTabIndex() != processor.m_cur_tab_index)
		//	m_wavefilter_tab.setCurrentTabIndex(processor.m_cur_tab_index, false);
	}
}

bool PaulstretchpluginAudioProcessorEditor::isInterestedInFileDrag(const StringArray & files)
{
	if (files.size() == 0)
		return false;
	File f(files[0]);
	String extension = f.getFileExtension().toLowerCase();
	if (processor.m_afm->getWildcardForAllFormats().containsIgnoreCase(extension))
		return true;
	return false;

}

void PaulstretchpluginAudioProcessorEditor::filesDropped(const StringArray & files, int x, int y)
{
	if (files.size() > 0)
	{
        File file(files[0]);
		URL url = URL(file);
		processor.setAudioFile(url);
		toFront(true);
	}
}

void PaulstretchpluginAudioProcessorEditor::urlOpened(const URL& url)
{
    DBG("Got URL: " << url.toString(false));
    std::unique_ptr<InputStream> wi (url.createInputStream (false));
    if (wi != nullptr)
    {
        DBG("Attempting to load after input stream create: " << url.toString(false));
        processor.setAudioFile(url);
    } 
    toFront(true);
}


bool PaulstretchpluginAudioProcessorEditor::keyPressed(const KeyPress & press)
{
	std::function<bool(void)> action;
	if (press == 'I')
		action = [this]() { m_import_button.onClick(); ; return true; };
	return action && action();
}

void PaulstretchpluginAudioProcessorEditor::showSettingsMenu()
{
    PopupMenu m_settings_menu;
    if (JUCEApplicationBase::isStandaloneApp()) {
        m_settings_menu.addItem(11, "Audio Setup...", true, false);
    }
    m_settings_menu.addItem(4, "Reset parameters", true, false);
    m_settings_menu.addSeparator();
	m_settings_menu.addItem(5, "Load file with plugin state", true, processor.m_load_file_with_state);
	m_settings_menu.addItem(1, "Play when host transport running", true, processor.m_play_when_host_plays);
	m_settings_menu.addItem(2, "Capture when host transport running", true, processor.m_capture_when_host_plays);
    m_settings_menu.addSeparator();
	m_settings_menu.addItem(8, "Mute passthrough while capturing", true, processor.m_mute_while_capturing);
    m_settings_menu.addItem(10, "Mute processed audio output while capturing", true, processor.m_mute_processed_while_capturing);
	m_settings_menu.addItem(9, "Save captured audio to disk", true, processor.m_save_captured_audio);
	int capturelen = *processor.getFloatParameter(cpi_max_capture_len);
	PopupMenu capturelenmenu;
	
	for (int i=0;i<m_capturelens.size();++i)
		capturelenmenu.addItem(200+i, String(m_capturelens[i])+" seconds", true, capturelen == m_capturelens[i]);
	m_settings_menu.addSubMenu("Capture buffer length", capturelenmenu);
	
    m_settings_menu.addSeparator();
	m_settings_menu.addItem(3, "About...", true, false);
#ifdef JUCE_DEBUG
	m_settings_menu.addItem(6, "Dump preset to clipboard", true, false);
#endif
	m_settings_menu.addItem(7, "Show technical info", true, processor.m_show_technical_info);

    auto options = PopupMenu::Options().withTargetComponent(&m_settings_button);
#if JUCE_IOS
    options = options.withStandardItemHeight(34);
#endif
    if (!JUCEApplicationBase::isStandaloneApp()) {
        options = options.withParentComponent(this);
    }
    m_settings_menu.showMenuAsync(options,
		ModalCallbackFunction::forComponent(handleSettingsMenuModalCallback, this));
}

void PaulstretchpluginAudioProcessorEditor::showAbout()
{
    String fftlib;
#if !PS_USE_VDSP_FFT
    fftlib = fftwf_version;
#endif
	String juceversiontxt = String("JUCE ") + String(JUCE_MAJOR_VERSION) + "." + String(JUCE_MINOR_VERSION);
	String title = String(JucePlugin_Name) + " " + String(JucePlugin_VersionString);
#ifdef JUCE_DEBUG
	title += " (DEBUG)";
#endif
	String vstInfo;
	if (processor.wrapperType == AudioProcessor::wrapperType_VST ||
		processor.wrapperType == AudioProcessor::wrapperType_VST3)
		vstInfo = "VST Plug-In Technology by Steinberg.\n\n";
	PluginHostType host;

    auto * content = new Label();
    String text = title + "\n\n" +
    "Plugin/Application for extreme time stretching and other sound processing\nBuilt on " + String(__DATE__) + " " + String(__TIME__) + "\n"
    "Copyright (C) 2006-2011 Nasca Octavian Paul, Tg. Mures, Romania\n"
    "(C) 2017-2021 Xenakios\n"
    "(C) 2022 Jesse Chappell\n\n"
    +vstInfo;

    if (fftlib.isNotEmpty())
        text += String("Using ") + fftlib + String(" for FFT\n\n");

#if !JUCE_IOS
    text += juceversiontxt + String(" used under the GPL license.\n\n");
#endif

    // text += String("GPL licensed source code for this plugin at : https://bitbucket.org/xenakios/paulstretchplugin/overview\n");

    if (host.type != juce::PluginHostType::UnknownHost) {
        text += String("Running in : ") + host.getHostDescription()+ String("\n");
    }

    content->setJustificationType(Justification::centred);
    content->setText(text, dontSendNotification);

    auto wrap = std::make_unique<Viewport>();
    wrap->setViewedComponent(content, true); // takes ownership of content

    //std::unique_ptr<SettingsComponent> contptr(content);
    int defWidth = 450;
    int defHeight = 350;
#if JUCE_IOS
    defWidth = 320;
    defHeight = 350;
#endif

    content->setSize (defWidth, defHeight);
    wrap->setSize(jmin(defWidth, getWidth() - 20), jmin(defHeight, getHeight() - 24));

    auto bounds = getLocalArea(nullptr, m_settings_button.getScreenBounds());
    CallOutBox::launchAsynchronously(std::move(wrap), bounds, this);
}

void PaulstretchpluginAudioProcessorEditor::toggleFileBrowser()
{
#if JUCE_IOS

    String curropendir = processor.m_propsfile->m_props_file->getValue("importfilefolder",
                                                                    File::getSpecialLocation(File::userDocumentsDirectory).getFullPathName());

    Component * parent = JUCEApplication::isStandaloneApp() ? nullptr : this;

    fileChooser.reset(new FileChooser("Choose an audio file to open...",
                    curropendir,
                    "*.wav;*.mp3;*.m4a;*.aif;*.aiff;*.caf;*.ogg;*.flac",
                    true, false, parent));


    fileChooser->launchAsync (FileBrowserComponent::openMode | FileBrowserComponent::canSelectFiles,
                     [this] (const FileChooser& chooser)
                     {
        auto results = chooser.getURLResults();
        if (results.size() > 0)
        {
            auto url = results.getReference (0);

            std::unique_ptr<InputStream> wi (url.createInputStream (false));
            if (wi != nullptr)
            {
                File file = url.getLocalFile();
                DBG("Attempting to load from: " << file.getFullPathName());

                //curropendir = file.getParentDirectory();
                processor.setAudioFile(url);
                processor.m_propsfile->m_props_file->setValue("importfilefolder", file.getParentDirectory().getFullPathName());
            }
        }
    });


#else
    if (m_filechooser == nullptr)
	{
		m_filechooser = std::make_unique<MyFileBrowserComponent>(processor);
		addChildComponent(m_filechooser.get());
	}
	m_filechooser->setBounds(0, m_import_button.getBottom(), getWidth()/2, getHeight() - 75);
	m_filechooser->setVisible(!m_filechooser->isVisible());
	if (m_filechooser->isVisible())
		m_import_button.setButtonText("Hide browser");
	else
		m_import_button.setButtonText("Show browser");
#endif
}

WaveformComponent::WaveformComponent(AudioFormatManager* afm, AudioThumbnail* thumb, StretchAudioSource* sas)
	: m_sas(sas)
{
	TimeSelectionChangedCallback = [](Range<double>, int) {};
#ifdef JUCE_MODULE_AVAILABLE_juce_opengl
	if (m_use_opengl == true)
		m_ogl.attachTo(*this);
#endif
	m_thumbnail = thumb;
	m_thumbnail->addChangeListener(this);
	setOpaque(true);
}

WaveformComponent::~WaveformComponent()
{
#ifdef JUCE_MODULE_AVAILABLE_juce_opengl
	if (m_use_opengl == true)
		m_ogl.detach();
#endif
	m_thumbnail->removeChangeListener(this);
}

void WaveformComponent::changeListenerCallback(ChangeBroadcaster * /*cb*/)
{
	jassert(MessageManager::getInstance()->isThisTheMessageThread());
	m_image_dirty = true;
	//repaint();
}

void WaveformComponent::updateCachedImage()
{
	Graphics tempg(m_waveimage);
	tempg.fillAll(Colours::black);
	tempg.setColour(Colours::darkgrey);
	double thumblen = m_thumbnail->getTotalLength();
	m_thumbnail->drawChannels(tempg, { 0,0,getWidth(),getHeight() - m_topmargin },
		thumblen*m_view_range.getStart(), thumblen*m_view_range.getEnd(), 1.0f);
	m_image_dirty = false;
	++m_image_update_count;
}

void WaveformComponent::paint(Graphics & g)
{
	jassert(GetFileCallback);
	//Logger::writeToLog("Waveform component paint");
	g.fillAll(Colours::black);
	g.setColour(Colours::darkgrey);
	g.fillRect(0, 0, getWidth(), m_topmargin);
	if (m_thumbnail == nullptr || m_thumbnail->getTotalLength() < 0.01)
	{
		g.setColour(Colours::aqua.darker());
		g.drawText("No file loaded", 2, m_topmargin + 2, getWidth(), 20, Justification::topLeft);
		return;
	}
	g.setColour(Colours::lightslategrey);
	double thumblen = m_thumbnail->getTotalLength();
	double tick_interval = 1.0;
	if (thumblen > 60.0)
		tick_interval = 5.0;
	for (double secs = 0.0; secs < thumblen; secs += tick_interval)
	{
		float tickxcor = (float)jmap<double>(secs,
			thumblen*m_view_range.getStart(), thumblen*m_view_range.getEnd(), 0.0f, (float)getWidth());
		g.drawLine(tickxcor, 0.0, tickxcor, (float)m_topmargin, 1.0f);
	}
	
	
	bool m_use_cached_image = true;
	if (m_use_cached_image == true)
	{
		if (m_image_dirty == true || m_waveimage.getWidth() != getWidth()
			|| m_waveimage.getHeight() != getHeight() - m_topmargin)
		{
			if (m_waveimage.getWidth() != getWidth()
				|| m_waveimage.getHeight() != getHeight() - m_topmargin)
			{
				m_waveimage = Image(Image::ARGB, getWidth(), getHeight() - m_topmargin, true);
				++m_image_init_count;
			}
			updateCachedImage();
		}
		g.drawImage(m_waveimage, 0, m_topmargin, getWidth(), getHeight() - m_topmargin, 0, 0, getWidth(), getHeight() - m_topmargin);

	}
	else
	{
		g.setColour(Colours::darkgrey);
		m_thumbnail->drawChannels(g, { 0,m_topmargin,getWidth(),getHeight() - m_topmargin },
			thumblen*m_view_range.getStart(), thumblen*m_view_range.getEnd(), 1.0f);
	}
	if (m_sr > 0.0 && m_fft_size > 0 && m_time_sel_start>=0.0)
	{
		tick_interval = 1.0 / m_sr * m_fft_size;
		/*
		for (double secs = m_time_sel_start*thumblen; secs < m_time_sel_end*thumblen; secs += tick_interval)
		{
			float tickxcor = (float)jmap<double>(fmod(secs, thumblen),
				thumblen*m_view_range.getStart(), thumblen*m_view_range.getEnd(), 0.0f, (float)getWidth());
			g.drawLine(tickxcor, (float)m_topmargin, tickxcor, (float)50, 2.0f);
		}
		*/
	}

    Colour selcolor(0xffccaacc);

	if (m_is_at_selection_drag_area)
		g.setColour(selcolor.withAlpha(0.45f));
	else
		g.setColour(selcolor.withAlpha(0.4f));

	double sel_len = m_time_sel_end - m_time_sel_start;
	//if (sel_len > 0.0 && sel_len < 1.0)
	{
		int xcorleft = normalizedToViewX<int>(m_time_sel_start); 
		int xcorright = normalizedToViewX<int>(m_time_sel_end);
		g.fillRect(xcorleft, m_topmargin, xcorright - xcorleft, getHeight() - m_topmargin);
	}
	if (m_file_cached.first.getLength() > 0.0 && m_infotext.isEmpty() == false)
	{
		g.setColour(Colours::red.withAlpha(0.2f));
		int xcorleft = (int)jmap<double>(m_file_cached.first.getStart(), m_view_range.getStart(), m_view_range.getEnd(), 0, getWidth());
		int xcorright = (int)jmap<double>(m_file_cached.first.getEnd(), m_view_range.getStart(), m_view_range.getEnd(), 0, getWidth());
		g.fillRect(xcorleft, 0, xcorright - xcorleft, getHeight());
		xcorleft = (int)jmap<double>(m_file_cached.second.getStart(), m_view_range.getStart(), m_view_range.getEnd(), 0, getWidth());
		xcorright = (int)jmap<double>(m_file_cached.second.getEnd(), m_view_range.getStart(), m_view_range.getEnd(), 0, getWidth());
		if (xcorright - xcorleft>0)
		{
			g.setColour(Colours::blue.withAlpha(0.2f));
			g.fillRect(xcorleft, m_topmargin / 2, xcorright - xcorleft, getHeight());
		}
		g.setColour(Colours::white);
		//g.drawText(toString(m_file_cached.first), 0, 30, 200, 30, Justification::centredLeft);
		g.drawMultiLineText(m_infotext, 0, 30, getWidth(), Justification::topLeft);
	}

	g.setColour(Colours::white);
	if (CursorPosCallback)
	{
		double timediff = (Time::getMillisecondCounterHiRes() - m_last_source_pos_update_time)*(1.0/m_sas->getRate());
		double curpos = ((double)m_last_source_pos / m_sas->getOutputSamplerate());
		double prebufoffset = (double)m_sas->m_prebuffersize / m_sas->getOutputSamplerate();
		curpos -= prebufoffset;
		curpos = 1.0 / m_sas->getInfileLengthSeconds()*(curpos+(timediff / 1000.0));
		//g.fillRect(normalizedToViewX<int>(curpos), m_topmargin, 1, getHeight() - m_topmargin);
		//g.drawText(String(curpos), 1, 30, 200,30, Justification::left);
		g.fillRect(normalizedToViewX<int>(CursorPosCallback()), m_topmargin, 1, getHeight() - m_topmargin);
	}
	if (m_rec_pos >= 0.0)
	{
		g.setColour(Colours::lightpink);
		g.fillRect(normalizedToViewX<int>(m_rec_pos), m_topmargin, 1, getHeight() - m_topmargin);
	}
	g.setColour(Colours::aqua);
	g.drawText(URL::removeEscapeChars(GetFileCallback().getFileName()), 2, m_topmargin + 2, getWidth(), 20, Justification::topLeft);
	g.drawText(secondsToString2(thumblen), getWidth() - 200, m_topmargin + 2, 200, 20, Justification::topRight);
}

void WaveformComponent::timerCallback()
{
	if (m_sas->getLastSourcePosition() != m_last_source_pos)
	{
		m_last_source_pos = m_sas->getLastSourcePosition();
		m_last_source_pos_update_time = Time::getMillisecondCounterHiRes();
	}
	m_file_cached = m_sas->getFileCachedRangesNormalized();
	repaint();
}

void WaveformComponent::setFileCachedRange(std::pair<Range<double>, Range<double>> rng)
{
	m_file_cached = rng;
}

void WaveformComponent::setTimerEnabled(bool b)
{
	if (b == true)
		startTimer(100);
	else
		stopTimer();
}

void WaveformComponent::setViewRange(Range<double> rng)
{
	m_view_range = rng;
	m_waveimage = Image();
	repaint();
}


void WaveformComponent::mouseDown(const MouseEvent & e)
{
	m_mousedown = true;
	m_lock_timesel_set = true;
    m_timedrag_started = false;

	double pos = viewXToNormalized(e.x);
	if (e.mods.isCommandDown())
	{
		if (SeekCallback)
		{
			SeekCallback(pos);
			m_last_startpos = pos;
		}
		m_didseek = true;
	}
	else
	{
		m_time_sel_drag_target = getTimeSelectionEdge(e.x, e.y);
		m_drag_time_start = pos;
		if (m_time_sel_drag_target == 0)
		{
			//m_time_sel_start = 0.0;
			//m_time_sel_end = 1.0;
		}
	}

	repaint();
}

void WaveformComponent::mouseUp(const MouseEvent & e)
{
    int seektopmargin = getHeight() / 2;

	if (m_didchangetimeselection)
	{
		TimeSelectionChangedCallback(Range<double>(m_time_sel_start, m_time_sel_end), 1);
		m_didchangetimeselection = false;
	}
    else if (e.y < seektopmargin) {
        double pos = viewXToNormalized(e.x);
        if (SeekCallback)
        {
            SeekCallback(pos);
            m_last_startpos = pos;
        }
    }

    m_is_dragging_selection = false;
    m_lock_timesel_set = false;
    m_mousedown = false;
    m_didseek = false;
    m_timedrag_started = false;
}

void WaveformComponent::mouseDrag(const MouseEvent & e)
{
	if (m_didseek == true)
		return;

    int dragthresh = 3;
#if JUCE_IOS
    dragthresh = 6;
#endif

    if (!m_timedrag_started && abs(e.getDistanceFromDragStartX()) > dragthresh) {
        m_timedrag_started = true;
    }

    if (!m_timedrag_started) return;

	if (m_time_sel_drag_target == 0 && e.y>=50 && m_is_dragging_selection==false)
	{
		m_time_sel_start = m_drag_time_start;
		m_time_sel_end = viewXToNormalized(e.x);
	}
	double curlen = m_time_sel_end - m_time_sel_start;
	if (m_time_sel_drag_target == 0 && m_is_at_selection_drag_area)
	{
		m_is_dragging_selection = true;
		double diff = m_drag_time_start - viewXToNormalized(e.x);
		m_time_sel_start = jlimit<double>(0.0, 1.0-curlen, m_time_sel_start - diff);
		m_time_sel_end = jlimit<double>(curlen, 1.0, m_time_sel_end - diff);
		m_drag_time_start -= diff;
	}
	curlen = m_time_sel_end - m_time_sel_start;
	
    if (m_time_sel_drag_target == 1)
	{
		m_time_sel_start = viewXToNormalized(e.x);
    }
	if (m_time_sel_drag_target == 2)
	{
		m_time_sel_end = viewXToNormalized(e.x);
    }
	if (m_time_sel_start > m_time_sel_end)
	{
		std::swap(m_time_sel_start, m_time_sel_end);
		if (m_time_sel_drag_target == 1)
			m_time_sel_drag_target = 2;
		else if (m_time_sel_drag_target == 2)
			m_time_sel_drag_target = 1;
	}
	m_time_sel_start = jlimit(0.0, 1.0, m_time_sel_start);
	m_time_sel_end = jlimit(0.0, 1.0, m_time_sel_end);

	if (TimeSelectionChangedCallback)
	{
		if (m_time_sel_end>m_time_sel_start)
			TimeSelectionChangedCallback(Range<double>(m_time_sel_start, m_time_sel_end), 0);
		else
			TimeSelectionChangedCallback(Range<double>(0.0, 1.0), 0);
	}
	m_didchangetimeselection = true;
	repaint();
}

void WaveformComponent::mouseMove(const MouseEvent & e)
{
	m_time_sel_drag_target = getTimeSelectionEdge(e.x, e.y);
	if (m_time_sel_drag_target == 0)
		setMouseCursor(MouseCursor::NormalCursor);
	if (m_time_sel_drag_target == 1)
		setMouseCursor(MouseCursor::LeftRightResizeCursor);
	if (m_time_sel_drag_target == 2)
		setMouseCursor(MouseCursor::LeftRightResizeCursor);
	Range<int> temp(normalizedToViewX<int>(m_time_sel_start), normalizedToViewX<int>(m_time_sel_end));
	m_is_at_selection_drag_area = temp.contains(e.x) == true && e.y < 50;
}

void WaveformComponent::mouseDoubleClick(const MouseEvent & e)
{
	m_time_sel_start = 0.0;
	m_time_sel_end = 1.0;
	TimeSelectionChangedCallback({ 0.0,1.0 }, 0);
	repaint();
}

void WaveformComponent::mouseWheelMove(const MouseEvent & e, const MouseWheelDetails & wd)
{
	//return;

    double width = getWidth();
	double normt = viewXToNormalized(e.x);
	double curlen = m_view_range.getLength();
    double zoomFactor = 1.0 - curlen;

    double newfact = jlimit(0.0, 1.0, zoomFactor + wd.deltaY);
    double xratio = e.x / width;
    auto newScale = jmax (0.001, 1.0 * (1.0 - jlimit (0.0, 0.99, newfact)));

    double t0 = normt - newScale * xratio;
    double t1 = normt + newScale * (1.0 - xratio);

    t0 = jlimit(0.0,1.0, t0);
    t1 = jlimit(0.0,1.0, t1);

    DBG("normt: " << normt << " posratio: " << xratio << " curlen: " << curlen << " t0: " << t0 << " t1: " << t1 << " delta: " << wd.deltaY);

    jassert(t1 > t0);
	m_view_range = { t0,t1 };

	//m_view_range = m_view_range.constrainRange({ 0.0, 1.0 });
	jassert(m_view_range.getStart() >= 0.0 && m_view_range.getEnd() <= 1.0);
	jassert(m_view_range.getLength() > 0.001);
	if (ViewRangeChangedCallback)
		ViewRangeChangedCallback(m_view_range);
	m_image_dirty = true;
	repaint();

}

void WaveformComponent::setAudioInfo(double sr, double seekpos, int fftsize)
{
	m_sr = sr;
	m_fft_size = fftsize;
	m_last_startpos = seekpos;
}

Range<double> WaveformComponent::getTimeSelection()
{
	if (m_time_sel_start >= 0.0 && m_time_sel_end>m_time_sel_start + 0.001)
		return { m_time_sel_start, m_time_sel_end };
	return { 0.0, 1.0 };
}

void WaveformComponent::setTimeSelection(Range<double> rng)
{
	if (m_lock_timesel_set == true)
		return;
	if (rng.isEmpty())
		rng = { -1.0,1.0 };
	m_time_sel_start = rng.getStart();
	m_time_sel_end = rng.getEnd();
	repaint();
}

int WaveformComponent::getTimeSelectionEdge(int x, int y)
{
    int touchradius = 5;
#if JUCE_IOS
    touchradius = 10;
#endif
	int xcorleft = (int)jmap<double>(m_time_sel_start, m_view_range.getStart(), m_view_range.getEnd(), 0, getWidth());
	int xcorright = (int)jmap<double>(m_time_sel_end, m_view_range.getStart(), m_view_range.getEnd(), 0, getWidth());
	if (juce::Rectangle<int>(xcorleft - touchradius, m_topmargin, 2*touchradius, getHeight() - m_topmargin).contains(x, y))
		return 1;
	if (juce::Rectangle<int>(xcorright - touchradius, m_topmargin, 2*touchradius, getHeight() - m_topmargin).contains(x, y))
		return 2;
	return 0;
}

SpectralVisualizer::SpectralVisualizer()
{
	m_img = Image(Image::RGB, 500, 200, true);
}

void SpectralVisualizer::setState(const ProcessParameters & pars, int nfreqs, double samplerate)
{
	double t0 = Time::getMillisecondCounterHiRes();
	double hz = 440.0;
	int numharmonics = 40;
	double scaler = 1.0 / numharmonics;
	if (m_img.getWidth()!=getWidth() || m_img.getHeight()!=getHeight())
		m_img = Image(Image::RGB, getWidth(), getHeight(), true);
	if (m_nfreqs == 0 || nfreqs != m_nfreqs)
	{
		m_nfreqs = nfreqs;
		m_insamples = std::vector<REALTYPE>(nfreqs * 2);
		m_freqs1 = std::vector<REALTYPE>(nfreqs);
		m_freqs2 = std::vector<REALTYPE>(nfreqs);
		m_freqs3 = std::vector<REALTYPE>(nfreqs);
		m_fft = std::make_unique<FFT>(nfreqs*2);
		std::fill(m_insamples.begin(), m_insamples.end(), 0.0f);
		for (int i = 0; i < nfreqs; ++i)
		{
			for (int j = 0; j < numharmonics; ++j)
			{
				double oscgain = 1.0 - (1.0 / numharmonics)*j;
				m_insamples[i] += scaler * oscgain * sin(2 * c_PI / samplerate * i* (hz + hz * j));
			}
		}
	}
	
	//std::fill(m_freqs1.begin(), m_freqs1.end(), 0.0f);
	//std::fill(m_freqs2.begin(), m_freqs2.end(), 0.0f);
	//std::fill(m_freqs3.begin(), m_freqs3.end(), 0.0f);
	//std::fill(m_fft->freq.begin(), m_fft->freq.end(), 0.0f);
	for (int i = 0; i < nfreqs; ++i)
	{
		m_fft->smp[i] = m_insamples[i];
	}
	m_fft->applywindow(W_HAMMING);
	m_fft->smp2freq();
	double ratio = pow(2.0f, pars.pitch_shift.cents / 1200.0f);
	spectrum_do_pitch_shift(pars, nfreqs, m_fft->freq.data(), m_freqs2.data(), ratio);
	spectrum_do_freq_shift(pars, nfreqs, samplerate, m_freqs2.data(), m_freqs1.data());
	spectrum_do_compressor(pars, nfreqs, m_freqs1.data(), m_freqs2.data());
	spectrum_spread(nfreqs, samplerate, m_freqs3, m_freqs2.data(), m_freqs1.data(), pars.spread.bandwidth);
	//if (pars.harmonics.enabled)
	//	spectrum_do_harmonics(pars, m_freqs3, nfreqs, samplerate, m_freqs1.data(), m_freqs2.data());
	//else spectrum_copy(nfreqs, m_freqs1.data(), m_freqs2.data());
	Graphics g(m_img);
	g.fillAll(Colours::black);
	g.setColour(Colours::white);
	for (int i = 0; i < nfreqs; ++i)
	{
		double binfreq = (samplerate / 2 / nfreqs)*i;
		double xcor = jmap<double>(binfreq, 0.0, samplerate / 2.0, 0.0, getWidth());
		double ycor = getHeight()- jmap<double>(m_freqs2[i], 0.0, nfreqs/128, 0.0, getHeight());
		ycor = jlimit<double>(0.0, getHeight(), ycor);
		g.drawLine(xcor, getHeight(), xcor, ycor, 1.0);
	}
	double t1 = Time::getMillisecondCounterHiRes();
	m_elapsed = t1 - t0;
	repaint();
}

void SpectralVisualizer::paint(Graphics & g)
{
	g.drawImage(m_img, 0, 0, getWidth(), getHeight(), 0, 0, m_img.getWidth(), m_img.getHeight());
	g.setColour(Colours::yellow);
	g.drawText(String(m_elapsed, 1)+" ms", 1, 1, getWidth(), 30, Justification::topLeft);
}


SpectralChainEditor::SpectralChainEditor()
 : m_bgcolor(0xff0a0a0a), m_selbgcolor(0xff141f28), m_dragbgcolor(0xff1a1a1a)
{
    m_disabledImage = Drawable::createFromImageData(BinaryData::power_svg, BinaryData::power_svgSize);
    m_enabledImage = Drawable::createFromImageData(BinaryData::power_sel_svg, BinaryData::power_sel_svgSize);
}

void SpectralChainEditor::paint(Graphics & g)
{
	//g.fillAll(Colours::black);
    g.setColour(Colours::black);
    g.fillRoundedRectangle(getLocalBounds().toFloat(), 4.0f);

	if (m_src == nullptr)
		return;
	
    int xoff = 3;
    int yoff = 3;
	float box_w = (getWidth() - 2*xoff) / m_order.size();
	int box_h = getHeight();

    float box_margin = jmin( (box_w * 0.25f), 16.0f);
    float arrowsize = jmin(10.0f, box_margin*0.5f);

	for (int i = 0; i < m_order.size(); ++i)
	{
		//if (i!=m_cur_index)
			drawBox(g, i, i*box_w + xoff, yoff, box_w - box_margin, box_h - 2*yoff);
		if (i<m_order.size() - 1)
			g.drawArrow(juce::Line<float>(i*box_w + (box_w - box_margin) + xoff + 1, box_h / 2, i*box_w + box_w + xoff, box_h / 2), 2.0f, arrowsize, arrowsize);
	}
	if (m_drag_x>=0 && m_drag_x<getWidth() && m_cur_index>=0)
		drawBox(g, m_cur_index, m_drag_x - m_downoffset_x + 5, yoff, box_w - box_margin - box_margin/2, box_h - 2*yoff);
}

void SpectralChainEditor::setSource(StretchAudioSource * src)
{
	m_src = src;
	m_order = m_src->getSpectrumProcessOrder();
	repaint();
}

void SpectralChainEditor::mouseDown(const MouseEvent & ev)
{
	m_did_drag = false;
    int xoff = 3;
    int yoff = 3;
    float box_w = (getWidth() - 2*xoff) / (float)m_order.size();
	int box_h = getHeight();
	m_cur_index = (int) ((ev.x - xoff) / box_w);
	if (m_cur_index >= 0)
	{
        bool done = false;
        juce::Rectangle<float> r(box_w*m_cur_index + 3, 3, 15, 15);
		if (r.contains(ev.x - xoff, ev.y - yoff))
		{
			toggleBool(m_order[m_cur_index].m_enabled);
			repaint();
            done = true;
		}

        if (ModuleSelectedCallback)
            ModuleSelectedCallback(m_order[m_cur_index].m_index);

        if (done) return;
    }
    m_drag_x = ev.x;
    m_downoffset_x = ev.x - xoff - box_w*m_cur_index;
	repaint();
}

void SpectralChainEditor::mouseDrag(const MouseEvent & ev)
{
    int xoff = 3;
    int yoff = 3;
    float box_w = (getWidth() - 2*xoff) / (float)m_order.size();
    juce::Rectangle<float> r(box_w*m_cur_index + 3, 3, 15, 15);
    if (r.contains(ev.x - xoff, ev.y - yoff))
        return;
    if (m_cur_index >= 0 && m_cur_index < m_order.size())
	{
		
		int box_h = getHeight();
		int new_index = (ev.x - xoff) / box_w;
		if (new_index >= 0 && new_index < m_order.size() && new_index != m_cur_index)
		{
			swapSpectrumProcesses(m_order[m_cur_index], m_order[new_index]);
			
			m_cur_index = new_index;
			m_did_drag = true;
			m_src->setSpectrumProcessOrder(m_order);
			if (ModuleOrderOrEnabledChangedCallback)
				ModuleOrderOrEnabledChangedCallback();
		}
		int diff = m_drag_x - ev.x;
		m_drag_x -= diff;
		repaint();
	}
}

void SpectralChainEditor::mouseUp(const MouseEvent & ev)
{
	m_drag_x = -1;
	//m_cur_index = -1;
	repaint();
}

void SpectralChainEditor::setModuleSelected(int id)
{
	if (id != m_cur_index)
	{
		m_cur_index = id;
		repaint();
	}
}

void SpectralChainEditor::moveModule(int old_id, int new_id)
{
	if (old_id == m_cur_index)
		return;
	std::swap(m_order[old_id], m_order[new_id]);
	m_cur_index = new_id;
	m_src->setSpectrumProcessOrder(m_order);
	repaint();
	if (ModuleOrderOrEnabledChangedCallback)
		ModuleOrderOrEnabledChangedCallback();
}

void SpectralChainEditor::drawBox(Graphics & g, int index, int x, int y, int w, int h)
{
	jassert(m_order[index].m_enabled != nullptr);
	String txt;
    bool enabled = ((bool)*m_order[index].m_enabled);

	if (m_order[index].m_index == 0)
		txt = "Harmonics";
	if (m_order[index].m_index == 1)
		txt = "Tonal vs Noise";
	if (m_order[index].m_index == 2)
		txt = "Frequency shift";
	if (m_order[index].m_index == 3)
		txt = "Pitch shift";
	if (m_order[index].m_index == 4)
		txt = "Ratios";
	if (m_order[index].m_index == 5)
		txt = "Spread";
	if (m_order[index].m_index == 6)
		txt = "Filter";
	if (m_order[index].m_index == 8)
		txt = "Compressor";
    if (m_order[index].m_index == 7)
        txt = "Free filter";

    Colour bgcolor = enabled ? m_selbgcolor : m_bgcolor;

    if (index == m_cur_index)
	{
        bgcolor = bgcolor.brighter(0.1f);
	}

    g.setColour(bgcolor);
    g.fillRoundedRectangle(x, y, w, h, 4.0f);


    // outline
	g.setColour(Colour(0xccaaaaaa));
    g.drawRoundedRectangle(x, y, w, h, 4.0f, 1.0f);

    g.setColour(Colour(0xffaaaaaa));
    if (w > 10) {
        g.drawFittedText(txt, x + 2,y,w-4,h-4, Justification::centredBottom, 3);
    }

    auto enableRect = Rectangle<float>(x + 2, y + 2, 16, 16);

    if (enabled) {
        m_enabledImage->drawWithin(g, enableRect, RectanglePlacement::centred, 1.0f);
    } else {
        m_disabledImage->drawWithin(g, enableRect, RectanglePlacement::centred, 0.7f);
    }

    // for arrow color
	g.setColour(Colours::white.withAlpha(0.8f));
}

ParameterComponent::ParameterComponent(AudioProcessorParameter * par, bool notifyOnlyOnRelease, bool useDrawableToggle)
: m_par(par)
{
	addAndMakeVisible(&m_label);
	m_labeldefcolor = m_label.findColour(Label::textColourId);
	m_label.setText(par->getName(50), dontSendNotification);
    m_label.setJustificationType(Justification::centredRight);
    m_label.setFont(16.0f);

	AudioParameterFloat* floatpar = dynamic_cast<AudioParameterFloat*>(par);
	if (floatpar)
	{
		m_slider = XenUtils::makeAddAndMakeVisible<MySlider>(*this,&floatpar->range);
		m_notify_only_on_release = notifyOnlyOnRelease;
		m_slider->setRange(floatpar->range.start, floatpar->range.end, floatpar->range.interval);
		m_slider->setValue(*floatpar, dontSendNotification);
        m_slider->setTextBoxStyle(Slider::TextBoxLeft, false, 60, 34);
		m_slider->addListener(this);
		m_slider->setDoubleClickReturnValue(true, floatpar->range.convertFrom0to1(par->getDefaultValue()));
        m_slider->setViewportIgnoreDragFlag(true);
        m_slider->setScrollWheelEnabled(false);
        m_slider->setTitle(floatpar->getName(50));

	}
	AudioParameterInt* intpar = dynamic_cast<AudioParameterInt*>(par);
	if (intpar)
	{
		m_slider = XenUtils::makeAddAndMakeVisible<MySlider>(*this);
		m_notify_only_on_release = notifyOnlyOnRelease;
		m_slider->setRange(intpar->getRange().getStart(), intpar->getRange().getEnd(), 1.0);
		m_slider->setValue(*intpar, dontSendNotification);
        m_slider->setTextBoxStyle(Slider::TextBoxLeft, false, 60, 34);
		m_slider->addListener(this);
        m_slider->setViewportIgnoreDragFlag(true);
        m_slider->setScrollWheelEnabled(false
                                        );
        m_slider->setTitle(intpar->getName(50));
	}
	AudioParameterChoice* choicepar = dynamic_cast<AudioParameterChoice*>(par);
	if (choicepar)
	{

	}
	AudioParameterBool* boolpar = dynamic_cast<AudioParameterBool*>(par);
	if (boolpar)
	{
        if (useDrawableToggle) {
            m_drawtogglebut = std::make_unique<DrawableButton>("but", DrawableButton::ImageFitted);
            m_drawtogglebut->setToggleState(*boolpar, dontSendNotification);
            m_drawtogglebut->addListener(this);
            m_drawtogglebut->setTitle(par->getName(50));
            m_drawtogglebut->setTooltip(par->getName(50));
            m_drawtogglebut->setClickingTogglesState(true);
            m_drawtogglebut->setColour(TextButton::buttonColourId, Colours::transparentBlack);
            m_drawtogglebut->setColour(TextButton::buttonOnColourId, Colours::transparentBlack);
            m_drawtogglebut->setColour(DrawableButton::backgroundColourId, Colours::transparentBlack);
            m_drawtogglebut->setColour(DrawableButton::backgroundOnColourId, Colours::transparentBlack);

            addAndMakeVisible(m_drawtogglebut.get());
        }
        else {
            m_togglebut = std::make_unique<ToggleButton>();
            m_togglebut->setToggleState(*boolpar, dontSendNotification);
            m_togglebut->addListener(this);
            m_togglebut->setButtonText(par->getName(50));
            addAndMakeVisible(m_togglebut.get());
        }
	}
}

void ParameterComponent::resized()
{
    int h = getHeight();
	if (m_slider)
	{
		//int labw = 200;
        int labw = 120;
        if (getWidth() < 280) {
            labw = 60;
            m_label.setFont(12.0f);
        }
        else if (getWidth() < 350) {
            labw = 100;
            m_label.setFont(14.0f);
        } else {
            m_label.setFont(16.0f);
        }
		m_label.setBounds(0, 0, labw, h);
		m_slider->setBounds(m_label.getRight() + 1, 0, getWidth() - 2 - m_label.getWidth(), h);
	}
    else if (m_togglebut) {
		m_togglebut->setBounds(1, 0, getWidth() - 1, h);
    }
    else if (m_drawtogglebut) {
        m_drawtogglebut->setBounds(1, 0, getWidth() - 1, h);
    }

}

void ParameterComponent::sliderValueChanged(Slider * slid)
{
	if (m_notify_only_on_release == true)
		return;
	AudioParameterFloat* floatpar = dynamic_cast<AudioParameterFloat*>(m_par);
	if (floatpar != nullptr)
		*floatpar = slid->getValue();
	AudioParameterInt* intpar = dynamic_cast<AudioParameterInt*>(m_par);
	if (intpar != nullptr)
		*intpar = slid->getValue();
}

void ParameterComponent::sliderDragStarted(Slider * slid)
{
	m_dragging = true;
}

void ParameterComponent::sliderDragEnded(Slider * slid)
{
	m_dragging = false;
	if (m_notify_only_on_release == false)
		return;
	AudioParameterFloat* floatpar = dynamic_cast<AudioParameterFloat*>(m_par);
	if (floatpar != nullptr)
		*floatpar = slid->getValue();
	AudioParameterInt* intpar = dynamic_cast<AudioParameterInt*>(m_par);
	if (intpar != nullptr)
		*intpar = slid->getValue();
}

void ParameterComponent::buttonClicked(Button * but)
{
	AudioParameterBool* boolpar = dynamic_cast<AudioParameterBool*>(m_par);
	if (m_togglebut != nullptr)
	{
		if (m_togglebut->getToggleState()!=*boolpar)
			*boolpar = m_togglebut->getToggleState();
	}
    else if (m_drawtogglebut != nullptr)
    {
        if (m_drawtogglebut->getToggleState() != *boolpar)
            *boolpar = m_drawtogglebut->getToggleState();
    }
}

void ParameterComponent::updateComponent()
{
	AudioParameterFloat* floatpar = dynamic_cast<AudioParameterFloat*>(m_par);
	if (floatpar != nullptr && m_slider != nullptr && m_dragging == false && (float)m_slider->getValue() != *floatpar)
	{
		m_slider->setValue(*floatpar, dontSendNotification);
	}
	AudioParameterInt* intpar = dynamic_cast<AudioParameterInt*>(m_par);
	if (intpar != nullptr && m_slider != nullptr && m_dragging == false && (int)m_slider->getValue() != *intpar)
	{
		m_slider->setValue(*intpar, dontSendNotification);
	}
	AudioParameterBool* boolpar = dynamic_cast<AudioParameterBool*>(m_par);
    if (boolpar!=nullptr) {
        if ( m_togglebut != nullptr)
        {
            if (m_togglebut->getToggleState() != *boolpar)
                m_togglebut->setToggleState(*boolpar, dontSendNotification);
        }
        else if ( m_drawtogglebut != nullptr)
        {
            if (m_drawtogglebut->getToggleState() != *boolpar)
                m_drawtogglebut->setToggleState(*boolpar, dontSendNotification);
        }
    }
}

void ParameterComponent::setHighLighted(bool b)
{
	if (b == false)
	{
		m_label.setColour(Label::textColourId, m_labeldefcolor);
		if (m_togglebut)
			m_togglebut->setColour(ToggleButton::textColourId, m_labeldefcolor);
        //else if (m_drawtogglebut)
        //    m_drawtogglebut->setColour(ToggleButton::textColourId, m_labeldefcolor);
	}
	else
	{
		m_label.setColour(Label::textColourId, Colours::yellow);
		if (m_togglebut)
			m_togglebut->setColour(ToggleButton::textColourId, Colours::yellow);
        //else if (m_drawtogglebut)
        //    m_drawtogglebut->setColour(ToggleButton::textColourId, Colours::yellow);
	}
}

MySlider::MySlider(NormalisableRange<float>* range) : m_range(range)
{
}

double MySlider::proportionOfLengthToValue(double x)
{
	if (m_range)
		return m_range->convertFrom0to1(x);
	return Slider::proportionOfLengthToValue(x);
}

double MySlider::valueToProportionOfLength(double x)
{
	if (m_range)
		return m_range->convertTo0to1(x);
	return Slider::valueToProportionOfLength(x);
}

PerfMeterComponent::PerfMeterComponent(PaulstretchpluginAudioProcessor * p) 
	: m_proc(p) 
{
    m_gradient.isRadial = false;
    m_gradient.addColour(0.0, Colours::red);
    m_gradient.addColour(0.25, Colours::yellow);
    m_gradient.addColour(1.0, Colours::green);
	startTimer(30);
}

void PerfMeterComponent::paint(Graphics & g)
{
    m_gradient.point1 = {0.0f,0.0f};
    m_gradient.point2 = {(float)getWidth(),0.0f};
    g.fillAll(Colours::grey);
	double amt = m_proc->getPreBufferingPercent();

    if (enabled) {
        g.setColour(Colours::green.withAlpha(0.8f));
    } else {
        g.setColour(Colours::darkgrey.withAlpha(0.8f));
    }

	int w = amt * getWidth();
    //g.setGradientFill(m_gradient);
    g.fillRect(0, 0, w, getHeight());
	g.setColour(Colours::white.withAlpha(0.4f));
	g.drawRect(0, 0, getWidth(), getHeight());
    g.setColour(Colours::white);
	g.setFont(10.0f);
	if (m_proc->getPreBufferAmount()>0)
		g.drawText("PREBUFFER", 0, 0, getWidth(), getHeight(), Justification::centred);
	else
		g.drawText("NO PREBUFFER", 0, 0, getWidth(), getHeight(), Justification::centred);
}

void PerfMeterComponent::mouseDown(const MouseEvent & ev)
{
	PopupMenu bufferingmenu;
	int curbufamount = m_proc->getPreBufferAmount();
	bufferingmenu.addItem(100, "None", true, curbufamount == -1);
	bufferingmenu.addItem(101, "Small", true, curbufamount == 1);
	bufferingmenu.addItem(102, "Medium", true, curbufamount == 2);
	bufferingmenu.addItem(103, "Large", true, curbufamount == 3);
	bufferingmenu.addItem(104, "Very large", true, curbufamount == 4);
	bufferingmenu.addItem(105, "Huge", true, curbufamount == 5);

    auto opts = PopupMenu::Options().withTargetComponent(this);
    if (!JUCEApplicationBase::isStandaloneApp()) {
        if (auto * editor = findParentComponentOfClass<PaulstretchpluginAudioProcessorEditor>()) {
            opts = opts.withParentComponent(editor);
        }
    }
#if JUCE_IOS
    opts = opts.withStandardItemHeight(34);
#endif

    bufferingmenu.showMenuAsync(opts, [this](int r) {
        if (r >= 100 && r < 200)
        {
            if (r == 100)
                m_proc->m_use_backgroundbuffering = false;
            if (r > 100)
                m_proc->setPreBufferAmount(r - 100);
        }
    });
}

void PerfMeterComponent::timerCallback()
{
	repaint();
}

void zoom_scrollbar::mouseDown(const MouseEvent &e)
{
	m_drag_start_x = e.x;
}

void zoom_scrollbar::mouseDoubleClick (const MouseEvent&)
{
    // reset
    m_therange.setStart(0.0);
    m_therange.setEnd(1.0);
    repaint();

    if (RangeChanged)
        RangeChanged(m_therange);
}


void zoom_scrollbar::mouseMove(const MouseEvent &e)
{
	auto ha = get_hot_area(e.x, e.y);
	if (ha == ha_left_edge || m_hot_area == ha_right_edge)
		setMouseCursor(MouseCursor::LeftRightResizeCursor);
	else
		setMouseCursor(MouseCursor::NormalCursor);
	if (ha != m_hot_area)
	{
		m_hot_area = ha;
		repaint();
	}
}

void zoom_scrollbar::mouseDrag(const MouseEvent &e)
{
	if (m_hot_area == ha_none)
		return;
	if (m_hot_area == ha_left_edge)
	{
		double new_left_edge = 1.0 / getWidth()*e.x;
		m_therange.setStart(jlimit(0.0, m_therange.getEnd() - 0.01, new_left_edge));
		repaint();
	}
	if (m_hot_area == ha_right_edge)
	{
		double new_right_edge = 1.0 / getWidth()*e.x;
		m_therange.setEnd(jlimit(m_therange.getStart() + 0.01, 1.0, new_right_edge));
		repaint();
	}
	if (m_hot_area == ha_handle)
	{
		double delta = 1.0 / getWidth()*(e.x - m_drag_start_x);
		//double old_start = m_start;
		//double old_end = m_end;
		double old_len = m_therange.getLength();
		m_therange.setStart(jlimit(0.0, 1.0 - old_len, m_therange.getStart() + delta));
		m_therange.setEnd(jlimit(old_len, m_therange.getStart() + old_len, m_therange.getEnd() + delta));
		m_drag_start_x = e.x;
		repaint();
	}
	if (RangeChanged)
		RangeChanged(m_therange);
}

void zoom_scrollbar::mouseEnter(const MouseEvent & event)
{
	m_hot_area = get_hot_area(event.x, event.y);
	repaint();
}

void zoom_scrollbar::mouseExit(const MouseEvent &)
{
	m_hot_area = ha_none;
	repaint();
}

void zoom_scrollbar::paint(Graphics &g)
{
	g.setColour(Colours::darkgrey);
	g.fillRect(0, 0, getWidth(), getHeight());
	int x0 = (int)(getWidth()*m_therange.getStart());
	int x1 = (int)(getWidth()*m_therange.getEnd());
	if (m_hot_area != ha_none)
		g.setColour(Colours::white.withAlpha(0.8f));
	else g.setColour(Colours::grey);
	//g.fillRect(x0, 0, x1 - x0, getHeight());
    g.fillRoundedRectangle(x0, 0, x1 - x0, getHeight(), 8.0f);

}

void zoom_scrollbar::setRange(Range<double> rng, bool docallback)
{
	if (rng.isEmpty())
		return;
	m_therange = rng.constrainRange({ 0.0,1.0 });
	if (RangeChanged && docallback)
		RangeChanged(m_therange);
	repaint();
}

zoom_scrollbar::hot_area zoom_scrollbar::get_hot_area(int x, int)
{
    int radius = 10;
#if JUCE_IOS
    radius *= 2;
#endif
	int x0 = (int)(getWidth()*m_therange.getStart());
	int x1 = (int)(getWidth()*m_therange.getEnd());
	if (is_in_range(x, x0 - radius, x0 + radius))
		return ha_left_edge;
	if (is_in_range(x, x1 - radius, x1 + radius))
		return ha_right_edge;
	if (is_in_range(x, x0 + radius, x1 - radius))
		return ha_handle;
	return ha_none;
}

RatioMixerEditor::RatioMixerEditor(int numratios)
{
	for (int i = 0; i < numratios; ++i)
	{
		auto ratslid = std::make_unique<Slider>(Slider::LinearHorizontal,Slider::TextBoxBelow);
		ratslid->setRange(0.125, 8.0);
		ratslid->onValueChange = [this,i]() {OnRatioChanged(i, m_ratio_sliders[i]->getValue()); };
        ratslid->setNumDecimalPlacesToDisplay(3);
        addAndMakeVisible(ratslid.get());
		m_ratio_sliders.emplace_back(std::move(ratslid));
		
		auto ratlevslid = std::make_unique<Slider>();
		ratlevslid->setRange(0.0, 1.0);
        ratlevslid->setNumDecimalPlacesToDisplay(3);
		ratlevslid->setSliderStyle(Slider::LinearVertical);
		if (i==3)
			ratlevslid->setValue(1.0,dontSendNotification);
		else ratlevslid->setValue(0.0,dontSendNotification);
		ratlevslid->onValueChange = [this, i]() { OnRatioLevelChanged(i, m_ratio_level_sliders[i]->getValue()); };
		addAndMakeVisible(ratlevslid.get());
		m_ratio_level_sliders.emplace_back(std::move(ratlevslid));

        auto ratlab = std::make_unique<Label>();
        ratlab->setJustificationType(Justification::centred);
        ratlab->setText(String(i+1), dontSendNotification);
        addAndMakeVisible(ratlab.get());
        m_labels.emplace_back(std::move(ratlab));
	}

    if (numratios > 0) m_ratio_sliders[0]->setDoubleClickReturnValue(true, 0.25);
    if (numratios > 1) m_ratio_sliders[1]->setDoubleClickReturnValue(true, 0.5);
    if (numratios > 2) m_ratio_sliders[2]->setDoubleClickReturnValue(true, 1.0);
    if (numratios > 3) m_ratio_sliders[3]->setDoubleClickReturnValue(true, 2.0);
    if (numratios > 4) m_ratio_sliders[4]->setDoubleClickReturnValue(true, 3.0);
    if (numratios > 5) m_ratio_sliders[5]->setDoubleClickReturnValue(true, 4.0);
    if (numratios > 6) m_ratio_sliders[6]->setDoubleClickReturnValue(true, 1.5);
    if (numratios > 7) m_ratio_sliders[7]->setDoubleClickReturnValue(true, 2/3.0);

	startTimer(200);
	setOpaque(true);
}

void RatioMixerEditor::resized()
{
    int minslidw = 65;
    int maxslidw = 120;
    int minslidh = 55;
    int minrslidh = 32;
    int maxrslidh = 45;
    FlexBox contentbox;
    contentbox.flexDirection = FlexBox::Direction::row;
    contentbox.flexWrap = FlexBox::Wrap::wrap;
    //contentbox.alignContent = FlexBox::AlignContent::flexStart;

    std::vector<FlexBox> itemboxes;
    itemboxes.resize(m_ratio_sliders.size());

	int nsliders = (int) m_ratio_sliders.size();
	int slidw = getWidth() / nsliders;

    for (int i = 0; i < nsliders; ++i)
	{
        itemboxes[i].flexDirection = FlexBox::Direction::column;
        itemboxes[i].items.add(FlexItem(minslidw, minslidh, *m_ratio_level_sliders[i]).withFlex(1));
        itemboxes[i].items.add(FlexItem(minslidw, minrslidh, *m_ratio_sliders[i]).withFlex(1).withMaxHeight(maxrslidh));

        contentbox.items.add(FlexItem(minslidw, minslidh + minrslidh, itemboxes[i]).withMargin(1).withFlex(1).withMaxWidth(maxslidw));
	}

    contentbox.performLayout(getLocalBounds().reduced(1));

    for (int i = 0; i < nsliders; ++i)
    {
        m_labels[i]->setBounds(m_ratio_level_sliders[i]->getX(), m_ratio_level_sliders[i]->getY() + 1, m_ratio_level_sliders[i]->getWidth() - 2 , 16);
    }
}

void RatioMixerEditor::timerCallback()
{
	if (!GetParameterValue)
		return;
	for (int i = 0; i < m_ratio_level_sliders.size(); ++i)
	{
		double v = GetParameterValue(0, i);
		if (v!=m_ratio_sliders[i]->getValue())
			m_ratio_sliders[i]->setValue(v, dontSendNotification);
		v = GetParameterValue(1, i);
		if (v!=m_ratio_level_sliders[i]->getValue())
			m_ratio_level_sliders[i]->setValue(v, dontSendNotification);
	}
}

void RatioMixerEditor::paint(Graphics & g)
{
	g.fillAll(Colour(0xff222222));
    /*
    g.setColour(Colours::white);
	auto nsliders = m_ratio_sliders.size();
	int slidw = getWidth() / nsliders;
	for (int i = 0; i < 8; ++i)
		g.drawText(String(i + 1), slidw / 2 + slidw * i - 8, 1, 15, 15, Justification::centred);
     */
}

FreeFilterComponent::FreeFilterComponent(PaulstretchpluginAudioProcessor* proc) 
	: m_env(proc->getStretchSource()->getMutex()), m_cs(proc->getStretchSource()->getMutex()), m_proc(proc)
{
    m_viewport = std::make_unique<Viewport>();
    m_viewport->setViewedComponent(&m_container, false);
    addAndMakeVisible(m_viewport.get());

	addAndMakeVisible(m_env);
	const auto& pars = m_proc->getParameters();
	m_parcomps.emplace_back(std::make_unique<ParameterComponent>(pars[cpi_freefilter_shiftx],false));
	m_container.addAndMakeVisible(m_parcomps.back().get());
	m_parcomps.emplace_back(std::make_unique<ParameterComponent>(pars[cpi_freefilter_shifty], false));
    m_container.addAndMakeVisible(m_parcomps.back().get());
	m_parcomps.emplace_back(std::make_unique<ParameterComponent>(pars[cpi_freefilter_scaley], false));
    m_container.addAndMakeVisible(m_parcomps.back().get());
	m_parcomps.emplace_back(std::make_unique<ParameterComponent>(pars[cpi_freefilter_tilty], false));
    m_container.addAndMakeVisible(m_parcomps.back().get());
	m_parcomps.emplace_back(std::make_unique<ParameterComponent>(pars[cpi_freefilter_randomy_numbands], false));
    m_container.addAndMakeVisible(m_parcomps.back().get());
	m_parcomps.emplace_back(std::make_unique<ParameterComponent>(pars[cpi_freefilter_randomy_rate], false));
    m_container.addAndMakeVisible(m_parcomps.back().get());
	m_parcomps.emplace_back(std::make_unique<ParameterComponent>(pars[cpi_freefilter_randomy_amount], false));
    m_container.addAndMakeVisible(m_parcomps.back().get());
}

void FreeFilterComponent::resized()
{
    int minslidwidth = 230;
    int slidh = 24;
    int margin = 1;

#if JUCE_IOS
    slidh = 28;
#endif

    FlexBox mainbox;

    FlexBox slidbox;
    slidbox.flexDirection = FlexBox::Direction::column;

    slidbox.items.add(FlexItem(3, 2).withMargin(0));


    for (int i = 0; i < m_parcomps.size(); ++i)
    {
        //m_parcomps[i]->setBounds(1, 1+25*i, m_slidwidth - 2, 24);
        slidbox.items.add(FlexItem(minslidwidth, slidh, *m_parcomps[i]).withMargin(margin).withFlex(0));
    }

    int minh = 0;
    for (auto & item : slidbox.items ) {
        minh += item.minHeight + item.margin.top + item.margin.bottom;
    }

    int vpminh = 3*slidh + 3*margin;

    if (minslidwidth < getWidth() * 0.4) {

        mainbox.flexDirection = FlexBox::Direction::row;

        //m_env.setBounds(m_slidwidth, 0, getWidth() - m_slidwidth, getHeight());



        mainbox.items.add(FlexItem(minslidwidth, vpminh, *m_viewport).withMargin(0).withFlex(1).withMaxWidth(m_slidwidth));
        mainbox.items.add(FlexItem(100, 50, m_env).withMargin(0).withFlex(3));
    }
    else {
        mainbox.flexDirection = FlexBox::Direction::column;

        //m_env.setBounds(m_slidwidth, 0, getWidth() - m_slidwidth, getHeight());

        slidbox.flexDirection = FlexBox::Direction::column;
        slidbox.flexWrap = FlexBox::Wrap::wrap;


        mainbox.items.add(FlexItem(minslidwidth, vpminh, *m_viewport).withMargin(0).withFlex(0));
        mainbox.items.add(FlexItem(100, 3).withMargin(0));
        mainbox.items.add(FlexItem(100, 50, m_env).withMargin(0).withFlex(3));
    }

    mainbox.performLayout(getLocalBounds());

    auto contw =  m_viewport->getWidth() - (minh > m_viewport->getHeight() ? m_viewport->getScrollBarThickness() : 0);
    auto contbounds = Rectangle<int>(0, 0, contw, minh);
    m_container.setBounds(contbounds);
    slidbox.performLayout(contbounds);

}

void FreeFilterComponent::paint(Graphics & g)
{
	g.setColour(Colour(0xff222222));

	g.fillRect(0, 0, getWidth(), getHeight());
}

void FreeFilterComponent::updateParameterComponents()
{
	for (auto& e : m_parcomps)
		e->updateComponent();
}

ParameterGroupComponent::ParameterGroupComponent(const String & name_, int groupid, PaulstretchpluginAudioProcessor* proc, bool showtoggle)
:name(name_), groupId(groupid), m_proc(proc), m_bgcolor(0xff1a1a1a), m_selbgcolor(0xff141f28)
{
    if (name_.isNotEmpty()) {
        m_namelabel = std::make_unique<Label>("name", name);
        addAndMakeVisible(m_namelabel.get());
    }

    if (showtoggle) {
        //m_enableButton = std::make_unique<DrawableButton>("ena", DrawableButton::ImageFitted);
        //m_enableButton = std::make_unique<ToggleButton>();
        //m_enableButton->setColour(DrawableButton::backgroundOnColourId, Colours::blue);
        m_enableButton = std::make_unique<DrawableButton>("reven", DrawableButton::ButtonStyle::ImageFitted);
        std::unique_ptr<Drawable> powerimg(Drawable::createFromImageData(BinaryData::power_svg, BinaryData::power_svgSize));
        std::unique_ptr<Drawable> powerselimg(Drawable::createFromImageData(BinaryData::power_sel_svg, BinaryData::power_sel_svgSize));
        m_enableButton->setImages(powerimg.get(), nullptr, nullptr, nullptr, powerselimg.get());
        m_enableButton->setClickingTogglesState(true);
        m_enableButton->setColour(TextButton::buttonColourId, Colours::transparentBlack);
        m_enableButton->setColour(TextButton::buttonOnColourId, Colours::transparentBlack);
        m_enableButton->setColour(DrawableButton::backgroundColourId, Colours::transparentBlack);
        m_enableButton->setColour(DrawableButton::backgroundOnColourId, Colours::transparentBlack);

        m_enableButton->onClick = [this]() {
            if (groupId < 0) {
                if (EnabledChangedCallback)
                    EnabledChangedCallback();
            }
            else {
                auto order = m_proc->getStretchSource()->getSpectrumProcessOrder();
                for (int i=0; i < order.size(); ++i) {
                    if (order[i].m_index == groupId) {
                        toggleBool(order[i].m_enabled);
                        m_enableButton->setToggleState(order[i].m_enabled->get(), dontSendNotification);
                        if (EnabledChangedCallback)
                            EnabledChangedCallback();
                        break;
                    }
                }
            }
        };
        addAndMakeVisible(m_enableButton.get());
    }

}

int ParameterGroupComponent::getMinimumHeight(int forWidth)
{
    if (m_lastForWidth != forWidth || m_lastCompSize != m_parcomps.size()) {
        m_minHeight = doLayout(Rectangle<int>(0,0, forWidth, 2000));

        m_lastForWidth = forWidth;
        m_lastCompSize = (int) m_parcomps.size();
    }

    return m_minHeight;
}

void ParameterGroupComponent::addParameterComponent(ParameterComponent * pcomp)
{
    if (pcomp) {
        addAndMakeVisible(pcomp);
        m_parcomps.push_back(pcomp);
        //m_parcomps.emplace_back(pcomp);
    }
}

void ParameterGroupComponent::replaceParameterComponent(ParameterComponent * oldcomp, ParameterComponent * newcomp)
{
    for (int i = 0; i < m_parcomps.size(); ++i)
    {
        if (m_parcomps[i] == oldcomp) {
            removeChildComponent(oldcomp);
            addAndMakeVisible(newcomp);
            m_parcomps[i] = newcomp;
            resized();
            break;
        }
    }
}


int ParameterGroupComponent::doLayout(Rectangle<int> bounds)
{
    int titlew = m_namelabel ? 100 : m_enableButton ? 40 : 0;
    int enablew = m_enableButton ? 40 : 0;
    int enablemaxh = 34;
    int minitemw = 260;
    int minitemh = 26;
    int margin = 1;
    int outsidemargin = 4;

#if JUCE_IOS
    minitemh = 34;
    outsidemargin = 4;
#endif


    FlexBox mainbox;
    mainbox.flexDirection = FlexBox::Direction::row;


    FlexBox contentbox;
    contentbox.flexDirection = FlexBox::Direction::row;
    contentbox.flexWrap = FlexBox::Wrap::wrap;
    contentbox.alignContent = FlexBox::AlignContent::flexStart;

    FlexBox titlebox;

    if (titlew > 0) {
        titlebox.flexDirection = FlexBox::Direction::row;
        //titlebox.items.add(FlexItem(4, minitemh));

        if (m_enableButton) {
            titlebox.items.add(FlexItem(enablew, minitemh, *m_enableButton).withMargin(margin).withMaxHeight(enablemaxh));
        }

        if (m_namelabel) {
            titlebox.items.add(FlexItem(titlew-enablew, minitemh, *m_namelabel).withMargin(margin).withFlex(1));
        }

        mainbox.items.add(FlexItem(titlew, enablemaxh, titlebox).withMargin(1)/*.withAlignSelf(FlexItem::AlignSelf::center)*/);
    }

    for (int i = 0; i < m_parcomps.size(); ++i)
    {
        contentbox.items.add(FlexItem(minitemw, minitemh, *m_parcomps[i]).withMargin(margin).withFlex(1));
    }

    mainbox.items.add(FlexItem(minitemw, minitemh, contentbox).withFlex(1).withMargin(outsidemargin));


    mainbox.performLayout(bounds);

    int minh = contentbox.items.size() > 0 ? contentbox.items.getLast().currentBounds.getBottom() + margin + outsidemargin : minitemh;

    return minh;
}

void ParameterGroupComponent::resized()
{

    doLayout(getLocalBounds());
}

void ParameterGroupComponent::paint(Graphics & g)
{
    if (m_enableButton && groupId >= 0 && m_enableButton->getToggleState()) {
        g.setColour(m_selbgcolor);
    } else {
        g.setColour(m_bgcolor);
    }
    g.fillRoundedRectangle(getLocalBounds().toFloat(), 4.0f);
    //g.fillRect(0, 0, getWidth(), getHeight());
}

void ParameterGroupComponent::updateParameterComponents()
{
    bool enabled = true;

    if (m_enableButton && groupId >= 0) {
        auto order = m_proc->getStretchSource()->getSpectrumProcessOrder();
        for (int i=0; i < order.size(); ++i) {
            if (order[i].m_index == groupId) {
                enabled = order[i].m_enabled->get();
                m_enableButton->setToggleState(enabled, dontSendNotification);
                m_enableButton->setAlpha(enabled ? 1.0f : 0.75f);
                break;
            }
        }
    }

    for (auto& e : m_parcomps) {
        e->updateComponent();
        e->setAlpha(enabled ? 1.0f : 0.5f);
    }
    repaint();
}

void AudioFilePreviewComponent::processBlock(double sr, AudioBuffer<float>& buf)
{
	if (m_reader != nullptr)
	{
		m_reader->read(&buf, 0, buf.getNumSamples(), m_playpos, true, true);
		m_playpos += buf.getNumSamples();
		if (m_playpos >= m_reader->lengthInSamples)
			m_playpos = 0;
	}
}

MyFileBrowserComponent::MyFileBrowserComponent(PaulstretchpluginAudioProcessor & p) :
	 m_filefilter(p.m_afm->getWildcardForAllFormats(),String(),String()), m_proc(p)
{
	String initiallocfn = m_proc.m_propsfile->m_props_file->getValue("importfilefolder",
		File::getSpecialLocation(File::userHomeDirectory).getFullPathName());
	File initialloc(initiallocfn);
	m_fbcomp = std::make_unique<FileBrowserComponent>(FileBrowserComponent::openMode | FileBrowserComponent::canSelectFiles,
		initialloc, &m_filefilter, nullptr);
	m_fbcomp->addListener(this);
	addAndMakeVisible(m_fbcomp.get());
	//setLookAndFeel(&m_filebwlookandfeel);
}

MyFileBrowserComponent::~MyFileBrowserComponent()
{
	setLookAndFeel(nullptr);
}

void MyFileBrowserComponent::resized()
{
	m_fbcomp->setBounds(0, 0, getWidth(), getHeight());
}

void MyFileBrowserComponent::paint(Graphics & g)
{
	g.fillAll(Colours::black.withAlpha(0.9f));
}

void MyFileBrowserComponent::selectionChanged()
{
}

void MyFileBrowserComponent::fileClicked(const File & file, const MouseEvent & e)
{
}

void MyFileBrowserComponent::fileDoubleClicked(const File & file)
{
	m_proc.setAudioFile(URL(file));
	m_proc.m_propsfile->m_props_file->setValue("importfilefolder", file.getParentDirectory().getFullPathName());
}

void MyFileBrowserComponent::browserRootChanged(const File & newRoot)
{
	m_proc.m_propsfile->m_props_file->setValue("importfilefolder", newRoot.getFullPathName());
}
