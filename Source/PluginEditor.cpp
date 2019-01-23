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

static void handleSettingsMenuModalCallback(int choice, PaulstretchpluginAudioProcessorEditor* ed)
{
	ed->executeModalMenuAction(0,choice);
}

//==============================================================================
PaulstretchpluginAudioProcessorEditor::PaulstretchpluginAudioProcessorEditor(PaulstretchpluginAudioProcessor& p)
	: AudioProcessorEditor(&p),
	m_wavecomponent(p.m_afm,p.m_thumb.get(), p.getStretchSource()),
	processor(p), m_perfmeter(&p),
    m_free_filter_component(&p),
    m_wavefilter_tab(p.m_cur_tab_index),
	m_filefilter(p.m_afm->getWildcardForAllFormats(),String(),String())
{
	
	
	
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
	m_wavefilter_tab.setTabBarDepth(20);
    
    addAndMakeVisible(&m_perfmeter);
	
	addAndMakeVisible(&m_import_button);
	m_import_button.setButtonText("Show browser");
	m_import_button.onClick = [this]()
	{ 
		toggleFileBrowser();
	};
	
	addAndMakeVisible(&m_settings_button);
	m_settings_button.setButtonText("Settings...");
	m_settings_button.onClick = [this]() { showSettingsMenu(); };
	
	if (processor.wrapperType == AudioProcessor::wrapperType_Standalone)
	{
		addAndMakeVisible(&m_render_button);
		m_render_button.setButtonText("Render...");
		m_render_button.onClick = [this]() { showRenderDialog(); };
	}

	addAndMakeVisible(m_rewind_button);
	m_rewind_button.setButtonText("<<");
	m_rewind_button.onClick = [this]() 
	{
		*processor.getBoolParameter(cpi_rewind) = true;
		//processor.getStretchSource()->seekPercent(processor.getStretchSource()->getPlayRange().getStart());
	};

	addAndMakeVisible(&m_info_label);
	m_info_label.setJustificationType(Justification::centredRight);

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
		int group_id = -1;
		if (i == cpi_harmonicsbw || i == cpi_harmonicsfreq || i == cpi_harmonicsgauss || i == cpi_numharmonics)
			group_id = 0;
		if (i == cpi_octavesm2 || i == cpi_octavesm1 || i == cpi_octaves0 || i == cpi_octaves1 || i == cpi_octaves15 ||
			i == cpi_octaves2 || i==cpi_octaves_extra1 || i==cpi_octaves_extra2)
			group_id = -2; // -2 for not included in the main parameters page
		if (i >= cpi_octaves_ratio0 && i <= cpi_octaves_ratio7)
			group_id = -2;
		if ((i >= cpi_enable_spec_module0 && i <= cpi_enable_spec_module8))
			group_id = -2;
		if (i == cpi_tonalvsnoisebw || i == cpi_tonalvsnoisepreserve)
			group_id = 1;
		if (i == cpi_filter_low || i == cpi_filter_high)
			group_id = 6;
		if (i == cpi_compress)
			group_id = 8;
		if (i == cpi_spreadamount)
			group_id = 5;
		if (i == cpi_frequencyshift)
			group_id = 2;
		if (i == cpi_pitchshift)
			group_id = 3;
		if (i == cpi_freefilter_scaley || i == cpi_freefilter_shiftx || i == cpi_freefilter_shifty ||
			i == cpi_freefilter_tilty || i == cpi_freefilter_randomy_amount || i == cpi_freefilter_randomy_numbands
			|| i == cpi_freefilter_randomy_rate)
			group_id = -2;
		if (group_id >= -1)
		{
			m_parcomps.emplace_back(std::make_unique<ParameterComponent>(pars[i], notifyonlyonrelease));
			m_parcomps.back()->m_group_id = group_id;
			if (i == cpi_capture_trigger)
				m_parcomps.back()->m_nonparamstate = &processor.m_is_recording;
			if (group_id >= -1)
				addAndMakeVisible(m_parcomps.back().get());
		}
		else
		{
			m_parcomps.push_back(nullptr);
		}
	}
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
	m_wavecomponent.ShowFileCacheRange = true;
	m_spec_order_ed.setSource(processor.getStretchSource());
	addAndMakeVisible(&m_spec_order_ed);
	m_spec_order_ed.ModuleSelectedCallback = [this](int id)
	{
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
	m_wavefilter_tab.addTab("Waveform", Colours::white, m_wave_container, true);
	m_wavefilter_tab.addTab("Ratio mixer", Colours::white, &m_ratiomixeditor, false);
	m_wavefilter_tab.addTab("Free filter", Colours::white, &m_free_filter_component, false);
	m_wavefilter_tab.addTab("Spectrum", Colours::white, &m_sonogram, false);

	addAndMakeVisible(&m_wavefilter_tab);
    setSize (1200, 320+14*25);
    startTimer(1, 100);
	startTimer(2, 1000);
	startTimer(3, 200);
	m_wavecomponent.startTimer(100);
	
}

PaulstretchpluginAudioProcessorEditor::~PaulstretchpluginAudioProcessorEditor()
{
	//Logger::writeToLog("PaulX Editor destroyed");
}

void PaulstretchpluginAudioProcessorEditor::showRenderDialog()
{
	auto content = new RenderSettingsComponent(&processor);
	content->setSize(content->getWidth(), content->getPreferredHeight());
	/*CallOutBox& myBox =*/ CallOutBox::launchAsynchronously(content, m_render_button.getBounds(), this);
}

void PaulstretchpluginAudioProcessorEditor::executeModalMenuAction(int menuid, int r)
{
	if (r >= 200 && r < 210)
	{
		int caplen = m_capturelens[r - 200];
		*processor.getFloatParameter(cpi_max_capture_len) = (float)caplen;
	}
	if (r == 1)
	{
		toggleBool(processor.m_play_when_host_plays);
	}
	if (r == 2)
	{
		toggleBool(processor.m_capture_when_host_plays);
	}
	if (r == 8)
	{
		toggleBool(processor.m_mute_while_capturing);
	}
	if (r == 4)
	{
		processor.resetParameters();
	}
	if (r == 5)
	{
		toggleBool(processor.m_load_file_with_state);
	}
	if (r == 9)
	{
		toggleBool(processor.m_save_captured_audio);
	}
	if (r == 3)
	{
		showAbout();
	}

	if (r == 6)
	{
		ValueTree tree = processor.getStateTree(true, true);
		MemoryBlock destData;
		MemoryOutputStream stream(destData, true);
		tree.writeToStream(stream);
		String txt = Base64::toBase64(destData.getData(), destData.getSize());
		SystemClipboard::copyTextToClipboard(txt);
	}
	if (r == 7)
	{
		toggleBool(processor.m_show_technical_info);
		processor.m_propsfile->m_props_file->setValue("showtechnicalinfo", processor.m_show_technical_info);
	}
}



void PaulstretchpluginAudioProcessorEditor::paint (Graphics& g)
{
	g.fillAll(Colours::darkgrey);
}

void PaulstretchpluginAudioProcessorEditor::resized()
{
	m_import_button.setBounds(1, 1, 60, 24);
	m_import_button.changeWidthToFitText();
	m_settings_button.setBounds(m_import_button.getRight() + 1, 1, 60, 24);
	m_settings_button.changeWidthToFitText();
	int yoffs = m_settings_button.getRight() + 1;
	if (processor.wrapperType == AudioProcessor::wrapperType_Standalone)
	{
		m_render_button.setBounds(yoffs, 1, 60, 24);
		m_render_button.changeWidthToFitText();
		yoffs = m_render_button.getRight() + 1;
	}
	m_rewind_button.setBounds(yoffs, 1, 30, 24);
	yoffs = m_rewind_button.getRight() + 1;
	m_perfmeter.setBounds(yoffs, 1, 150, 24);
	m_info_label.setBounds(m_perfmeter.getRight() + 1, m_settings_button.getY(),
		getWidth()- m_perfmeter.getRight()-1, 24);
	int w = getWidth();
	int xoffs = 1;
	yoffs = 30;
	int div = w / 6;
	//std::vector<std::vector<int>> layout;
	//layout.emplace_back(cpi_capture_enabled,	cpi_passthrough,	cpi_pause_enabled,	cpi_freeze);
	//layout.emplace_back(cpi_main_volume,		cpi_num_inchans,	cpi_num_outchans);
	m_parcomps[cpi_capture_trigger]->setBounds(xoffs, yoffs, div-1, 24);
	//xoffs += div;
	//m_parcomps[cpi_max_capture_len]->setBounds(xoffs, yoffs, div - 1, 24);
	xoffs += div;
	m_parcomps[cpi_passthrough]->setBounds(xoffs, yoffs, div - 1, 24);
	xoffs += div;
	m_parcomps[cpi_pause_enabled]->setBounds(xoffs, yoffs, div-1, 24);
	xoffs += div;
	m_parcomps[cpi_freeze]->setBounds(xoffs, yoffs, div - 1, 24);
	xoffs += div;
	m_parcomps[cpi_bypass_stretch]->setBounds(xoffs, yoffs, div - 1, 24);
	xoffs += div;
	m_parcomps[cpi_looping_enabled]->setBounds(xoffs, yoffs, div - 1, 24);
	xoffs = 1;
	yoffs += 25;
	div = w / 3;
	m_parcomps[cpi_main_volume]->setBounds(xoffs, yoffs, div-1, 24);
	xoffs += div;
	m_parcomps[cpi_num_inchans]->setBounds(xoffs, yoffs, div - 1, 24);
	xoffs += div;
	m_parcomps[cpi_num_outchans]->setBounds(xoffs, yoffs, div-1, 24);
	div = w / 2;
	xoffs = 1;
	yoffs += 25;
	m_parcomps[cpi_fftsize]->setBounds(xoffs, yoffs, div - 1, 24);
	xoffs += div;
	m_parcomps[cpi_stretchamount]->setBounds(xoffs, yoffs, div - 1, 24);
	m_parcomps[cpi_dryplayrate]->setBounds(xoffs, yoffs, div - 1, 24);
	xoffs = 1;
	yoffs += 25;
	m_parcomps[cpi_pitchshift]->setBounds(xoffs, yoffs, div - 1, 24);
	xoffs += div;
	m_parcomps[cpi_frequencyshift]->setBounds(xoffs, yoffs, div - 1, 24);
	xoffs = 1;
	yoffs += 25;
	m_parcomps[cpi_numharmonics]->setBounds(xoffs, yoffs, div - 1, 24);
	xoffs += div;
	m_parcomps[cpi_harmonicsfreq]->setBounds(xoffs, yoffs, div - 1, 24);
	xoffs = 1;
	yoffs += 25;
	m_parcomps[cpi_harmonicsbw]->setBounds(xoffs, yoffs, div - 1, 24);
	xoffs += div;
	m_parcomps[cpi_harmonicsgauss]->setBounds(xoffs, yoffs, div - 1, 24);
	xoffs = 1;
	yoffs += 25;
	m_parcomps[cpi_spreadamount]->setBounds(xoffs, yoffs, div - 1, 24);
	xoffs += div;
	m_parcomps[cpi_compress]->setBounds(xoffs, yoffs, div - 1, 24);
	xoffs = 1;
	yoffs += 25;
	m_parcomps[cpi_tonalvsnoisebw]->setBounds(xoffs, yoffs, div - 1, 24);
	xoffs += div;
	m_parcomps[cpi_tonalvsnoisepreserve]->setBounds(xoffs, yoffs, div - 1, 24);
	xoffs = 1;
	yoffs += 25;
	// filter here
	m_parcomps[cpi_filter_low]->setBounds(xoffs, yoffs, div - 1, 24);
	xoffs += div;
	m_parcomps[cpi_filter_high]->setBounds(xoffs, yoffs, div - 1, 24);
	
	xoffs = 1;
	yoffs += 25;
	
	m_parcomps[cpi_loopxfadelen]->setBounds(xoffs, yoffs, div - 1, 24);
	xoffs += div;
	m_parcomps[cpi_onsetdetection]->setBounds(xoffs, yoffs, div - 1, 24);
	xoffs = 1;
	yoffs += 25;
	m_parcomps[cpi_soundstart]->setBounds(xoffs, yoffs, div - 1, 24);
	xoffs += div;
	m_parcomps[cpi_soundend]->setBounds(xoffs, yoffs, div - 1, 24);
	//yoffs += 25;
	//xoffs = 1;
	
	yoffs += 25;
	int remain_h = getHeight() - 1 - yoffs;
	m_spec_order_ed.setBounds(1, yoffs, getWidth() - 2, remain_h / 9 * 1);
	m_wavefilter_tab.setBounds(1, m_spec_order_ed.getBottom() + 1, getWidth() - 2, remain_h / 9 * 8);
	m_wavecomponent.setBounds(m_wave_container->getX(), 0, m_wave_container->getWidth(),
		m_wave_container->getHeight()-16);
	m_zs.setBounds(m_wave_container->getX(), m_wavecomponent.getBottom(), m_wave_container->getWidth(), 15);
	//m_wavecomponent.setBounds(1, m_spec_order_ed.getBottom()+1, getWidth()-2, remain_h/5*4);
	
    
    
}

void PaulstretchpluginAudioProcessorEditor::timerCallback(int id)
{
	if (id == 1)
	{
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
		if (processor.m_show_technical_info)
		{
			infotext += String(processor.getStretchSource()->getDiskReadSampleCount()) + " ";
			infotext += String(processor.m_prepare_count)+" ";
            infotext += String(processor.getStretchSource()->m_param_change_count);
			infotext += " param changes ";
		}
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
		if (processor.m_show_technical_info)
			infotext += " " + String(m_wavecomponent.m_image_init_count) + " " + String(m_wavecomponent.m_image_update_count)+ " ";
		if (processor.m_offline_render_state >= 0 && processor.m_offline_render_state <= 100)
			infotext += String(processor.m_offline_render_state)+"%";
		if (processor.m_capture_save_state == 1)
			infotext += "Saving captured audio...";
		m_info_label.setText(infotext, dontSendNotification);
		
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
		m_parcomps[cpi_dryplayrate]->setVisible(*processor.getBoolParameter(cpi_bypass_stretch));
		m_parcomps[cpi_stretchamount]->setVisible(!(*processor.getBoolParameter(cpi_bypass_stretch)));
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
		File f(files[0]);
		processor.setAudioFile(f);
		toFront(true);
	}
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
    m_settings_menu.addItem(4, "Reset parameters", true, false);
	m_settings_menu.addItem(5, "Load file with plugin state", true, processor.m_load_file_with_state);
	m_settings_menu.addItem(1, "Play when host transport running", true, processor.m_play_when_host_plays);
	m_settings_menu.addItem(2, "Capture when host transport running", true, processor.m_capture_when_host_plays);
	m_settings_menu.addItem(8, "Mute audio while capturing", true, processor.m_mute_while_capturing);
	m_settings_menu.addItem(9, "Save captured audio to disk", true, processor.m_save_captured_audio);
	int capturelen = *processor.getFloatParameter(cpi_max_capture_len);
	PopupMenu capturelenmenu;
	
	for (int i=0;i<m_capturelens.size();++i)
		capturelenmenu.addItem(200+i, String(m_capturelens[i])+" seconds", true, capturelen == m_capturelens[i]);
	m_settings_menu.addSubMenu("Capture buffer length", capturelenmenu);
	
	m_settings_menu.addItem(3, "About...", true, false);
#ifdef JUCE_DEBUG
	m_settings_menu.addItem(6, "Dump preset to clipboard", true, false);
#endif
	m_settings_menu.addItem(7, "Show technical info", true, processor.m_show_technical_info);
	m_settings_menu.showMenuAsync(PopupMenu::Options(), 
		ModalCallbackFunction::forComponent(handleSettingsMenuModalCallback, this));
}

void PaulstretchpluginAudioProcessorEditor::showAbout()
{
	String fftlib = fftwf_version;
	String juceversiontxt = String("JUCE ") + String(JUCE_MAJOR_VERSION) + "." + String(JUCE_MINOR_VERSION);
	String title = g_plugintitle;
#ifdef JUCE_DEBUG
	title += " (DEBUG)";
#endif
	String vstInfo;
	if (processor.wrapperType == AudioProcessor::wrapperType_VST ||
		processor.wrapperType == AudioProcessor::wrapperType_VST3)
		vstInfo = "VST Plug-In Technology by Steinberg.\n\n";
	PluginHostType host;
	AlertWindow::showMessageBoxAsync(AlertWindow::InfoIcon,
		title,
		"Plugin for extreme time stretching and other sound processing\nBuilt on " + String(__DATE__) + " " + String(__TIME__) + "\n"
		"Copyright (C) 2006-2011 Nasca Octavian Paul, Tg. Mures, Romania\n"
		"(C) 2017-2019 Xenakios\n\n"+vstInfo+
		"Using " + fftlib + " for FFT\n\n"
		+ juceversiontxt + " (c) Roli. Used under the GPL license.\n\n"
		"GPL licensed source code for this plugin at : https://bitbucket.org/xenakios/paulstretchplugin/overview\n"
		"Running in : "+host.getHostDescription()+"\n"
		, "OK",
		this);

}

void PaulstretchpluginAudioProcessorEditor::toggleFileBrowser()
{
	if (m_filechooser == nullptr)
	{
		m_filechooser = std::make_unique<MyFileBrowserComponent>(processor);
		addChildComponent(m_filechooser.get());
	}
	m_filechooser->setBounds(0, 50, getWidth(), getHeight() - 60);
	m_filechooser->setVisible(!m_filechooser->isVisible());
	if (m_filechooser->isVisible())
		m_import_button.setButtonText("Hide browser");
	else
		m_import_button.setButtonText("Show browser");
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
	if (m_is_at_selection_drag_area)
		g.setColour(Colours::white.withAlpha(0.6f));
	else
		g.setColour(Colours::white.withAlpha(0.5f));

	double sel_len = m_time_sel_end - m_time_sel_start;
	//if (sel_len > 0.0 && sel_len < 1.0)
	{
		int xcorleft = normalizedToViewX<int>(m_time_sel_start); 
		int xcorright = normalizedToViewX<int>(m_time_sel_end);
		g.fillRect(xcorleft, m_topmargin, xcorright - xcorleft, getHeight() - m_topmargin);
	}
	if (m_file_cached.first.getLength() > 0.0 &&
		(bool)ShowFileCacheRange.getValue())
	{
		g.setColour(Colours::red.withAlpha(0.2f));
		int xcorleft = (int)jmap<double>(m_file_cached.first.getStart(), m_view_range.getStart(), m_view_range.getEnd(), 0, getWidth());
		int xcorright = (int)jmap<double>(m_file_cached.first.getEnd(), m_view_range.getStart(), m_view_range.getEnd(), 0, getWidth());
		g.fillRect(xcorleft, 0, xcorright - xcorleft, m_topmargin / 2);
		xcorleft = (int)jmap<double>(m_file_cached.second.getStart(), m_view_range.getStart(), m_view_range.getEnd(), 0, getWidth());
		xcorright = (int)jmap<double>(m_file_cached.second.getEnd(), m_view_range.getStart(), m_view_range.getEnd(), 0, getWidth());
		if (xcorright - xcorleft>0)
		{
			g.setColour(Colours::blue.withAlpha(0.2f));
			g.fillRect(xcorleft, m_topmargin / 2, xcorright - xcorleft, m_topmargin / 2);
		}
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
	g.drawText(GetFileCallback().getFileName(), 2, m_topmargin + 2, getWidth(), 20, Justification::topLeft);
	g.drawText(secondsToString2(thumblen), getWidth() - 200, m_topmargin + 2, 200, 20, Justification::topRight);
}

void WaveformComponent::timerCallback()
{
	if (m_sas->getLastSourcePosition() != m_last_source_pos)
	{
		m_last_source_pos = m_sas->getLastSourcePosition();
		m_last_source_pos_update_time = Time::getMillisecondCounterHiRes();
	}
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
	double pos = viewXToNormalized(e.x);
	if (e.y < m_topmargin || e.mods.isCommandDown())
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

void WaveformComponent::mouseUp(const MouseEvent & /*e*/)
{
	m_is_dragging_selection = false;
	m_lock_timesel_set = false;
	m_mousedown = false;
	m_didseek = false;
	if (m_didchangetimeselection)
	{
		TimeSelectionChangedCallback(Range<double>(m_time_sel_start, m_time_sel_end), 1);
		m_didchangetimeselection = false;
	}
}

void WaveformComponent::mouseDrag(const MouseEvent & e)
{
	if (m_didseek == true)
		return;
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
	return;
	/*
	double factor = 0.9;
	if (wd.deltaY < 0.0)
		factor = 1.11111;
	double normt = viewXToNormalized(e.x);
	double curlen = m_view_range.getLength();
	double newlen = curlen * factor;
	double oldt0 = m_view_range.getStart();
	double oldt1 = m_view_range.getEnd();
	double t0 = jlimit(0.0,1.0, normt + (curlen - newlen));
	double t1 = jlimit(0.0,1.0, t0+newlen);
	jassert(t1 > t0);
	m_view_range = { t0,t1 };
	//m_view_range = m_view_range.constrainRange({ 0.0, 1.0 });
	jassert(m_view_range.getStart() >= 0.0 && m_view_range.getEnd() <= 1.0);
	jassert(m_view_range.getLength() > 0.001);
	if (ViewRangeChangedCallback)
		ViewRangeChangedCallback(m_view_range);
	m_image_dirty = true;
	repaint();
	*/
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
	int xcorleft = (int)jmap<double>(m_time_sel_start, m_view_range.getStart(), m_view_range.getEnd(), 0, getWidth());
	int xcorright = (int)jmap<double>(m_time_sel_end, m_view_range.getStart(), m_view_range.getEnd(), 0, getWidth());
	if (juce::Rectangle<int>(xcorleft - 5, m_topmargin, 10, getHeight() - m_topmargin).contains(x, y))
		return 1;
	if (juce::Rectangle<int>(xcorright - 5, m_topmargin, 10, getHeight() - m_topmargin).contains(x, y))
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

void SpectralChainEditor::paint(Graphics & g)
{
	g.fillAll(Colours::black);
	if (m_src == nullptr)
		return;
	
	int box_w = getWidth() / m_order.size();
	int box_h = getHeight();
	for (int i = 0; i < m_order.size(); ++i)
	{
		//if (i!=m_cur_index)
			drawBox(g, i, i*box_w, 0, box_w - 20, box_h);
		if (i<m_order.size() - 1)
			g.drawArrow(juce::Line<float>(i*box_w + (box_w - 20), box_h / 2, i*box_w + box_w, box_h / 2), 2.0f, 12.0f, 12.0f);
	}
	if (m_drag_x>=0 && m_drag_x<getWidth() && m_cur_index>=0)
		drawBox(g, m_cur_index, m_drag_x, 0, box_w - 30, box_h);
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
    int box_w = getWidth() / m_order.size();
	int box_h = getHeight();
	m_cur_index = ev.x / box_w;
	if (m_cur_index >= 0)
	{
		if (ModuleSelectedCallback)
			ModuleSelectedCallback(m_order[m_cur_index].m_index);
		juce::Rectangle<int> r(box_w*m_cur_index, 1, 12, 12);
		if (r.contains(ev.x, ev.y))
		{
			toggleBool(m_order[m_cur_index].m_enabled);
			repaint();
            return;
		}
	}
	m_drag_x = ev.x;
	repaint();
}

void SpectralChainEditor::mouseDrag(const MouseEvent & ev)
{
    int box_w = getWidth() / m_order.size();
    juce::Rectangle<int> r(box_w*m_cur_index, 1, 12, 12);
    if (r.contains(ev.x, ev.y))
        return;
    if (m_cur_index >= 0 && m_cur_index < m_order.size())
	{
		
		int box_h = getHeight();
		int new_index = ev.x / box_w;
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
	if (index == m_cur_index)
	{
		g.setColour(Colours::darkgrey);
		//g.fillRect(i*box_w, 0, box_w - 30, box_h - 1);
		g.fillRect(x, y, w, h);
	}
	g.setColour(Colours::white);
	g.drawRect(x, y, w, h);
	g.drawFittedText(txt, x,y,w,h-5, Justification::centredBottom, 3);
	//g.drawFittedText(m_order[index].m_enabled->name, x, y, w, h, Justification::centred, 3);
	g.setColour(Colours::gold);
	g.drawRect(x + 2, y + 2, 12, 12);
	if ((bool)*m_order[index].m_enabled == true)
	{
		g.drawLine(x+2, y+2, x+14, y+14);
		g.drawLine(x+2, y+14, x+14, y+2);
	}
	g.setColour(Colours::white);
}

ParameterComponent::ParameterComponent(AudioProcessorParameter * par, bool notifyOnlyOnRelease) : m_par(par)
{
	addAndMakeVisible(&m_label);
	m_labeldefcolor = m_label.findColour(Label::textColourId);
	m_label.setText(par->getName(50), dontSendNotification);
	AudioParameterFloat* floatpar = dynamic_cast<AudioParameterFloat*>(par);
	if (floatpar)
	{
		m_slider = makeAddAndMakeVisible<MySlider>(&floatpar->range);
		m_notify_only_on_release = notifyOnlyOnRelease;
		m_slider->setRange(floatpar->range.start, floatpar->range.end, floatpar->range.interval);
		m_slider->setValue(*floatpar, dontSendNotification);
		m_slider->addListener(this);
		m_slider->setDoubleClickReturnValue(true, floatpar->range.convertFrom0to1(par->getDefaultValue()));
	}
	AudioParameterInt* intpar = dynamic_cast<AudioParameterInt*>(par);
	if (intpar)
	{
		m_slider = makeAddAndMakeVisible<MySlider>();
		m_notify_only_on_release = notifyOnlyOnRelease;
		m_slider->setRange(intpar->getRange().getStart(), intpar->getRange().getEnd(), 1.0);
		m_slider->setValue(*intpar, dontSendNotification);
		m_slider->addListener(this);
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
		m_togglebut->setButtonText(par->getName(50));
		addAndMakeVisible(m_togglebut.get());
	}
}

void ParameterComponent::resized()
{
	if (m_slider)
	{
		int labw = 200;
		if (getWidth() < 400)
			labw = 100;
		m_label.setBounds(0, 0, labw, 24);
		m_slider->setBounds(m_label.getRight() + 1, 0, getWidth() - 2 - m_label.getWidth(), 24);
	}
	if (m_togglebut)
		m_togglebut->setBounds(1, 0, getWidth() - 1, 24);
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
	if (m_togglebut != nullptr) // && m_togglebut->getToggleState() != *boolpar)
	{
		if (m_nonparamstate == nullptr)
		{
			if (m_togglebut->getToggleState()!=*boolpar)
				*boolpar = m_togglebut->getToggleState();
		}
		else
		{
			// If we have the non-parameter state pointer, just set the target parameter to true.
			// Logic in the AudioProcessor determines what should be done and it sets the parameter immediately back
			// to false when it sees the parameter is true.
			*boolpar = true;
		}
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
	if (m_togglebut != nullptr)
	{
		if (m_nonparamstate == nullptr)
		{
			if (m_togglebut->getToggleState() != *boolpar)
				m_togglebut->setToggleState(*boolpar, dontSendNotification);
		}
		else
		{
			// If we have the non-parameter state pointer, get the button toggle state from that
			if (m_togglebut->getToggleState()!=*m_nonparamstate)
				m_togglebut->setToggleState(*m_nonparamstate, dontSendNotification);
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
	}
	else
	{
		m_label.setColour(Label::textColourId, Colours::yellow);
		if (m_togglebut)
			m_togglebut->setColour(ToggleButton::textColourId, Colours::yellow);
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
	g.setColour(Colours::green);
	int w = amt * getWidth();
    //g.setGradientFill(m_gradient);
    g.fillRect(0, 0, w, getHeight());
	g.setColour(Colours::white);
	g.drawRect(0, 0, getWidth(), getHeight());
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
	int r = bufferingmenu.show();
	if (r >= 100 && r < 200)
	{
		if (r == 100)
			m_proc->m_use_backgroundbuffering = false;
		if (r > 100)
			m_proc->setPreBufferAmount(r - 100);
	}
}

void PerfMeterComponent::timerCallback()
{
	repaint();
}

void zoom_scrollbar::mouseDown(const MouseEvent &e)
{
	m_drag_start_x = e.x;
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
		g.setColour(Colours::white);
	else g.setColour(Colours::lightgrey);
	g.fillRect(x0, 0, x1 - x0, getHeight());
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
	int x0 = (int)(getWidth()*m_therange.getStart());
	int x1 = (int)(getWidth()*m_therange.getEnd());
	if (is_in_range(x, x0 - 5, x0 + 5))
		return ha_left_edge;
	if (is_in_range(x, x1 - 5, x1 + 5))
		return ha_right_edge;
	if (is_in_range(x, x0 + 5, x1 - 5))
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
		addAndMakeVisible(ratslid.get());
		m_ratio_sliders.emplace_back(std::move(ratslid));
		
		auto ratlevslid = std::make_unique<Slider>();
		ratlevslid->setRange(0.0, 1.0);
		ratlevslid->setSliderStyle(Slider::LinearVertical);
		if (i==3)
			ratlevslid->setValue(1.0,dontSendNotification);
		else ratlevslid->setValue(0.0,dontSendNotification);
		ratlevslid->onValueChange = [this, i]() { OnRatioLevelChanged(i, m_ratio_level_sliders[i]->getValue()); };
		addAndMakeVisible(ratlevslid.get());
		m_ratio_level_sliders.emplace_back(std::move(ratlevslid));
	}
	startTimer(200);
	setOpaque(true);
}

void RatioMixerEditor::resized()
{
	int nsliders = m_ratio_sliders.size();
	int slidw = getWidth() / nsliders;
	for (int i = 0; i < nsliders; ++i)
	{
		m_ratio_level_sliders[i]->setBounds(slidw/2+slidw * i-10, 15, 20, getHeight() - 55);
		m_ratio_sliders[i]->setBounds(slidw * i, getHeight() - 48, slidw - 5, 47);
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
	g.fillAll(Colours::grey);
	g.setColour(Colours::white);
	auto nsliders = m_ratio_sliders.size();
	int slidw = getWidth() / nsliders;
	for (int i = 0; i < 8; ++i)
		g.drawText(String(i + 1), slidw / 2 + slidw * i - 8, 1, 15, 15, Justification::centred);
}

FreeFilterComponent::FreeFilterComponent(PaulstretchpluginAudioProcessor* proc) 
	: m_env(proc->getStretchSource()->getMutex()), m_cs(proc->getStretchSource()->getMutex()), m_proc(proc)
{
	addAndMakeVisible(m_env);
	const auto& pars = m_proc->getParameters();
	m_parcomps.emplace_back(std::make_unique<ParameterComponent>(pars[cpi_freefilter_shiftx],false));
	addAndMakeVisible(m_parcomps.back().get());
	m_parcomps.emplace_back(std::make_unique<ParameterComponent>(pars[cpi_freefilter_shifty], false));
	addAndMakeVisible(m_parcomps.back().get());
	m_parcomps.emplace_back(std::make_unique<ParameterComponent>(pars[cpi_freefilter_scaley], false));
	addAndMakeVisible(m_parcomps.back().get());
	m_parcomps.emplace_back(std::make_unique<ParameterComponent>(pars[cpi_freefilter_tilty], false));
	addAndMakeVisible(m_parcomps.back().get());
	m_parcomps.emplace_back(std::make_unique<ParameterComponent>(pars[cpi_freefilter_randomy_numbands], false));
	addAndMakeVisible(m_parcomps.back().get());
	m_parcomps.emplace_back(std::make_unique<ParameterComponent>(pars[cpi_freefilter_randomy_rate], false));
	addAndMakeVisible(m_parcomps.back().get());
	m_parcomps.emplace_back(std::make_unique<ParameterComponent>(pars[cpi_freefilter_randomy_amount], false));
	addAndMakeVisible(m_parcomps.back().get());
}

void FreeFilterComponent::resized()
{
	m_env.setBounds(m_slidwidth, 0, getWidth() - m_slidwidth, getHeight());
	for (int i = 0; i < m_parcomps.size(); ++i)
	{
		m_parcomps[i]->setBounds(1, 1+25*i, m_slidwidth - 2, 24);
	}
	
}

void FreeFilterComponent::paint(Graphics & g)
{
	g.setColour(Colours::grey);
	g.fillRect(0, 0, m_slidwidth, getHeight());
}

void FreeFilterComponent::updateParameterComponents()
{
	for (auto& e : m_parcomps)
		e->updateComponent();
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
	m_proc(p), m_filefilter(p.m_afm->getWildcardForAllFormats(),String(),String())
{
	String initiallocfn = m_proc.m_propsfile->m_props_file->getValue("importfilefolder",
		File::getSpecialLocation(File::userHomeDirectory).getFullPathName());
	File initialloc(initiallocfn);
	m_fbcomp = std::make_unique<FileBrowserComponent>(1 | 4,
		initialloc, &m_filefilter, nullptr);
	m_fbcomp->addListener(this);
	addAndMakeVisible(m_fbcomp.get());
}

void MyFileBrowserComponent::resized()
{
	m_fbcomp->setBounds(0, 0, getWidth(), getHeight());
}

void MyFileBrowserComponent::paint(Graphics & g)
{
	//g.fillAll(Colours::yellow);
}

void MyFileBrowserComponent::selectionChanged()
{
}

void MyFileBrowserComponent::fileClicked(const File & file, const MouseEvent & e)
{
}

void MyFileBrowserComponent::fileDoubleClicked(const File & file)
{
	m_proc.setAudioFile(file);
	m_proc.m_propsfile->m_props_file->setValue("importfilefolder", file.getParentDirectory().getFullPathName());
}

void MyFileBrowserComponent::browserRootChanged(const File & newRoot)
{
}
