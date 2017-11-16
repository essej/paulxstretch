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
    m_wavecomponent(p.m_afm.get()),
    processor (p)

{
	addAndMakeVisible(&m_import_button);
	m_import_button.setButtonText("Import file...");
	m_import_button.addListener(this);
	addAndMakeVisible(&m_info_label);
	addAndMakeVisible(&m_wavecomponent);
	const auto& pars = processor.getParameters();
	for (int i=0;i<pars.size();++i)
	{
		m_parcomps.push_back(std::make_shared<ParameterComponent>(pars[i]));
		m_parcomps.back()->setBounds(1, i * 25, 598, 24);
		addAndMakeVisible(m_parcomps.back().get());
	}
	addAndMakeVisible(&m_rec_enable);
	m_rec_enable.setButtonText("Capture");
	m_rec_enable.addListener(this);
	setSize (700, pars.size()*25+200);
	m_wavecomponent.TimeSelectionChangedCallback = [this](Range<double> range, int which)
	{
		*processor.getFloatParameter(5) = range.getStart();
		*processor.getFloatParameter(6) = range.getEnd();
	};
	m_wavecomponent.CursorPosCallback = [this]()
	{
		return processor.m_control->getLivePlayPosition();
	};
	m_wavecomponent.ShowFileCacheRange = true;
	startTimer(1, 100);
	startTimer(2, 1000);
	m_wavecomponent.startTimer(100);
}

PaulstretchpluginAudioProcessorEditor::~PaulstretchpluginAudioProcessorEditor()
{
}

void PaulstretchpluginAudioProcessorEditor::buttonClicked(Button * but)
{
	if (but == &m_rec_enable)
	{
		processor.setRecordingEnabled(but->getToggleState());
	}
	if (but == &m_import_button)
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
}

//==============================================================================
void PaulstretchpluginAudioProcessorEditor::paint (Graphics& g)
{
	g.fillAll(Colours::darkgrey);
}

void PaulstretchpluginAudioProcessorEditor::resized()
{
	m_rec_enable.setBounds(1, m_parcomps.back()->getBottom()+1, 10, 24);
	m_rec_enable.changeWidthToFitText();
	m_info_label.setBounds(m_rec_enable.getRight() + 1, m_rec_enable.getY(), 60, 24);
	m_import_button.setBounds(m_info_label.getRight() + 1, m_rec_enable.getY(), 60, 24);
	m_import_button.changeWidthToFitText();
	m_wavecomponent.setBounds(1, m_info_label.getBottom()+1, getWidth()-2, getHeight()-1-m_info_label.getBottom());
}

void PaulstretchpluginAudioProcessorEditor::timerCallback(int id)
{
	if (id == 1)
	{
		for (auto& e : m_parcomps)
			e->updateComponent();
		if (processor.isRecordingEnabled() != m_rec_enable.getToggleState())
			m_rec_enable.setToggleState(processor.isRecordingEnabled(), dontSendNotification);
		if (processor.isRecordingEnabled())
		{
			m_wavecomponent.setRecordingPosition(processor.getRecordingPositionPercent());
		} else
			m_wavecomponent.setRecordingPosition(-1.0);
		//m_info_label.setText(String(processor.m_control->getStretchAudioSource()->m_param_change_count), dontSendNotification);
        double prebufavail=processor.m_control->getPreBufferingPercent();
        m_info_label.setText(String(prebufavail,1), dontSendNotification);
	}
	if (id == 2)
	{
		if (processor.getAudioFile() != File() && processor.getAudioFile() != m_wavecomponent.getAudioFile())
		{
			m_wavecomponent.setAudioFile(processor.getAudioFile());
		}
		m_wavecomponent.setTimeSelection(processor.getTimeSelection());
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

WaveformComponent::WaveformComponent(AudioFormatManager* afm) : m_thumbcache(100)
{
	TimeSelectionChangedCallback = [](Range<double>, int) {};
	if (m_use_opengl == true)
		m_ogl.attachTo(*this);
	// The default priority of 2 is a bit too low in some cases, it seems...
	m_thumbcache.getTimeSliceThread().setPriority(3);
	m_thumb = std::make_unique<AudioThumbnail>(512, *afm, m_thumbcache);
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
			m_thumbcache.removeThumb(m_thumb->getHashCode());
		if (m_thumb != nullptr)
			m_thumb->reset(0, 0.0);
		m_thumb->setSource(new FileInputSource(f));
		m_curfile = f;
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
