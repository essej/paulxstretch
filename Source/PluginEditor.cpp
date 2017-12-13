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
    : AudioProcessorEditor (&p),
    m_wavecomponent(p.m_afm),
    processor (p)

{
	addAndMakeVisible(&m_import_button);
	m_import_button.setButtonText("Import file...");
	attachCallback(m_import_button, [this]() { chooseFile(); });
	
	addAndMakeVisible(&m_info_label);
	addAndMakeVisible(&m_wavecomponent);
	const auto& pars = processor.getParameters();
	for (int i=0;i<pars.size();++i)
	{
		AudioProcessorParameterWithID* parid = dynamic_cast<AudioProcessorParameterWithID*>(pars[i]);
		jassert(parid);
		if (parid)
		{
			bool notifyonlyonrelease = false;
			if (parid->paramID.startsWith("fftsize"))
				notifyonlyonrelease = true;
			m_parcomps.push_back(std::make_shared<ParameterComponent>(pars[i],notifyonlyonrelease));
			addAndMakeVisible(m_parcomps.back().get());
		}
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
	
	m_info_label.setBounds(m_import_button.getRight() + 1, m_import_button.getY(), getWidth()-m_import_button.getRight()-1, 24);
	
	for (int i = 0; i < m_parcomps.size(); ++i)
	{
		int gridx = i % 2;
		int gridy = i / 2;
		m_parcomps[i]->setBounds(1+gridx*(getWidth()/2), 30 + gridy * 25, getWidth()/2-2, 24);
	}
	int yoffs = m_parcomps.back()->getBottom() + 1;
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
		String infotext = String(processor.getPreBufferingPercent(), 1) + " " + String(processor.getStretchSource()->m_param_change_count);
		m_info_label.setText(infotext, dontSendNotification);
	}
	if (id == 2)
	{
		if (processor.getAudioFile() != File() && processor.getAudioFile() != m_wavecomponent.getAudioFile())
		{
			m_wavecomponent.setAudioFile(processor.getAudioFile());
		}
        if (processor.getAudioFile()==File() && processor.isRecordingEnabled()==false && m_wavecomponent.isUsingAudioBuffer()==false)
        {
            auto bufptr = processor.getStretchSource()->getSourceAudioBuffer();
            if (bufptr!=nullptr)
                m_wavecomponent.setAudioBuffer(bufptr,
                                               processor.getSampleRate(), bufptr->getNumSamples());
        }
		m_wavecomponent.setTimeSelection(processor.getTimeSelection());
		
	}
	if (id == 3)
	{
		//m_specvis.setState(processor.getStretchSource()->getProcessParameters(), processor.getStretchSource()->getFFTSize() / 2,
		//	processor.getSampleRate());
	}
}

void PaulstretchpluginAudioProcessorEditor::setAudioFile(File f)
{
	m_wavecomponent.setAudioFile(f);
}

void PaulstretchpluginAudioProcessorEditor::setAudioBuffer(AudioBuffer<float>* buf, int samplerate, int len)
{
	MessageManager::callAsync([this,buf, samplerate, len]()
	{
		m_wavecomponent.setAudioBuffer(buf, samplerate, len);
	});
}

void PaulstretchpluginAudioProcessorEditor::beginAddingAudioBlocks(int channels, int samplerate, int totalllen)
{
	m_wavecomponent.beginAddingAudioBlocks(channels, samplerate, totalllen);
}

void PaulstretchpluginAudioProcessorEditor::addAudioBlock(AudioBuffer<float>& buf, int samplerate, int pos)
{
	m_wavecomponent.addAudioBlock(buf, samplerate, pos);
}

void PaulstretchpluginAudioProcessorEditor::chooseFile()
{
#ifdef WIN32
	File initialloc("C:/MusicAudio/sourcesamples");
#else
	File initialloc("/Users/teemu/AudioProjects/sourcesamples");
#endif
	FileChooser myChooser("Please select audio file...",
		initialloc,
		"*.wav");
	if (myChooser.browseForFileToOpen())
	{
		processor.setAudioFile(myChooser.getResult());
		if (processor.getAudioFile() != File())
		{
			m_wavecomponent.setAudioFile(processor.getAudioFile());
		}
	}
}

WaveformComponent::WaveformComponent(AudioFormatManager* afm)
{
	TimeSelectionChangedCallback = [](Range<double>, int) {};
	if (m_use_opengl == true)
		m_ogl.attachTo(*this);
	// The default priority of 2 is a bit too low in some cases, it seems...
	m_thumbcache->getTimeSliceThread().setPriority(3);
	m_thumb = std::make_unique<AudioThumbnail>(512, *afm, *m_thumbcache);
	m_thumb->addChangeListener(this);
	setOpaque(true);
}

WaveformComponent::~WaveformComponent()
{
	if (m_use_opengl == true)
		m_ogl.detach();
}

void WaveformComponent::changeListenerCallback(ChangeBroadcaster * /*cb*/)
{
	m_waveimage = Image();
	repaint();
}

void WaveformComponent::paint(Graphics & g)
{
	//Logger::writeToLog("Waveform component paint");
	g.fillAll(Colours::black);
	g.setColour(Colours::darkgrey);
	g.fillRect(0, 0, getWidth(), m_topmargin);
	if (m_thumb == nullptr || m_thumb->getTotalLength() < 0.1)
	{
		g.setColour(Colours::aqua.darker());
		g.drawText("No file loaded", 2, m_topmargin + 2, getWidth(), 20, Justification::topLeft);
		return;
	}
	g.setColour(Colours::lightslategrey);
	double thumblen = m_thumb->getTotalLength();
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
			m_thumb->drawChannels(tempg, { 0,0,getWidth(),getHeight() - m_topmargin },
				thumblen*m_view_range.getStart(), thumblen*m_view_range.getEnd(), 1.0f);
		}
		g.drawImage(m_waveimage, 0, m_topmargin, getWidth(), getHeight() - m_topmargin, 0, 0, getWidth(), getHeight() - m_topmargin);

	}
	else
	{
		//g.fillAll(Colours::black);
		g.setColour(Colours::darkgrey);
		m_thumb->drawChannels(g, { 0,m_topmargin,getWidth(),getHeight() - m_topmargin },
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
	g.drawText(m_curfile.getFullPathName(), 2, m_topmargin + 2, getWidth(), 20, Justification::topLeft);
}

void WaveformComponent::setAudioFile(File f)
{
	if (f.existsAsFile())
	{
		m_waveimage = Image();
		if (m_thumb != nullptr && f == m_curfile) // reloading same file, might happen that the overview needs to be redone...
			m_thumbcache->removeThumb(m_thumb->getHashCode());
		if (m_thumb != nullptr)
			m_thumb->reset(0, 0.0);
		m_thumb->setSource(new FileInputSource(f));
		m_curfile = f;
        m_using_audio_buffer = false;
	}
	else
	{
		m_thumb->setSource(nullptr);
		m_curfile = File();
	}
	repaint();
}

void WaveformComponent::setAudioBuffer(AudioBuffer<float>* buf, int samplerate, int len)
{
    jassert(buf!=nullptr);
    m_using_audio_buffer = true;
    m_waveimage = Image();
	m_curfile = File();
	m_thumb->reset(buf->getNumChannels(), samplerate, len);
	m_thumb->addBlock(0, *buf, 0, len);
}

void WaveformComponent::beginAddingAudioBlocks(int channels, int samplerate, int totalllen)
{
	m_waveimage = Image();
	m_curfile = File();
	m_thumb->reset(channels, samplerate, totalllen);
}

void WaveformComponent::addAudioBlock(AudioBuffer<float>& buf, int samplerate, int pos)
{
	m_thumb->addBlock(pos, buf, 0, buf.getNumSamples());
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
		String txt;
		if (m_order[i] == 0)
			txt = "Harmonics";
		if (m_order[i] == 1)
			txt = "Tonal vs Noise";
		if (m_order[i] == 2)
			txt = "Frequency shift";
		if (m_order[i] == 3)
			txt = "Pitch shift";
		if (m_order[i] == 4)
			txt = "Octaves";
		if (m_order[i] == 5)
			txt = "Spread";
		if (m_order[i] == 6)
			txt = "Filter";
		if (m_order[i] == 7)
			txt = "Compressor";
		if (i == m_cur_index)
		{
			g.setColour(Colours::darkgrey);
			g.fillRect(i*box_w, 0, box_w - 30, box_h - 1);
		}
		g.setColour(Colours::white);
		g.drawRect(i*box_w, 0, box_w - 30, box_h - 1);
		g.drawFittedText(txt, i*box_w, 0, box_w - 30, box_h - 1, Justification::centred, 3);
		if (i<m_order.size() - 1)
			g.drawArrow(juce::Line<float>(i*box_w + (box_w - 30), box_h / 2, i*box_w + box_w, box_h / 2), 2.0f, 15.0f, 15.0f);
	}
}

void SpectralChainEditor::mouseDown(const MouseEvent & ev)
{
	m_did_drag = false;
	int box_w = getWidth() / m_order.size();
	int box_h = getHeight();
	m_cur_index = ev.x / box_w;
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
			repaint();
		}
	}
}

void SpectralChainEditor::mouseUp(const MouseEvent & ev)
{
	if (m_did_drag == true)
	{
		//m_src->setSpectrumProcessOrder(m_order);
	}
}
