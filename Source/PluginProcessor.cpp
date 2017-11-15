/*
  ==============================================================================

    This file was auto-generated!

    It contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#undef min
#undef max

//==============================================================================
PaulstretchpluginAudioProcessor::PaulstretchpluginAudioProcessor()
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
	m_recbuffer.setSize(2, 44100);
	m_recbuffer.clear();
	m_afm = std::make_unique<AudioFormatManager>();
	m_afm->registerBasicFormats();
	m_control = std::make_unique<Control>(m_afm.get());
	m_control->setPreBufferAmount(2);
	m_control->ppar.pitch_shift.enabled = true;
	m_control->ppar.freq_shift.enabled = true;
	m_control->setOnsetDetection(0.0);
	m_control->getStretchAudioSource()->setLoopingEnabled(true);
	addParameter(new AudioParameterFloat("mainvolume0", "Main volume", -24.0f, 12.0f, -3.0f));
	addParameter(new AudioParameterFloat("stretchamount0", "Stretch amount", 0.1f, 128.0f, 1.0f));
	addParameter(new AudioParameterFloat("fftsize0", "FFT size", 0.0f, 1.0f, 0.7f));
	addParameter(new AudioParameterFloat("pitchshift0", "Pitch shift", -24.0f, 24.0f, 0.0f));
	addParameter(new AudioParameterFloat("freqshift0", "Frequency shift", -1000.0f, 1000.0f, 0.0f));
	addParameter(new AudioParameterFloat("playrange_start0", "Sound start", 0.0f, 1.0f, 0.0f));
	addParameter(new AudioParameterFloat("playrange_end0", "Sound end", 0.0f, 1.0f, 1.0f));
}

PaulstretchpluginAudioProcessor::~PaulstretchpluginAudioProcessor()
{
	m_control->stopplay();
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
    return 0.0;
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

//==============================================================================
void PaulstretchpluginAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
	if (getNumOutputChannels() != m_cur_num_out_chans)
		m_ready_to_play = false;
	if (m_using_memory_buffer == true)
	{
		int len = jlimit(100,m_recbuffer.getNumSamples(), m_rec_pos);
		m_control->getStretchAudioSource()->setAudioBufferAsInputSource(&m_recbuffer, 
			getSampleRate(), 
			len);
		callGUI([this,len](auto ed) { ed->setAudioBuffer(&m_recbuffer, getSampleRate(), len); },false);
	}
	if (m_ready_to_play == false)
	{
		m_control->setFFTSize(0.7);
		m_control->update_player_stretch();
		m_control->update_process_parameters();
		
		String err;
		m_control->startplay(false, true,
			{ *getFloatParameter(5),*getFloatParameter(6) },
		2, err);
		m_cur_num_out_chans = getNumOutputChannels();
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
	std::lock_guard<std::mutex> locker(m_mutex);
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
		callGUI([this, &buffer](PaulstretchpluginAudioProcessorEditor*ed) 
		{
			ed->addAudioBlock(buffer, getSampleRate(), m_rec_pos);
		}, false);
		m_rec_pos = (m_rec_pos + buffer.getNumSamples()) % recbuflenframes;
		return;
	}
	m_control->getStretchAudioSource()->val_MainVolume = (float)*getFloatParameter(0);
	m_control->getStretchAudioSource()->setRate(*getFloatParameter(1));
	m_control->getStretchAudioSource()->val_XFadeLen = 0.1;
	//m_control->setFFTSize(*getFloatParameter(2));
	m_control->ppar.pitch_shift.cents = *getFloatParameter(3) * 100.0;
	m_control->ppar.freq_shift.Hz = *getFloatParameter(4);
	double t0 = *getFloatParameter(5);
	double t1 = *getFloatParameter(6);
	if (t0 > t1)
		std::swap(t0, t1);
	if (t1 - t0 < 0.001)
		t1 = t0 + 0.001;
	m_control->getStretchAudioSource()->setPlayRange({ t0,t1 }, true);
	m_control->update_process_parameters();
	m_control->processAudio(buffer);
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
		paramtree.setProperty(par->paramID, (double)*par, nullptr);
	}
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
		for (int i = 0; i<getNumParameters(); ++i)
		{
			auto par = getFloatParameter(i);
			double parval = tree.getProperty(par->paramID, (double)*par);
			*par = parval;
		}
		String fn = tree.getProperty("importedfile");
		if (fn.isEmpty() == false)
		{
			m_using_memory_buffer = false;
			File f(fn);
			setAudioFile(f);
			Timer::callAfterDelay(500, [this,f]() 
			{
				callGUI([f,this](PaulstretchpluginAudioProcessorEditor* ed)
				{
					ed->setAudioFile(f);
					ed->m_wavecomponent.setTimeSelection({ *getFloatParameter(5),*getFloatParameter(6) });
				}, false);
			});
			
		}
	}
}

void PaulstretchpluginAudioProcessor::setRecordingEnabled(bool b)
{
	std::lock_guard<std::mutex> locker(m_mutex);
	int lenbufframes = getSampleRate()*m_max_reclen;
	if (b == true)
	{
		m_using_memory_buffer = true;
		m_current_file = File();
		m_recbuffer.setSize(2, m_max_reclen*getSampleRate()+4096);
		m_recbuffer.clear();
		m_rec_pos = 0;
		callGUI([this,lenbufframes](PaulstretchpluginAudioProcessorEditor* ed) 
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
	std::lock_guard<std::mutex> locker(m_mutex);
	m_control->set_input_file(f, [this,f](String) 
	{
		
	});
	m_current_file = f;
	m_using_memory_buffer = false;
	return String();
}

void PaulstretchpluginAudioProcessor::finishRecording(int lenrecording)
{
	m_is_recording = false;
	m_control->getStretchAudioSource()->setAudioBufferAsInputSource(&m_recbuffer, getSampleRate(), lenrecording);
	m_control->getStretchAudioSource()->setPlayRange({ *getFloatParameter(5),*getFloatParameter(6) }, true);
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
