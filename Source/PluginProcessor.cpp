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
#include <set>
#include <thread>

#ifdef WIN32
#undef min
#undef max
#endif

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

inline AudioParameterFloat* make_floatpar(String id, String name, float minv, float maxv, float defv, float step, float skew)
{
	return new AudioParameterFloat(id, name, NormalisableRange<float>(minv, maxv, step, skew), defv);
}

//==============================================================================
PaulstretchpluginAudioProcessor::PaulstretchpluginAudioProcessor()
	: m_bufferingthread("pspluginprebufferthread")
{
	m_playposinfo.timeInSeconds = 0.0;
	
    m_free_filter_envelope = std::make_shared<breakpoint_envelope>();
	m_free_filter_envelope->SetName("Free filter");
	m_free_filter_envelope->AddNode({ 0.0,0.75 });
	m_free_filter_envelope->AddNode({ 1.0,0.75 });
	m_free_filter_envelope->set_reset_nodes(m_free_filter_envelope->get_all_nodes());
    m_recbuffer.setSize(2, 44100);
	m_recbuffer.clear();
	if (m_afm->getNumKnownFormats()==0)
		m_afm->registerBasicFormats();
	m_thumb = std::make_unique<AudioThumbnail>(512, *m_afm, *m_thumbcache);
	
	m_sm_enab_pars[0] = new AudioParameterBool("enab_specmodule0", "Enable harmonics", false);
	m_sm_enab_pars[1] = new AudioParameterBool("enab_specmodule1", "Enable tonal vs noise", false);
	m_sm_enab_pars[2] = new AudioParameterBool("enab_specmodule2", "Enable frequency shift", true);
	m_sm_enab_pars[3] = new AudioParameterBool("enab_specmodule3", "Enable pitch shift", true);
	m_sm_enab_pars[4] = new AudioParameterBool("enab_specmodule4", "Enable ratios", false);
	m_sm_enab_pars[5] = new AudioParameterBool("enab_specmodule5", "Enable spread", false);
	m_sm_enab_pars[6] = new AudioParameterBool("enab_specmodule6", "Enable filter", true);
	m_sm_enab_pars[7] = new AudioParameterBool("enab_specmodule7", "Enable free filter", true);
	m_sm_enab_pars[8] = new AudioParameterBool("enab_specmodule8", "Enable compressor", false);
	

	m_stretch_source = std::make_unique<StretchAudioSource>(2, m_afm,m_sm_enab_pars);
	
	m_stretch_source->setOnsetDetection(0.0);
	m_stretch_source->setLoopingEnabled(true);
	m_stretch_source->setFFTWindowingType(1);
	addParameter(make_floatpar("mainvolume0", "Main volume", -24.0, 12.0, -3.0, 0.1, 1.0)); 
	addParameter(make_floatpar("stretchamount0", "Stretch amount", 0.1, 1024.0, 2.0, 0.1, 0.25)); 
	addParameter(make_floatpar("fftsize0", "FFT size", 0.0, 1.0, 0.7, 0.01, 1.0));
	addParameter(make_floatpar("pitchshift0", "Pitch shift", -24.0f, 24.0f, 0.0f, 0.1,1.0)); // 3
	addParameter(make_floatpar("freqshift0", "Frequency shift", -1000.0f, 1000.0f, 0.0f, 1.0, 1.0)); // 4
	addParameter(make_floatpar("playrange_start0", "Sound start", 0.0f, 1.0f, 0.0f, 0.0001,1.0)); // 5
	addParameter(make_floatpar("playrange_end0", "Sound end", 0.0f, 1.0f, 1.0f, 0.0001,1.0)); // 6
	addParameter(new AudioParameterBool("freeze0", "Freeze", false)); // 7
	addParameter(make_floatpar("spread0", "Frequency spread", 0.0f, 1.0f, 0.0f, 0.001,1.0)); // 8
	addParameter(make_floatpar("compress0", "Compress", 0.0f, 1.0f, 0.0f, 0.001,1.0)); // 9
	addParameter(make_floatpar("loopxfadelen0", "Loop xfade length", 0.0f, 1.0f, 0.01f, 0.001, 1.0)); // 10
    addParameter(new AudioParameterInt("numharmonics0", "Num harmonics", 1, 100, 10)); // 11
	addParameter(make_floatpar("harmonicsfreq0", "Harmonics base freq", 1.0, 5000.0, 128.0, 0.1, 0.5));
	addParameter(make_floatpar("harmonicsbw0", "Harmonics bandwidth", 0.1f, 200.0f, 25.0f, 0.01, 1.0)); // 13
	addParameter(new AudioParameterBool("harmonicsgauss0", "Gaussian harmonics", false)); // 14
	addParameter(make_floatpar("octavemixm2_0", "2 octaves down level", 0.0f, 1.0f, 0.0f, 0.001, 1.0)); // 15
	addParameter(make_floatpar("octavemixm1_0", "Octave down level", 0.0f, 1.0f, 0.0f, 0.001, 1.0)); // 16
	addParameter(make_floatpar("octavemix0_0", "Normal pitch level", 0.0f, 1.0f, 1.0f, 0.001, 1.0)); // 17
	addParameter(make_floatpar("octavemix1_0", "1 octave up level", 0.0f, 1.0f, 0.0f, 0.001, 1.0)); // 18
	addParameter(make_floatpar("octavemix15_0", "1 octave and fifth up level", 0.0f, 1.0f, 0.0f, 0.001, 1.0)); // 19
	addParameter(make_floatpar("octavemix2_0", "2 octaves up level", 0.0f, 1.0f, 0.0f, 0.001, 1.0)); // 20
	addParameter(make_floatpar("tonalvsnoisebw_0", "Tonal vs Noise BW", 0.74f, 1.0f, 0.74f, 0.001, 1.0)); // 21
	addParameter(make_floatpar("tonalvsnoisepreserve_0", "Tonal vs Noise preserve", -1.0f, 1.0f, 0.5f, 0.001, 1.0)); // 22
	auto filt_convertFrom0To1Func = [](float rangemin, float rangemax, float value) 
	{
		if (value < 0.5f)
			return jmap<float>(value, 0.0f, 0.5f, 20.0f, 1000.0f);
		return jmap<float>(value, 0.5f, 1.0f, 1000.0f, 20000.0f);
	};
	auto filt_convertTo0To1Func = [](float rangemin, float rangemax, float value)
	{
		if (value < 1000.0f)
			return jmap<float>(value, 20.0f, 1000.0f, 0.0f, 0.5f);
		return jmap<float>(value, 1000.0f, 20000.0f, 0.5f, 1.0f);
	};
	addParameter(new AudioParameterFloat("filter_low_0", "Filter low",
                                         NormalisableRange<float>(20.0f, 20000.0f, 
											 filt_convertFrom0To1Func, filt_convertTo0To1Func), 20.0f)); // 23
	addParameter(new AudioParameterFloat("filter_high_0", "Filter high",
                                         NormalisableRange<float>(20.0f, 20000.0f, 
											 filt_convertFrom0To1Func,filt_convertTo0To1Func), 20000.0f));; // 24
	addParameter(make_floatpar("onsetdetect_0", "Onset detection", 0.0f, 1.0f, 0.0f, 0.01, 1.0)); // 25
	addParameter(new AudioParameterBool("capture_enabled0", "Capture", false)); // 26
	m_outchansparam = new AudioParameterInt("numoutchans0", "Num outs", 2, 8, 2); // 27
	addParameter(m_outchansparam); // 27
	addParameter(new AudioParameterBool("pause_enabled0", "Pause", false)); // 28
	addParameter(new AudioParameterFloat("maxcapturelen_0", "Max capture length", 1.0f, 120.0f, 10.0f)); // 29
	addParameter(new AudioParameterBool("passthrough0", "Pass input through", false)); // 30
	addParameter(new AudioParameterBool("markdirty0", "Internal (don't use)", false)); // 31
	m_inchansparam = new AudioParameterInt("numinchans0", "Num ins", 2, 8, 2); // 32
	addParameter(m_inchansparam); // 32
	addParameter(new AudioParameterBool("bypass_stretch0", "Bypass stretch", false)); // 33
	addParameter(new AudioParameterFloat("freefilter_shiftx_0", "Free filter shift X", -1.0f, 1.0f, 0.0f)); // 34
	addParameter(new AudioParameterFloat("freefilter_shifty_0", "Free filter shift Y", -1.0f, 1.0f, 0.0f)); // 35
	addParameter(new AudioParameterFloat("freefilter_scaley_0", "Free filter scale Y", -1.0f, 1.0f, 1.0f)); // 36
	addParameter(new AudioParameterFloat("freefilter_tilty_0", "Free filter tilt Y", -1.0f, 1.0f, 0.0f)); // 37
	addParameter(new AudioParameterInt("freefilter_randomybands0", "Random bands", 2, 128, 16)); // 38
	addParameter(new AudioParameterInt("freefilter_randomyrate0", "Random rate", 1, 32, 2)); // 39
	addParameter(new AudioParameterFloat("freefilter_randomyamount0", "Random amount", 0.0, 1.0, 0.0)); // 40
	for (int i = 0; i < 9; ++i) // 41-49
	{
		addParameter(m_sm_enab_pars[i]);
		m_sm_enab_pars[i]->addListener(this);
	}

	addParameter(make_floatpar("octavemix_extra0_0", "Ratio mix 7 level", 0.0f, 1.0f, 0.0f, 0.001, 1.0)); // 50
	addParameter(make_floatpar("octavemix_extra1_0", "Ratio mix 8 level", 0.0f, 1.0f, 0.0f, 0.001, 1.0)); // 51

	std::array<double,8> initialratios{ 0.25,0.5,1.0,2.0,3.0,4.0,1.5,1.0 / 1.5 };
	// 52-59
	for (int i = 0; i < 8; ++i)
	{
		addParameter(make_floatpar("ratiomix_ratio_"+String(i)+"_0", "Ratio mix ratio "+String(i+1), 0.125f, 8.0f, 
			initialratios[i], 
			0.001, 
			1.0)); 
	}

	addParameter(new AudioParameterBool("loop_enabled0", "Loop", true)); // 60
	addParameter(new AudioParameterBool("rewind0", "Rewind", false)); // 61
	auto dprate_convertFrom0To1Func = [](float rangemin, float rangemax, float value)
	{
		if (value < 0.5f)
			return jmap<float>(value, 0.0f, 0.5f, 0.1f, 1.0f);
		return jmap<float>(value, 0.5f, 1.0f, 1.0f, 8.0f);
	};
	auto dprate_convertTo0To1Func = [](float rangemin, float rangemax, float value)
	{
		if (value < 1.0f)
			return jmap<float>(value, 0.1f, 1.0f, 0.0f, 0.5f);
		return jmap<float>(value, 1.0f, 8.0f, 0.5f, 1.0f);
	};
	addParameter(new AudioParameterFloat("dryplayrate0", "Dry playrate",
		NormalisableRange<float>(0.1f, 8.0f,
			dprate_convertFrom0To1Func, dprate_convertTo0To1Func), 1.0f)); // 62
	auto& pars = getParameters();
	for (const auto& p : pars)
		m_reset_pars.push_back(p->getValue());
	setPreBufferAmount(2);
    startTimer(1, 50);
	m_show_technical_info = m_propsfile->m_props_file->getBoolValue("showtechnicalinfo", false);
	
}

PaulstretchpluginAudioProcessor::~PaulstretchpluginAudioProcessor()
{
	//Logger::writeToLog("PaulX AudioProcessor destroyed");
	m_thumb->removeAllChangeListeners();
	m_thumb = nullptr;
	m_bufferingthread.stopThread(1000);
}

void PaulstretchpluginAudioProcessor::resetParameters()
{
	ScopedLock locker(m_cs);
	for (int i = 0; i < m_reset_pars.size(); ++i)
	{
		if (i!=cpi_main_volume && i!=cpi_passthrough)
			setParameter(i, m_reset_pars[i]);
	}
}

void PaulstretchpluginAudioProcessor::setPreBufferAmount(int x)
{
	int temp = jlimit(0, 5, x);
	if (temp != m_prebuffer_amount || m_use_backgroundbuffering == false)
	{
        m_use_backgroundbuffering = true;
        m_prebuffer_amount = temp;
		m_recreate_buffering_source = true;
        ScopedLock locker(m_cs);
        m_prebuffering_inited = false;
        m_cur_num_out_chans = *m_outchansparam;
        //Logger::writeToLog("Switching to use " + String(m_cur_num_out_chans) + " out channels");
        String err;
        startplay({ *getFloatParameter(cpi_soundstart),*getFloatParameter(cpi_soundend) },
                  m_cur_num_out_chans, m_curmaxblocksize, err);
        m_prebuffering_inited = true;
	}
}

int PaulstretchpluginAudioProcessor::getPreBufferAmount()
{
	if (m_use_backgroundbuffering == false)
		return -1;
	return m_prebuffer_amount;
}

ValueTree PaulstretchpluginAudioProcessor::getStateTree(bool ignoreoptions, bool ignorefile)
{
	ValueTree paramtree("paulstretch3pluginstate");
	storeToTreeProperties(paramtree, nullptr, getParameters());
    if (m_current_file != File() && ignorefile == false)
	{
		paramtree.setProperty("importedfile", m_current_file.getFullPathName(), nullptr);
	}
	auto specorder = m_stretch_source->getSpectrumProcessOrder();
	paramtree.setProperty("numspectralstagesb", (int)specorder.size(), nullptr);
	for (int i = 0; i < specorder.size(); ++i)
	{
		paramtree.setProperty("specorderb" + String(i), specorder[i].m_index, nullptr);
	}
	if (ignoreoptions == false)
	{
		if (m_use_backgroundbuffering)
			paramtree.setProperty("prebufamount", m_prebuffer_amount, nullptr);
		else
			paramtree.setProperty("prebufamount", -1, nullptr);
		paramtree.setProperty("loadfilewithstate", m_load_file_with_state, nullptr);
		storeToTreeProperties(paramtree, nullptr, "playwhenhostrunning", m_play_when_host_plays, "capturewhenhostrunning", m_capture_when_host_plays);
		storeToTreeProperties(paramtree, nullptr, "mutewhilecapturing", m_mute_while_capturing);
	}
	storeToTreeProperties(paramtree, nullptr, "tabaindex", m_cur_tab_index);
	storeToTreeProperties(paramtree, nullptr, "waveviewrange", m_wave_view_range);
    ValueTree freefilterstate = m_free_filter_envelope->saveState(Identifier("freefilter_envelope"));
    paramtree.addChild(freefilterstate, -1, nullptr);
    return paramtree;
}

void PaulstretchpluginAudioProcessor::setStateFromTree(ValueTree tree)
{
	if (tree.isValid())
	{
		{
			ScopedLock locker(m_cs);
            ValueTree freefilterstate = tree.getChildWithName("freefilter_envelope");
            m_free_filter_envelope->restoreState(freefilterstate);
            m_load_file_with_state = tree.getProperty("loadfilewithstate", true);
			getFromTreeProperties(tree, "playwhenhostrunning", m_play_when_host_plays, 
				"capturewhenhostrunning", m_capture_when_host_plays,"mutewhilecapturing",m_mute_while_capturing);
			getFromTreeProperties(tree, "tabaindex", m_cur_tab_index);
			if (tree.hasProperty("numspectralstagesb"))
			{
				std::vector<SpectrumProcess> old_order = m_stretch_source->getSpectrumProcessOrder();
				std::vector<SpectrumProcess> new_order;
				int ordersize = tree.getProperty("numspectralstagesb");
				if (ordersize == old_order.size())
				{
					for (int i = 0; i < ordersize; ++i)
					{
						int index = tree.getProperty("specorderb" + String(i));
						new_order.push_back({ index, old_order[index].m_enabled });
					}
					m_stretch_source->setSpectrumProcessOrder(new_order);
				}
			}
			getFromTreeProperties(tree, "waveviewrange", m_wave_view_range);
			getFromTreeProperties(tree, getParameters());
			
        }
		int prebufamt = tree.getProperty("prebufamount", 2);
		if (prebufamt == -1)
			m_use_backgroundbuffering = false;
		else
			setPreBufferAmount(prebufamt);
		if (m_load_file_with_state == true)
		{
			String fn = tree.getProperty("importedfile");
			if (fn.isEmpty() == false)
			{
				File f(fn);
				setAudioFile(f);
			}
		}
		m_state_dirty = true;
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
	return 0.0;
	//return (double)m_bufamounts[m_prebuffer_amount]/getSampleRate();
}

int PaulstretchpluginAudioProcessor::getNumPrograms()
{
	return 1;
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
	return String();
}

void PaulstretchpluginAudioProcessor::changeProgramName (int index, const String& newName)
{
}

void PaulstretchpluginAudioProcessor::parameterValueChanged(int parameterIndex, float newValue)
{
	if (parameterIndex >= cpi_enable_spec_module0 && parameterIndex <= cpi_enable_spec_module8)
	{
		m_stretch_source->setSpectralModuleEnabled(parameterIndex - cpi_enable_spec_module0, newValue >= 0.5);
	}
}

void PaulstretchpluginAudioProcessor::parameterGestureChanged(int parameterIndex, bool gestureIsStarting)
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
	m_stretch_source->setPlayRange(playrange);
	m_stretch_source->setFreeFilterEnvelope(m_free_filter_envelope);
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
	m_buffering_source->prepareToPlay(maxBlockSize, getSampleRateChecked());
}

void PaulstretchpluginAudioProcessor::setParameters(const std::vector<double>& pars)
{
	ScopedLock locker(m_cs);
	for (int i = 0; i < getNumParameters(); ++i)
	{
		if (i<pars.size())
			setParameter(i, pars[i]);
	}
}

void PaulstretchpluginAudioProcessor::updateStretchParametersFromPluginParameters(ProcessParameters & pars)
{
	pars.pitch_shift.cents = *getFloatParameter(cpi_pitchshift) * 100.0;
	pars.freq_shift.Hz = *getFloatParameter(cpi_frequencyshift);

	pars.spread.bandwidth = *getFloatParameter(cpi_spreadamount);

	pars.compressor.power = *getFloatParameter(cpi_compress);

	pars.harmonics.nharmonics = *getIntParameter(cpi_numharmonics);
	pars.harmonics.freq = *getFloatParameter(cpi_harmonicsfreq);
	pars.harmonics.bandwidth = *getFloatParameter(cpi_harmonicsbw);
	pars.harmonics.gauss = getParameter(cpi_harmonicsgauss);

	pars.octave.om2 = *getFloatParameter(cpi_octavesm2);
	pars.octave.om1 = *getFloatParameter(cpi_octavesm1);
	pars.octave.o0 = *getFloatParameter(cpi_octaves0);
	pars.octave.o1 = *getFloatParameter(cpi_octaves1);
	pars.octave.o15 = *getFloatParameter(cpi_octaves15);
	pars.octave.o2 = *getFloatParameter(cpi_octaves2);

	pars.ratiomix.ratiolevels[0]= *getFloatParameter(cpi_octavesm2);
	pars.ratiomix.ratiolevels[1] = *getFloatParameter(cpi_octavesm1);
	pars.ratiomix.ratiolevels[2] = *getFloatParameter(cpi_octaves0);
	pars.ratiomix.ratiolevels[3] = *getFloatParameter(cpi_octaves1);
	pars.ratiomix.ratiolevels[4] = *getFloatParameter(cpi_octaves15);
	pars.ratiomix.ratiolevels[5] = *getFloatParameter(cpi_octaves2);
	pars.ratiomix.ratiolevels[6] = *getFloatParameter(cpi_octaves_extra1);
	pars.ratiomix.ratiolevels[7] = *getFloatParameter(cpi_octaves_extra2);

	for (int i = 0; i < 8; ++i)
		pars.ratiomix.ratios[i] = *getFloatParameter((int)cpi_octaves_ratio0 + i);

	pars.filter.low = *getFloatParameter(cpi_filter_low);
	pars.filter.high = *getFloatParameter(cpi_filter_high);

	pars.tonal_vs_noise.bandwidth = *getFloatParameter(cpi_tonalvsnoisebw);
	pars.tonal_vs_noise.preserve = *getFloatParameter(cpi_tonalvsnoisepreserve);
}

String PaulstretchpluginAudioProcessor::offlineRender(File outputfile)
{
	File outputfiletouse = outputfile.getNonexistentSibling();
	ValueTree state = getStateTree(false, false);
	auto processor = std::make_shared<PaulstretchpluginAudioProcessor>();
	processor->setStateFromTree(state);
	int blocksize = 2048;
	int numoutchans = *processor->getIntParameter(cpi_num_outchans);
	processor->prepareToPlay(44100.0, blocksize);
	double t0 = *processor->getFloatParameter(cpi_soundstart);
	double t1 = *processor->getFloatParameter(cpi_soundend);
	sanitizeTimeRange(t0, t1);
	double outsr = processor->getSampleRateChecked();
	WavAudioFormat wavformat;
	FileOutputStream* outstream = outputfiletouse.createOutputStream();
	if (outstream == nullptr)
		return "Could not create output file";
	auto writer = wavformat.createWriterFor(outstream, getSampleRateChecked(), numoutchans, 32, StringPairArray(), 0);
	if (writer == nullptr)
	{
		delete outstream;
		return "Could not create WAV writer";
	}
	auto rendertask = [processor,writer,blocksize,numoutchans, outsr, this]()
	{
		AudioBuffer<float> renderbuffer(numoutchans, blocksize);
		MidiBuffer dummymidi;
		int64_t outlen = 10 * outsr;
		int64_t outcounter = 0;
		AudioSourceChannelInfo asci(renderbuffer);
		m_offline_render_state = 0;
		m_offline_render_cancel_requested = false;
		while (outcounter < outlen)
		{
			if (m_offline_render_cancel_requested == true)
				break;
			processor->processBlock(renderbuffer, dummymidi);
			writer->writeFromAudioSampleBuffer(renderbuffer, 0, blocksize);
			outcounter += blocksize;
			m_offline_render_state = 100.0 / outlen * outcounter;
		}
		m_offline_render_state = 200;
		delete writer;
	};
	std::thread th(rendertask);
	th.detach();
	return "Rendered OK";
}

double PaulstretchpluginAudioProcessor::getSampleRateChecked()
{
	if (m_cur_sr < 1.0 || m_cur_sr>1000000.0)
		return 44100.0;
	return m_cur_sr;
}

void PaulstretchpluginAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    ++m_prepare_count;
    ScopedLock locker(m_cs);
	m_cur_sr = sampleRate;
	m_curmaxblocksize = samplesPerBlock;
	m_input_buffer.setSize(getMainBusNumInputChannels(), samplesPerBlock);
	*getBoolParameter(cpi_rewind) = false;
	m_lastrewind = false;
	int numoutchans = *m_outchansparam;
	if (numoutchans != m_cur_num_out_chans)
		m_prebuffering_inited = false;
	if (m_using_memory_buffer == true)
	{
		int len = jlimit(100,m_recbuffer.getNumSamples(), 
			int(getSampleRateChecked()*(*getFloatParameter(cpi_max_capture_len))));
		m_stretch_source->setAudioBufferAsInputSource(&m_recbuffer, 
			getSampleRateChecked(), 
			len);
		//m_thumb->reset(m_recbuffer.getNumChannels(), sampleRate, len);
	}
	if (m_prebuffering_inited == false)
	{
		setFFTSize(*getFloatParameter(cpi_fftsize));
		m_stretch_source->setProcessParameters(&m_ppar);
		m_stretch_source->setFFTWindowingType(1);
		String err;
		startplay({ *getFloatParameter(cpi_soundstart),*getFloatParameter(cpi_soundend) },
		numoutchans, samplesPerBlock, err);
		m_cur_num_out_chans = numoutchans;
		m_prebuffering_inited = true;
	}
	else
	{
		m_buffering_source->prepareToPlay(samplesPerBlock, getSampleRateChecked());
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

/*
void PaulstretchpluginAudioProcessor::processBlock (AudioBuffer<double>& buffer, MidiBuffer&)
{
    jassert(false);
}
*/

void PaulstretchpluginAudioProcessor::processBlock (AudioSampleBuffer& buffer, MidiBuffer& midiMessages)
{
	
	ScopedLock locker(m_cs);
	const int totalNumInputChannels = getTotalNumInputChannels();
	const int totalNumOutputChannels = getTotalNumOutputChannels();
	AudioPlayHead* phead = getPlayHead();
	if (phead != nullptr)
	{
		phead->getCurrentPosition(m_playposinfo);
	}
	else
		m_playposinfo.isPlaying = false;
	ScopedNoDenormals noDenormals;
	double srtemp = getSampleRate();
	if (srtemp != m_cur_sr)
		m_cur_sr = srtemp;
	m_prebufsmoother.setSlope(0.9, srtemp / buffer.getNumSamples());
	m_smoothed_prebuffer_ready = m_prebufsmoother.process(m_buffering_source->getPercentReady());
	
	for (int i = 0; i < totalNumInputChannels; ++i)
		m_input_buffer.copyFrom(i, 0, buffer, i, 0, buffer.getNumSamples());
    for (int i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());
	if (m_prebuffering_inited == false)
		return;
	if (m_is_recording == true)
	{
		if (m_playposinfo.isPlaying == false && m_capture_when_host_plays == true)
			return;
		int recbuflenframes = m_max_reclen * getSampleRate();
		copyAudioBufferWrappingPosition(buffer, m_recbuffer, m_rec_pos, recbuflenframes);
		m_thumb->addBlock(m_rec_pos, buffer, 0, buffer.getNumSamples());
		m_rec_pos = (m_rec_pos + buffer.getNumSamples()) % recbuflenframes;
		m_rec_count += buffer.getNumSamples();
		if (m_rec_count<recbuflenframes)
			m_recorded_range = { 0, m_rec_count };
		if (m_mute_while_capturing == true)
			buffer.clear();
		return;
	}
	jassert(m_buffering_source != nullptr);
	jassert(m_bufferingthread.isThreadRunning());
	double t0 = *getFloatParameter(cpi_soundstart);
	double t1 = *getFloatParameter(cpi_soundend);
	sanitizeTimeRange(t0, t1);
	m_stretch_source->setPlayRange({ t0,t1 });
	if (m_last_host_playing == false && m_playposinfo.isPlaying)
	{
		m_stretch_source->seekPercent(*getFloatParameter(cpi_soundstart));
		m_last_host_playing = true;
	}
	else if (m_last_host_playing == true && m_playposinfo.isPlaying == false)
	{
		m_last_host_playing = false;
	}
	if (m_play_when_host_plays == true && m_playposinfo.isPlaying == false)
		return;
	m_free_filter_envelope->m_transform_x_shift = *getFloatParameter(cpi_freefilter_shiftx);
	m_free_filter_envelope->m_transform_y_shift = *getFloatParameter(cpi_freefilter_shifty);
	m_free_filter_envelope->m_transform_y_scale = *getFloatParameter(cpi_freefilter_scaley);
	m_free_filter_envelope->m_transform_y_tilt = *getFloatParameter(cpi_freefilter_tilty);
	m_free_filter_envelope->m_transform_y_random_bands = *getIntParameter(cpi_freefilter_randomy_numbands);
	m_free_filter_envelope->m_transform_y_random_rate = *getIntParameter(cpi_freefilter_randomy_rate);
	m_free_filter_envelope->m_transform_y_random_amount = *getFloatParameter(cpi_freefilter_randomy_amount);

	//m_stretch_source->setSpectralModulesEnabled(m_sm_enab_pars);

	if (m_stretch_source->isLoopEnabled() != *getBoolParameter(cpi_looping_enabled))
		m_stretch_source->setLoopingEnabled(*getBoolParameter(cpi_looping_enabled));
	bool rew = *getBoolParameter(cpi_rewind);
	if (rew != m_lastrewind)
	{
		if (rew == true)
		{
			*getBoolParameter(cpi_rewind) = false;
			m_stretch_source->seekPercent(m_stretch_source->getPlayRange().getStart());
		}
		m_lastrewind = rew;
	}

	m_stretch_source->setMainVolume(*getFloatParameter(cpi_main_volume));
	m_stretch_source->setRate(*getFloatParameter(cpi_stretchamount));
	m_stretch_source->setPreviewDry(*getBoolParameter(cpi_bypass_stretch));
	m_stretch_source->setDryPlayrate(*getFloatParameter(cpi_dryplayrate));
	setFFTSize(*getFloatParameter(cpi_fftsize));
	
	updateStretchParametersFromPluginParameters(m_ppar);

	m_stretch_source->setOnsetDetection(*getFloatParameter(cpi_onsetdetection));
	m_stretch_source->setLoopXFadeLength(*getFloatParameter(cpi_loopxfadelen));
	
	
	
	m_stretch_source->setFreezing(getParameter(cpi_freeze));
	m_stretch_source->setPaused(getParameter(cpi_pause_enabled));
	m_stretch_source->setProcessParameters(&m_ppar);
	AudioSourceChannelInfo aif(buffer);
	if (isNonRealtime() || m_use_backgroundbuffering == false)
	{
		m_stretch_source->getNextAudioBlock(aif);
	}
	else
	{
		m_buffering_source->getNextAudioBlock(aif);
	}
	if (m_is_recording == false && getParameter(cpi_passthrough) > 0.5f)
	{
		for (int i = 0; i < totalNumInputChannels; ++i)
		{
			buffer.addFrom(i, 0, m_input_buffer, i, 0, buffer.getNumSamples());
		}
	}
	for (int i = 0; i < buffer.getNumChannels(); ++i)
	{
		for (int j = 0; j < buffer.getNumSamples(); ++j)
		{
			float sample = buffer.getSample(i,j);
			if (std::isnan(sample) || std::isinf(sample))
				++m_abnormal_output_samples;
		}
	}
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
	ValueTree paramtree = getStateTree(false,false);
	MemoryOutputStream stream(destData,true);
	paramtree.writeToStream(stream);
}

void PaulstretchpluginAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
	ValueTree tree = ValueTree::readFromData(data, sizeInBytes);
	setStateFromTree(tree);
}

void PaulstretchpluginAudioProcessor::setDirty()
{
	toggleBool(getBoolParameter(cpi_markdirty));
}

void PaulstretchpluginAudioProcessor::setRecordingEnabled(bool b)
{
	ScopedLock locker(m_cs);
	int lenbufframes = getSampleRateChecked()*m_max_reclen;
	if (b == true)
	{
		m_using_memory_buffer = true;
		m_current_file = File();
		int numchans = *m_inchansparam;
		m_recbuffer.setSize(numchans, m_max_reclen*getSampleRateChecked()+4096,false,false,true);
		m_recbuffer.clear();
		m_rec_pos = 0;
		m_thumb->reset(m_recbuffer.getNumChannels(), getSampleRateChecked(), lenbufframes);
		m_is_recording = true;
		m_recorded_range = Range<int>();
		m_rec_count = 0;
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
		if (ai->numChannels > 8)
		{
			return "Too many channels in file "+f.getFullPathName();
		}
		if (ai->bitsPerSample>32)
		{
			return "Too high bit depth in file " + f.getFullPathName();
		}
		m_thumb->setSource(new FileInputSource(f));
		ScopedLock locker(m_cs);
		m_stretch_source->setAudioFile(f);
		//Range<double> currange{ *getFloatParameter(cpi_soundstart),*getFloatParameter(cpi_soundend) };
		//if (currange.contains(m_stretch_source->getInfilePositionPercent())==false)
			m_stretch_source->seekPercent(*getFloatParameter(cpi_soundstart));
		m_current_file = f;
        m_current_file_date = m_current_file.getLastModificationTime();
		m_using_memory_buffer = false;
		setDirty();
		return String();
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
	return m_smoothed_prebuffer_ready;
}

void PaulstretchpluginAudioProcessor::timerCallback(int id)
{
	if (id == 1)
	{
		bool capture = *getBoolParameter(cpi_capture_enabled);
		if (capture == false && m_max_reclen != *getFloatParameter(cpi_max_capture_len))
		{
			m_max_reclen = *getFloatParameter(cpi_max_capture_len);
			//Logger::writeToLog("Changing max capture len to " + String(m_max_reclen));
		}
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
			m_prebuffering_inited = false;
			m_cur_num_out_chans = *m_outchansparam;
			//Logger::writeToLog("Switching to use " + String(m_cur_num_out_chans) + " out channels");
			String err;
			startplay({ *getFloatParameter(cpi_soundstart),*getFloatParameter(cpi_soundend) },
				m_cur_num_out_chans, m_curmaxblocksize, err);
			m_prebuffering_inited = true;
		}
	}
}

pointer_sized_int PaulstretchpluginAudioProcessor::handleVstPluginCanDo(int32 index, pointer_sized_int value, void * ptr, float opt)
{
	if (strcmp((char*)ptr, "xenakios") == 0)
	{
		if (index == 0 && (void*)value!=nullptr)
		{
			double t0 = *getFloatParameter(cpi_soundstart);
			double t1 = *getFloatParameter(cpi_soundend);
			double outlen = (t1-t0)*m_stretch_source->getInfileLengthSeconds()*(*getFloatParameter(cpi_stretchamount));
			//std::cout << "host requested output length, result " << outlen << "\n";
			*((double*)value) = outlen;
		}
		if (index == 1 && (void*)value!=nullptr)
		{
			String fn(CharPointer_UTF8((char*)value));
			//std::cout << "host requested to set audio file " << fn << "\n";
			auto err = setAudioFile(File(fn));
			if (err.isEmpty()==false)
				std::cout << err << "\n";
		}
		return 1;
	}
	
	return pointer_sized_int();
}

pointer_sized_int PaulstretchpluginAudioProcessor::handleVstManufacturerSpecific(int32 index, pointer_sized_int value, void * ptr, float opt)
{
	return pointer_sized_int();
}

void PaulstretchpluginAudioProcessor::finishRecording(int lenrecording)
{
	m_is_recording = false;
	m_stretch_source->setAudioBufferAsInputSource(&m_recbuffer, getSampleRateChecked(), lenrecording);
	*getFloatParameter(cpi_soundstart) = 0.0f;
	*getFloatParameter(cpi_soundend) = jlimit<double>(0.01, 1.0, 1.0 / lenrecording * m_rec_count);
}

AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PaulstretchpluginAudioProcessor();
}
