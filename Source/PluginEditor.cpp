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

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <array>

extern String g_plugintitle;

//==============================================================================
PaulstretchpluginAudioProcessorEditor::PaulstretchpluginAudioProcessorEditor(PaulstretchpluginAudioProcessor& p)
	: AudioProcessorEditor(&p),
	m_wavecomponent(p.m_afm,p.m_thumb.get()),
	processor(p)

{
	addAndMakeVisible(&m_import_button);
	m_import_button.setButtonText("Import file...");
	m_import_button.onClick = [this]() { chooseFile(); };
	
	addAndMakeVisible(&m_settings_button);
	m_settings_button.setButtonText("Settings...");
	m_settings_button.onClick = [this]() { showSettingsMenu(); };
	
	addAndMakeVisible(&m_info_label);
	m_info_label.setJustificationType(Justification::centredRight);

	m_wavecomponent.GetFileCallback = [this]() { return processor.getAudioFile(); };
	addAndMakeVisible(&m_wavecomponent);
	const auto& pars = processor.getParameters();
	for (int i=0;i<pars.size();++i)
	{
		AudioProcessorParameterWithID* parid = dynamic_cast<AudioProcessorParameterWithID*>(pars[i]);
		jassert(parid);
		bool notifyonlyonrelease = false;
		if (parid->paramID.startsWith("fftsize") || parid->paramID.startsWith("numoutchans"))
			notifyonlyonrelease = true;
		m_parcomps.push_back(std::make_shared<ParameterComponent>(pars[i],notifyonlyonrelease));
		addAndMakeVisible(m_parcomps.back().get());
	}
	
	//addAndMakeVisible(&m_specvis);
	setSize (1000, 30+(pars.size()/2)*25+200);
	m_wavecomponent.TimeSelectionChangedCallback = [this](Range<double> range, int which)
	{
		*processor.getFloatParameter(5) = range.getStart();
		*processor.getFloatParameter(6) = range.getEnd();
	};
	m_wavecomponent.CursorPosCallback = [this]()
	{
		return processor.getStretchSource()->getInfilePositionPercent();
	};
	m_wavecomponent.ShowFileCacheRange = true;
	m_spec_order_ed.setSource(processor.getStretchSource());
	addAndMakeVisible(&m_spec_order_ed);
	startTimer(1, 100);
	startTimer(2, 1000);
	startTimer(3, 200);
	m_wavecomponent.startTimer(100);
	
}

PaulstretchpluginAudioProcessorEditor::~PaulstretchpluginAudioProcessorEditor()
{
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
	m_info_label.setBounds(m_settings_button.getRight() + 1, m_settings_button.getY(), 
		getWidth()-m_settings_button.getRight()-1, 24);
	int w = getWidth();
	int xoffs = 1;
	int yoffs = 30;
	int div = w / 4;
	m_parcomps[cpi_capture_enabled]->setBounds(xoffs, yoffs, div-1, 24);
	//xoffs += div;
	//m_parcomps[cpi_max_capture_len]->setBounds(xoffs, yoffs, div - 1, 24);
	xoffs += div;
	m_parcomps[cpi_passthrough]->setBounds(xoffs, yoffs, div - 1, 24);
	xoffs += div;
	m_parcomps[cpi_pause_enabled]->setBounds(xoffs, yoffs, div-1, 24);
	xoffs += div;
	m_parcomps[cpi_freeze]->setBounds(xoffs, yoffs, div - 1, 24);
	xoffs = 1;
	yoffs += 25;
	div = w / 2;
	m_parcomps[cpi_main_volume]->setBounds(xoffs, yoffs, div-1, 24);
	xoffs += div;
	m_parcomps[cpi_num_outchans]->setBounds(xoffs, yoffs, div-1, 24);
	xoffs = 1;
	yoffs += 25;
	m_parcomps[cpi_fftsize]->setBounds(xoffs, yoffs, div - 1, 24);
	xoffs += div;
	m_parcomps[cpi_stretchamount]->setBounds(xoffs, yoffs, div - 1, 24);
	xoffs = 1;
	yoffs += 25;
	m_parcomps[cpi_pitchshift]->setBounds(xoffs, yoffs, div - 1, 24);
	xoffs += div;
	m_parcomps[cpi_frequencyshift]->setBounds(xoffs, yoffs, div - 1, 24);
	xoffs = 1;
	yoffs += 25;
	m_parcomps[cpi_octavesm2]->setBounds(xoffs, yoffs, div - 1, 24);
	xoffs += div;
	m_parcomps[cpi_octavesm1]->setBounds(xoffs, yoffs, div - 1, 24);
	xoffs = 1;
	yoffs += 25;
	m_parcomps[cpi_octaves0]->setBounds(xoffs, yoffs, div - 1, 24);
	xoffs += div;
	m_parcomps[cpi_octaves1]->setBounds(xoffs, yoffs, div - 1, 24);
	xoffs = 1;
	yoffs += 25;
	m_parcomps[cpi_octaves15]->setBounds(xoffs, yoffs, div - 1, 24);
	xoffs += div;
	m_parcomps[cpi_octaves2]->setBounds(xoffs, yoffs, div - 1, 24);
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
	m_parcomps[cpi_soundstart]->setBounds(xoffs, yoffs, div - 1, 24);
	xoffs += div;
	m_parcomps[cpi_soundend]->setBounds(xoffs, yoffs, div - 1, 24);
	xoffs = 1;
	yoffs += 25;
	m_parcomps[cpi_filter_low]->setBounds(xoffs, yoffs, div - 1, 24);
	xoffs += div;
	m_parcomps[cpi_filter_high]->setBounds(xoffs, yoffs, div - 1, 24);
	xoffs = 1;
	yoffs += 25;
	m_parcomps[cpi_loopxfadelen]->setBounds(xoffs, yoffs, div - 1, 24);
	xoffs += div;
	m_parcomps[cpi_onsetdetection]->setBounds(xoffs, yoffs, div - 1, 24);
	yoffs += 25;
	int remain_h = getHeight() - 1 - yoffs;
	m_spec_order_ed.setBounds(1, yoffs, getWidth() - 2, remain_h / 5 * 1);
	m_wavecomponent.setBounds(1, m_spec_order_ed.getBottom()+1, getWidth()-2, remain_h/5*4);
	//m_specvis.setBounds(1, yoffs, getWidth() - 2, getHeight() - 1 - yoffs);
}

void PaulstretchpluginAudioProcessorEditor::timerCallback(int id)
{
	if (id == 1)
	{
		for (auto& e : m_parcomps)
			e->updateComponent();
		if (processor.isRecordingEnabled())
		{
			m_wavecomponent.setRecordingPosition(processor.getRecordingPositionPercent());
		} else
			m_wavecomponent.setRecordingPosition(-1.0);
		String infotext = String(processor.getPreBufferingPercent()*100.0, 1) + "% buffered " 
			+ String(processor.getStretchSource()->m_param_change_count)+" param changes "+m_last_err+" FFT size "+
			String(processor.getStretchSource()->getFFTSize());
		if (processor.m_abnormal_output_samples > 0)
			infotext += " " + String(processor.m_abnormal_output_samples) + " invalid sample values";
		if (processor.isNonRealtime())
			infotext += " (offline rendering)";
		if (processor.m_playposinfo.isPlaying)
			infotext += " "+String(processor.m_playposinfo.timeInSeconds,1);
		
		m_info_label.setText(infotext, dontSendNotification);
	}
	if (id == 2)
	{
		m_wavecomponent.setTimeSelection(processor.getTimeSelection());
		if (processor.m_state_dirty)
		{
			m_spec_order_ed.setSource(processor.getStretchSource());
			processor.m_state_dirty = false;
		}
	}
	if (id == 3)
	{
		//m_specvis.setState(processor.getStretchSource()->getProcessParameters(), processor.getStretchSource()->getFFTSize() / 2,
		//	processor.getSampleRate());
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

void PaulstretchpluginAudioProcessorEditor::chooseFile()
{
	String initiallocfn = processor.m_propsfile->m_props_file->getValue("importfilefolder",
                                                File::getSpecialLocation(File::userHomeDirectory).getFullPathName());
    File initialloc(initiallocfn);
	String filterstring = processor.m_afm->getWildcardForAllFormats();
	FileChooser myChooser("Please select audio file...",
		initialloc,
		filterstring,true);
	if (myChooser.browseForFileToOpen())
	{
        File resu = myChooser.getResult();
        String pathname = resu.getFullPathName();
        if (pathname.startsWith("/localhost"))
        {
            pathname = pathname.substring(10);
            resu = File(pathname);
        }
        processor.m_propsfile->m_props_file->setValue("importfilefolder", resu.getParentDirectory().getFullPathName());
        m_last_err = processor.setAudioFile(resu);
	}
}

void PaulstretchpluginAudioProcessorEditor::showSettingsMenu()
{
	PopupMenu menu;
	menu.addItem(4, "Reset parameters", true, false);
	menu.addItem(5, "Load file with plugin state", true, processor.m_load_file_with_state);
	menu.addItem(1, "Play when host transport running", true, processor.m_play_when_host_plays);
	menu.addItem(2, "Capture when host transport running", true, processor.m_capture_when_host_plays);
	
	
    PopupMenu bufferingmenu;
    int curbufamount = processor.getPreBufferAmount();
    bufferingmenu.addItem(100,"None",true,curbufamount == -1);
    bufferingmenu.addItem(101,"Small",true,curbufamount == 1);
    bufferingmenu.addItem(102,"Medium",true,curbufamount == 2);
    bufferingmenu.addItem(103,"Large",true,curbufamount == 3);
    bufferingmenu.addItem(104,"Very large",true,curbufamount == 4);
    bufferingmenu.addItem(105,"Huge",true,curbufamount == 5);
    menu.addSubMenu("Prebuffering", bufferingmenu);
	int capturelen = *processor.getFloatParameter(cpi_max_capture_len);
	PopupMenu capturelenmenu;
	std::vector<int> capturelens{ 2,5,10,30,60,120 };
	for (int i=0;i<capturelens.size();++i)
		capturelenmenu.addItem(200+i, String(capturelens[i])+" seconds", true, capturelen == capturelens[i]);
	menu.addSubMenu("Capture buffer length", capturelenmenu);
	menu.addItem(3, "About...", true, false);
#ifdef JUCE_DEBUG
	menu.addItem(6, "Dump preset to clipboard", true, false);
#endif
	int r = menu.show();
	if (r >= 200 && r < 210)
	{
		int caplen = capturelens[r - 200];
		*processor.getFloatParameter(cpi_max_capture_len) = (float)caplen;
	}
	if (r == 1)
	{
		processor.m_play_when_host_plays = !processor.m_play_when_host_plays;
	}
	if (r == 2)
	{
		processor.m_capture_when_host_plays = !processor.m_capture_when_host_plays;
	}
	if (r == 4)
	{
		processor.resetParameters();
	}
	if (r == 5)
	{
		processor.m_load_file_with_state = !processor.m_load_file_with_state;
	}
	if (r == 3)
	{
		String fftlib = fftwf_version;
String juceversiontxt = String("JUCE ") + String(JUCE_MAJOR_VERSION) + "." + String(JUCE_MINOR_VERSION);
		AlertWindow::showMessageBoxAsync(AlertWindow::InfoIcon,
			g_plugintitle,
			"Plugin for extreme time stretching and other sound processing\nBuilt on " + String(__DATE__) + " " + String(__TIME__) + "\n"
			"Copyright (C) 2006-2011 Nasca Octavian Paul, Tg. Mures, Romania\n"
			"(C) 2017-2018 Xenakios\n\n"
			"Using " + fftlib + " for FFT\n\n"
			+ juceversiontxt + " (c) Roli. Used under the GPL license.\n\n"
			"GPL licensed source code for this plugin at : https://bitbucket.org/xenakios/paulstretchplugin/overview\n"
			, "OK",
			this);

	}
    if (r >= 100 && r < 200)
    {
        if (r == 100)
            processor.m_use_backgroundbuffering = false;
        if (r > 100)
            processor.setPreBufferAmount(r-100);
    }
	if (r == 6)
	{
		ValueTree tree = processor.getStateTree(true,true);
		MemoryBlock destData;
		MemoryOutputStream stream(destData, true);
		tree.writeToStream(stream);
		String txt = Base64::toBase64(destData.getData(), destData.getSize());
		SystemClipboard::copyTextToClipboard(txt);
	}
}

WaveformComponent::WaveformComponent(AudioFormatManager* afm, AudioThumbnail* thumb)
{
	TimeSelectionChangedCallback = [](Range<double>, int) {};
	if (m_use_opengl == true)
		m_ogl.attachTo(*this);
	m_thumbnail = thumb;
	m_thumbnail->addChangeListener(this);
	setOpaque(true);
}

WaveformComponent::~WaveformComponent()
{
	if (m_use_opengl == true)
		m_ogl.detach();
	m_thumbnail->removeChangeListener(this);
}

void WaveformComponent::changeListenerCallback(ChangeBroadcaster * /*cb*/)
{
	m_waveimage = Image();
	repaint();
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
		if (m_waveimage.isValid() == false || m_waveimage.getWidth() != getWidth()
			|| m_waveimage.getHeight() != getHeight() - m_topmargin)
		{
			//Logger::writeToLog("updating cached waveform image");
			m_waveimage = Image(Image::ARGB, getWidth(), getHeight() - m_topmargin, true);
			Graphics tempg(m_waveimage);
			tempg.fillAll(Colours::black);
			tempg.setColour(Colours::darkgrey);
			m_thumbnail->drawChannels(tempg, { 0,0,getWidth(),getHeight() - m_topmargin },
				thumblen*m_view_range.getStart(), thumblen*m_view_range.getEnd(), 1.0f);
		}
		g.drawImage(m_waveimage, 0, m_topmargin, getWidth(), getHeight() - m_topmargin, 0, 0, getWidth(), getHeight() - m_topmargin);

	}
	else
	{
		//g.fillAll(Colours::black);
		g.setColour(Colours::darkgrey);
		m_thumbnail->drawChannels(g, { 0,m_topmargin,getWidth(),getHeight() - m_topmargin },
			thumblen*m_view_range.getStart(), thumblen*m_view_range.getEnd(), 1.0f);
	}

	//g.setColour(Colours::darkgrey);
	//m_thumb->drawChannels(g, { 0,m_topmargin,getWidth(),getHeight()-m_topmargin }, 
	//	0.0, thumblen, 1.0f);
	g.setColour(Colours::white.withAlpha(0.5f));
	int xcorleft = (int)jmap<double>(m_time_sel_start, m_view_range.getStart(), m_view_range.getEnd(), 0, getWidth());
	int xcorright = (int)jmap<double>(m_time_sel_end, m_view_range.getStart(), m_view_range.getEnd(), 0, getWidth());
	g.fillRect(xcorleft, m_topmargin, xcorright - xcorleft, getHeight() - m_topmargin);
	if (m_file_cached.first.getLength() > 0.0 &&
		(bool)ShowFileCacheRange.getValue())
	{
		g.setColour(Colours::red.withAlpha(0.2f));
		xcorleft = (int)jmap<double>(m_file_cached.first.getStart(), m_view_range.getStart(), m_view_range.getEnd(), 0, getWidth());
		xcorright = (int)jmap<double>(m_file_cached.first.getEnd(), m_view_range.getStart(), m_view_range.getEnd(), 0, getWidth());
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
		double pos = jmap<double>(CursorPosCallback(), m_view_range.getStart(), m_view_range.getEnd(), 0, getWidth());
		g.fillRect((int)pos, m_topmargin, 1, getHeight() - m_topmargin);
	}
	if (m_rec_pos >= 0.0)
	{
		g.setColour(Colours::lightpink);
		double pos = jmap<double>(m_rec_pos, m_view_range.getStart(), m_view_range.getEnd(), 0, getWidth());
		g.fillRect((int)pos, m_topmargin, 1, getHeight() - m_topmargin);
	}
	g.setColour(Colours::aqua.darker());
	g.drawText(GetFileCallback().getFileName(), 2, m_topmargin + 2, getWidth(), 20, Justification::topLeft);
}

void WaveformComponent::timerCallback()
{
	repaint();
}

void WaveformComponent::setFileCachedRange(std::pair<Range<double>, Range<double>> rng)
{
	m_file_cached = rng;
	//repaint();
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
	double pos = jmap<double>(e.x, 0, getWidth(), m_view_range.getStart(), m_view_range.getEnd());
	if (e.y < m_topmargin)
	{
		if (SeekCallback)
			SeekCallback(pos);
		m_didseek = true;
	}
	else
	{
		m_time_sel_drag_target = getTimeSelectionEdge(e.x, e.y);
		m_drag_time_start = pos;
		if (m_time_sel_drag_target == 0)
		{
			m_time_sel_start = -1.0;
			m_time_sel_end = -1.0;
		}
	}

	repaint();
}

void WaveformComponent::mouseUp(const MouseEvent & /*e*/)
{
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
	if (m_time_sel_drag_target == 0)
	{
		m_time_sel_start = m_drag_time_start;
		m_time_sel_end = jmap<double>(e.x, 0, getWidth(), m_view_range.getStart(), m_view_range.getEnd());
	}
	if (m_time_sel_drag_target == 1)
	{
		m_time_sel_start = jmap<double>(e.x, 0, getWidth(), m_view_range.getStart(), m_view_range.getEnd());
	}
	if (m_time_sel_drag_target == 2)
	{
		m_time_sel_end = jmap<double>(e.x, 0, getWidth(), m_view_range.getStart(), m_view_range.getEnd());
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
				m_insamples[i] += scaler * oscgain * sin(2 * 3.141592653 / samplerate * i* (hz + hz * j));
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
	if (pars.harmonics.enabled)
		spectrum_do_harmonics(pars, m_freqs3, nfreqs, samplerate, m_freqs1.data(), m_freqs2.data());
	else spectrum_copy(nfreqs, m_freqs1.data(), m_freqs2.data());
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
			drawBox(g, i, i*box_w, 0, box_w - 30, box_h);
		if (i<m_order.size() - 1)
			g.drawArrow(juce::Line<float>(i*box_w + (box_w - 30), box_h / 2, i*box_w + box_w, box_h / 2), 2.0f, 15.0f, 15.0f);
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
	m_drag_x = -1;
	repaint();
}

void SpectralChainEditor::mouseDrag(const MouseEvent & ev)
{
	if (m_cur_index >= 0 && m_cur_index < m_order.size())
	{
		int box_w = getWidth() / m_order.size();
		int box_h = getHeight();
		int new_index = ev.x / box_w;
		if (new_index >= 0 && new_index < m_order.size() && new_index != m_cur_index)
		{
			std::swap(m_order[m_cur_index], m_order[new_index]);
			m_cur_index = new_index;
			m_did_drag = true;
			m_src->setSpectrumProcessOrder(m_order);
		}
		m_drag_x = ev.x;
		repaint();
	}
}

void SpectralChainEditor::mouseUp(const MouseEvent & ev)
{
	m_drag_x = -1;
	//m_cur_index = -1;
	repaint();
}

void SpectralChainEditor::drawBox(Graphics & g, int index, int x, int y, int w, int h)
{
	String txt;
	if (m_order[index] == 0)
		txt = "Harmonics";
	if (m_order[index] == 1)
		txt = "Tonal vs Noise";
	if (m_order[index] == 2)
		txt = "Frequency shift";
	if (m_order[index] == 3)
		txt = "Pitch shift";
	if (m_order[index] == 4)
		txt = "Octaves";
	if (m_order[index] == 5)
		txt = "Spread";
	if (m_order[index] == 6)
		txt = "Filter";
	if (m_order[index] == 7)
		txt = "Compressor";
	if (index == m_cur_index)
	{
		g.setColour(Colours::darkgrey);
		//g.fillRect(i*box_w, 0, box_w - 30, box_h - 1);
		g.fillRect(x, y, w, h);
	}
	g.setColour(Colours::white);
	g.drawRect(x, y, w, h);
	g.drawFittedText(txt, x,y,w,h, Justification::centred, 3);
}

ParameterComponent::ParameterComponent(AudioProcessorParameter * par, bool notifyOnlyOnRelease) : m_par(par)
{
	addAndMakeVisible(&m_label);
	m_label.setText(par->getName(50), dontSendNotification);
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
		m_togglebut->setButtonText(par->getName(50));
		addAndMakeVisible(m_togglebut.get());
	}
}

void ParameterComponent::resized()
{
	if (m_slider)
	{
		m_label.setBounds(0, 0, 200, 24);
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
	if (m_togglebut != nullptr && m_togglebut->getToggleState() != *boolpar)
	{
		*boolpar = m_togglebut->getToggleState();
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
	if (m_togglebut != nullptr && m_togglebut->getToggleState() != *boolpar)
	{
		m_togglebut->setToggleState(*boolpar, dontSendNotification);
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
