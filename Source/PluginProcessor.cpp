/*
  ==============================================================================

    This file was auto-generated!

    It contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <set>

#ifdef WIN32
#undef min
#undef max
#endif

std::set<PaulstretchpluginAudioProcessor*> g_activeprocessors;

template<typename F>
void callGUI(AudioProcessor* ap, F&& f, bool async)
{
    auto ed = dynamic_cast<PaulstretchpluginAudioProcessorEditor*>(ap->getActiveEditor());
    if (ed)
    {
        if (async == false)
            f(ed);
        else
            MessageManager::callAsync([ed,f]() { f(ed); });
    }
}

int get_optimized_updown(int n, bool up) {
	int orig_n = n;
	while (true) {
		n = orig_n;

		while (!(n % 11)) n /= 11;
		while (!(n % 7)) n /= 7;

		while (!(n % 5)) n /= 5;
		while (!(n % 3)) n /= 3;
		while (!(n % 2)) n /= 2;
		if (n<2) break;
		if (up) orig_n++;
		else orig_n--;
		if (orig_n<4) return 4;
	};
	return orig_n;
};

int optimizebufsize(int n) {
	int n1 = get_optimized_updown(n, false);
	int n2 = get_optimized_updown(n, true);
	if ((n - n1)<(n2 - n)) return n1;
	else return n2;
};

//==============================================================================
PaulstretchpluginAudioProcessor::PaulstretchpluginAudioProcessor()
	: m_bufferingthread("pspluginprebufferthread")
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
{
	g_activeprocessors.insert(this);
	m_recbuffer.setSize(2, 44100);
	m_recbuffer.clear();
	if (m_afm->getNumKnownFormats()==0)
		m_afm->registerBasicFormats();
	m_stretch_source = std::make_unique<StretchAudioSource>(2, m_afm);
	
	setPreBufferAmount(2);
	m_ppar.pitch_shift.enabled = true;
	m_ppar.freq_shift.enabled = true;
	m_ppar.filter.enabled = true;
    m_ppar.compressor.enabled = true;
	m_stretch_source->setOnsetDetection(0.0);
	m_stretch_source->setLoopingEnabled(true);
	m_stretch_source->setFFTWindowingType(1);
	addParameter(new AudioParameterFloat("mainvolume0", "Main volume", -24.0f, 12.0f, -3.0f)); // 0
	addParameter(new AudioParameterFloat("stretchamount0", "Stretch amount", 
		NormalisableRange<float>(0.1f, 128.0f, 0.01f, 0.5),1.0f)); // 1
	addParameter(new AudioParameterFloat("fftsize0", "FFT size", 0.0f, 1.0f, 0.7f)); // 2
	addParameter(new AudioParameterFloat("pitchshift0", "Pitch shift", -24.0f, 24.0f, 0.0f)); // 3
	addParameter(new AudioParameterFloat("freqshift0", "Frequency shift", -1000.0f, 1000.0f, 0.0f)); // 4
	addParameter(new AudioParameterFloat("playrange_start0", "Sound start", 0.0f, 1.0f, 0.0f)); // 5
	addParameter(new AudioParameterFloat("playrange_end0", "Sound end", 0.0f, 1.0f, 1.0f)); // 6
	addParameter(new AudioParameterBool("freeze0", "Freeze", false)); // 7
	addParameter(new AudioParameterFloat("spread0", "Frequency spread", 0.0f, 1.0f, 0.0f)); // 8
	addParameter(new AudioParameterFloat("compress0", "Compress", 0.0f, 1.0f, 0.0f)); // 9
	addParameter(new AudioParameterFloat("loopxfadelen0", "Loop xfade length", 0.0f, 1.0f, 0.0f)); // 10
	addParameter(new AudioParameterFloat("numharmonics0", "Num harmonics", 0.0f, 100.0f, 0.0f)); // 11
	addParameter(new AudioParameterFloat("harmonicsfreq0", "Harmonics base freq", 
		NormalisableRange<float>(1.0f, 5000.0f, 1.00f, 0.5), 128.0f)); // 12
	addParameter(new AudioParameterFloat("harmonicsbw0", "Harmonics bandwidth", 0.1f, 200.0f, 25.0f)); // 13
	addParameter(new AudioParameterBool("harmonicsgauss0", "Gaussian harmonics", false)); // 14
	addParameter(new AudioParameterFloat("octavemixm2_0", "2 octaves down level", 0.0f, 1.0f, 0.0f)); // 15
	addParameter(new AudioParameterFloat("octavemixm1_0", "Octave down level", 0.0f, 1.0f, 0.0f)); // 16
	addParameter(new AudioParameterFloat("octavemix0_0", "Normal pitch level", 0.0f, 1.0f, 1.0f)); // 17
	addParameter(new AudioParameterFloat("octavemix1_0", "1 octave up level", 0.0f, 1.0f, 0.0f)); // 18
	addParameter(new AudioParameterFloat("octavemix15_0", "1 octave and fifth up level", 0.0f, 1.0f, 0.0f)); // 19
	addParameter(new AudioParameterFloat("octavemix2_0", "2 octaves up level", 0.0f, 1.0f, 0.0f)); // 20
	addParameter(new AudioParameterFloat("tonalvsnoisebw_0", "Tonal vs Noise BW", 0.74f, 1.0f, 0.74f)); // 21
	addParameter(new AudioParameterFloat("tonalvsnoisepreserve_0", "Tonal vs Noise preserve", -1.0f, 1.0f, 0.5f)); // 22
	addParameter(new AudioParameterFloat("filter_low_0", "Filter low", 20.0f, 10000.0f, 20.0f)); // 23
	addParameter(new AudioParameterFloat("filter_high_0", "Filter high", 20.0f, 20000.0f, 20000.0f)); // 24
	addParameter(new AudioParameterFloat("onsetdetect_0", "Onset detection", 0.0f, 1.0f, 0.0f)); // 25
	addParameter(new AudioParameterBool("capture_enabled0", "Capture", false)); // 26
	m_outchansparam = new AudioParameterInt("numoutchans0", "Num output channels", 2, 8, 2); // 27
	addParameter(m_outchansparam); // 27
	addParameter(new AudioParameterBool("pause_enabled0", "Pause", false)); // 28
	startTimer(1, 50);
}

PaulstretchpluginAudioProcessor::~PaulstretchpluginAudioProcessor()
{
	g_activeprocessors.erase(this);
	m_bufferingthread.stopThread(1000);
}

void PaulstretchpluginAudioProcessor::setPreBufferAmount(int x)
{
	int temp = jlimit(0, 5, x);
	if (temp != m_prebuffer_amount)
	{
		m_prebuffer_amount = temp;
		m_recreate_buffering_source = true;
	}
}

//==============================================================================
const String PaulstretchpluginAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool PaulstretchpluginAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool PaulstretchpluginAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool PaulstretchpluginAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double PaulstretchpluginAudioProcessor::getTailLengthSeconds() const
{
    return (double)m_bufamounts[m_prebuffer_amount]/getSampleRate();
}

int PaulstretchpluginAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int PaulstretchpluginAudioProcessor::getCurrentProgram()
{
    return 0;
}

void PaulstretchpluginAudioProcessor::setCurrentProgram (int index)
{
}

const String PaulstretchpluginAudioProcessor::getProgramName (int index)
{
    return {};
}

void PaulstretchpluginAudioProcessor::changeProgramName (int index, const String& newName)
{
}

void PaulstretchpluginAudioProcessor::setFFTSize(double size)
{
	if (m_prebuffer_amount == 5)
		m_fft_size_to_use = pow(2, 7.0 + size * 14.5);
	else m_fft_size_to_use = pow(2, 7.0 + size * 10.0); // chicken out from allowing huge FFT sizes if not enough prebuffering
	int optim = optimizebufsize(m_fft_size_to_use);
	m_fft_size_to_use = optim;
	m_stretch_source->setFFTSize(optim);
	//Logger::writeToLog(String(m_fft_size_to_use));
}

void PaulstretchpluginAudioProcessor::startplay(Range<double> playrange, int numoutchans, int maxBlockSize, String& err)
{
	m_stretch_source->setPlayRange(playrange, true);

	int bufamt = m_bufamounts[m_prebuffer_amount];

	if (m_buffering_source != nullptr && numoutchans != m_buffering_source->getNumberOfChannels())
		m_recreate_buffering_source = true;
	if (m_recreate_buffering_source == true)
	{
		m_buffering_source = std::make_unique<MyBufferingAudioSource>(m_stretch_source.get(),
			m_bufferingthread, false, bufamt, numoutchans, false);
		m_recreate_buffering_source = false;
	}
	if (m_bufferingthread.isThreadRunning() == false)
		m_bufferingthread.startThread();
	m_stretch_source->setNumOutChannels(numoutchans);
	m_stretch_source->setFFTSize(m_fft_size_to_use);
	m_stretch_source->setProcessParameters(&m_ppar);
	m_last_outpos_pos = 0.0;
	m_last_in_pos = playrange.getStart()*m_stretch_source->getInfileLengthSeconds();
	m_buffering_source->prepareToPlay(maxBlockSize, getSampleRate());
};

void PaulstretchpluginAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
	ScopedLock locker(m_cs);
	m_curmaxblocksize = samplesPerBlock;
	int numoutchans = *m_outchansparam;
	if (numoutchans != m_cur_num_out_chans)
		m_ready_to_play = false;
	if (m_using_memory_buffer == true)
	{
		int len = jlimit(100,m_recbuffer.getNumSamples(), m_rec_pos);
		m_stretch_source->setAudioBufferAsInputSource(&m_recbuffer, 
			getSampleRate(), 
			len);
		callGUI(this,[this,len](auto ed) { ed->setAudioBuffer(&m_recbuffer, getSampleRate(), len); },false);
	}
	if (m_ready_to_play == false)
	{
		setFFTSize(*getFloatParameter(cpi_fftsize));
		m_stretch_source->setProcessParameters(&m_ppar);
		m_stretch_source->setFFTWindowingType(1);
		String err;
		startplay({ *getFloatParameter(cpi_soundstart),*getFloatParameter(cpi_soundend) },
		numoutchans, samplesPerBlock, err);
		m_cur_num_out_chans = numoutchans;
		m_ready_to_play = true;
	}
}

void PaulstretchpluginAudioProcessor::releaseResources()
{
	//m_control->stopplay();
	//m_ready_to_play = false;
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool PaulstretchpluginAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    if (layouts.getMainOutputChannelSet() != AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void copyAudioBufferWrappingPosition(const AudioBuffer<float>& src, AudioBuffer<float>& dest, int destbufpos, int maxdestpos)
{
	for (int i = 0; i < dest.getNumChannels(); ++i)
	{
		int channel_to_copy = i % src.getNumChannels();
		if (destbufpos + src.getNumSamples() > maxdestpos)
		{
			int wrappos = (destbufpos + src.getNumSamples()) % maxdestpos;
			int partial_len = src.getNumSamples() - wrappos;
			dest.copyFrom(channel_to_copy, destbufpos, src, channel_to_copy, 0, partial_len);
			dest.copyFrom(channel_to_copy, partial_len, src, channel_to_copy, 0, wrappos);
		}
		else
		{
			dest.copyFrom(channel_to_copy, destbufpos, src, channel_to_copy, 0, src.getNumSamples());
		}
	}
}

void PaulstretchpluginAudioProcessor::processBlock (AudioSampleBuffer& buffer, MidiBuffer& midiMessages)
{
	ScopedLock locker(m_cs);
	ScopedNoDenormals noDenormals;
    const int totalNumInputChannels  = getTotalNumInputChannels();
    const int totalNumOutputChannels = getTotalNumOutputChannels();

    for (int i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());
	if (m_ready_to_play == false)
		return;
	if (m_is_recording == true)
	{
		int recbuflenframes = m_max_reclen * getSampleRate();
		copyAudioBufferWrappingPosition(buffer, m_recbuffer, m_rec_pos, recbuflenframes);
		callGUI(this,[this, &buffer](PaulstretchpluginAudioProcessorEditor*ed) 
		{
			ed->addAudioBlock(buffer, getSampleRate(), m_rec_pos);
		}, false);
		m_rec_pos = (m_rec_pos + buffer.getNumSamples()) % recbuflenframes;
		return;
	}
	jassert(m_buffering_source != nullptr);
	jassert(m_bufferingthread.isThreadRunning());
	m_stretch_source->setMainVolume(*getFloatParameter(cpi_main_volume));
	m_stretch_source->setRate(*getFloatParameter(cpi_stretchamount));

	setFFTSize(*getFloatParameter(cpi_fftsize));
	m_ppar.pitch_shift.cents = *getFloatParameter(cpi_pitchshift) * 100.0;
	m_ppar.freq_shift.Hz = *getFloatParameter(cpi_frequencyshift);
	m_ppar.spread.enabled = *getFloatParameter(cpi_spreadamount) > 0.0f;
	m_ppar.spread.bandwidth = *getFloatParameter(cpi_spreadamount);
    m_ppar.compressor.enabled = *getFloatParameter(cpi_compress)>0.0f;
    m_ppar.compressor.power = *getFloatParameter(cpi_compress);
	m_ppar.harmonics.enabled = *getFloatParameter(cpi_numharmonics)>=1.0;
	m_ppar.harmonics.nharmonics = *getFloatParameter(cpi_numharmonics);
	m_ppar.harmonics.freq = *getFloatParameter(cpi_harmonicsfreq);
    m_ppar.harmonics.bandwidth = *getFloatParameter(cpi_harmonicsbw);
    m_ppar.harmonics.gauss = getParameter(cpi_harmonicsgauss);
	m_ppar.octave.om2 = *getFloatParameter(cpi_octavesm2);
	m_ppar.octave.om1 = *getFloatParameter(cpi_octavesm1);
	m_ppar.octave.o0 = *getFloatParameter(cpi_octaves0);
	m_ppar.octave.o1 = *getFloatParameter(cpi_octaves1);
	m_ppar.octave.o15 = *getFloatParameter(cpi_octaves15);
	m_ppar.octave.o2 = *getFloatParameter(cpi_octaves2);
	m_ppar.octave.enabled = true;
	m_ppar.filter.low = *getFloatParameter(cpi_filter_low);
	m_ppar.filter.high = *getFloatParameter(cpi_filter_high);
	m_ppar.tonal_vs_noise.enabled = (*getFloatParameter(cpi_tonalvsnoisebw)) > 0.75;
	m_ppar.tonal_vs_noise.bandwidth = *getFloatParameter(cpi_tonalvsnoisebw);
	m_ppar.tonal_vs_noise.preserve = *getFloatParameter(cpi_tonalvsnoisepreserve);
	m_stretch_source->setOnsetDetection(*getFloatParameter(cpi_onsetdetection));
	m_stretch_source->setLoopXFadeLength(*getFloatParameter(cpi_loopxfadelen));
	double t0 = *getFloatParameter(cpi_soundstart);
	double t1 = *getFloatParameter(cpi_soundend);
	if (t0 > t1)
		std::swap(t0, t1);
	if (t1 - t0 < 0.001)
		t1 = t0 + 0.001;
	m_stretch_source->setPlayRange({ t0,t1 }, true);
	m_stretch_source->setFreezing(getParameter(cpi_freeze));
	m_stretch_source->setPaused(getParameter(cpi_pause_enabled));
	m_stretch_source->setProcessParameters(&m_ppar);
	
	AudioSourceChannelInfo aif(buffer);
	m_buffering_source->getNextAudioBlock(aif);
}

//==============================================================================
bool PaulstretchpluginAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

AudioProcessorEditor* PaulstretchpluginAudioProcessor::createEditor()
{
	return new PaulstretchpluginAudioProcessorEditor (*this);
}

//==============================================================================
void PaulstretchpluginAudioProcessor::getStateInformation (MemoryBlock& destData)
{
	ValueTree paramtree("paulstretch3pluginstate");
	for (int i=0;i<getNumParameters();++i)
	{
		auto par = getFloatParameter(i);
		if (par != nullptr)
		{
			paramtree.setProperty(par->paramID, (double)*par, nullptr);
		}
	}
	paramtree.setProperty(m_outchansparam->paramID, (int)*m_outchansparam, nullptr);
	if (m_current_file != File())
	{
		paramtree.setProperty("importedfile", m_current_file.getFullPathName(), nullptr);
	}
	MemoryOutputStream stream(destData,true);
	paramtree.writeToStream(stream);
}

void PaulstretchpluginAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
	ValueTree tree = ValueTree::readFromData(data, sizeInBytes);
	if (tree.isValid())
	{
		{
			ScopedLock locker(m_cs);
			for (int i = 0; i < getNumParameters(); ++i)
			{
				auto par = getFloatParameter(i);
				if (par != nullptr)
				{
					double parval = tree.getProperty(par->paramID, (double)*par);
					*par = parval;
				}
			}
			if (tree.hasProperty(m_outchansparam->paramID))
				*m_outchansparam = tree.getProperty(m_outchansparam->paramID, 2);

		}
		String fn = tree.getProperty("importedfile");
		if (fn.isEmpty() == false)
		{
			File f(fn);
			setAudioFile(f);
		}
	}
}

void PaulstretchpluginAudioProcessor::setRecordingEnabled(bool b)
{
	ScopedLock locker(m_cs);
	int lenbufframes = getSampleRate()*m_max_reclen;
	if (b == true)
	{
		m_using_memory_buffer = true;
		m_current_file = File();
		m_recbuffer.setSize(2, m_max_reclen*getSampleRate()+4096,false,false,true);
		m_recbuffer.clear();
		m_rec_pos = 0;
		callGUI(this,[this,lenbufframes](PaulstretchpluginAudioProcessorEditor* ed)
		{
			ed->beginAddingAudioBlocks(2, getSampleRate(), lenbufframes);
		},false);
		m_is_recording = true;
	}
	else
	{
		if (m_is_recording == true)
		{
			finishRecording(lenbufframes);
		}
	}
}

double PaulstretchpluginAudioProcessor::getRecordingPositionPercent()
{
	if (m_is_recording==false)
		return 0.0;
	return 1.0 / m_recbuffer.getNumSamples()*m_rec_pos;
}

String PaulstretchpluginAudioProcessor::setAudioFile(File f)
{
	auto ai = unique_from_raw(m_afm->createReaderFor(f));
	if (ai != nullptr)
	{
		if (ai->numChannels > 32)
		{
			//MessageManager::callAsync([cb, file]() { cb("Too many channels in file " + file.getFullPathName()); });
			return "Too many channels in file "+f.getFullPathName();
		}
		if (ai->bitsPerSample>32)
		{
			//MessageManager::callAsync([cb, file]() { cb("Too high bit depth in file " + file.getFullPathName()); });
			return "Too high bit depth in file " + f.getFullPathName();
		}
		ScopedLock locker(m_cs);
		m_stretch_source->setAudioFile(f);
		m_current_file = f;
		m_using_memory_buffer = false;
		return String();
		//MessageManager::callAsync([cb, file]() { cb(String()); });

	}
	
	return "Could not open file " + f.getFullPathName();
}

Range<double> PaulstretchpluginAudioProcessor::getTimeSelection()
{
	return { *getFloatParameter(cpi_soundstart),*getFloatParameter(cpi_soundend) };
}

double PaulstretchpluginAudioProcessor::getPreBufferingPercent()
{
	if (m_buffering_source==nullptr)
		return 0.0;
	return m_buffering_source->getPercentReady();
}

void PaulstretchpluginAudioProcessor::timerCallback(int id)
{
	if (id == 1)
	{
		bool capture = getParameter(cpi_capture_enabled);
		if (capture == true && m_is_recording == false)
		{
			setRecordingEnabled(true);
			return;
		}
		if (capture == false && m_is_recording == true)
		{
			setRecordingEnabled(false);
			return;
		}
		if (m_cur_num_out_chans != *m_outchansparam)
		{
			jassert(m_curmaxblocksize > 0);
			ScopedLock locker(m_cs);
			m_ready_to_play = false;
			m_cur_num_out_chans = *m_outchansparam;
			//Logger::writeToLog("Switching to use " + String(m_cur_num_out_chans) + " out channels");
			String err;
			startplay({ *getFloatParameter(cpi_soundstart),*getFloatParameter(cpi_soundend) },
				m_cur_num_out_chans, m_curmaxblocksize, err);
			m_ready_to_play = true;
		}
	}
}

void PaulstretchpluginAudioProcessor::finishRecording(int lenrecording)
{
	m_is_recording = false;
	m_stretch_source->setAudioBufferAsInputSource(&m_recbuffer, getSampleRate(), lenrecording);
	m_stretch_source->setPlayRange({ *getFloatParameter(cpi_soundstart),*getFloatParameter(cpi_soundend) }, true);
	auto ed = dynamic_cast<PaulstretchpluginAudioProcessorEditor*>(getActiveEditor());
	if (ed)
	{
		//ed->setAudioBuffer(&m_recbuffer, getSampleRate(), lenrecording);
	}
}

//==============================================================================
// This creates new instances of the plugin..
AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PaulstretchpluginAudioProcessor();
}
