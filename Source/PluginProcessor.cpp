// SPDX-License-Identifier: GPLv3-or-later WITH Appstore-exception
// Copyright (C) 2017 Xenakios
// Copyright (C) 2020 Jesse Chappell


#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <set>
#include <thread>

#include "CrossPlatformUtils.h"

#ifdef WIN32
#undef min
#undef max
#endif

int get_optimized_updown(int n, bool up) {
	int orig_n = n;
	while (true) {
		n = orig_n;

#if PS_USE_VDSP_FFT
        // only powers of two allowed if using VDSP FFT
#elif PS_USE_PFFFT
        // only powers of two allowed if using pffft
#else
        while (!(n % 11)) n /= 11;
		while (!(n % 7)) n /= 7;
		while (!(n % 5)) n /= 5;
		while (!(n % 3)) n /= 3;
#endif

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
	return new AudioParameterFloat(juce::ParameterID(id, 1), name, NormalisableRange<float>(minv, maxv, step, skew), defv);
}

#if JUCE_IOS
#define ALTBUS_ACTIVE true
#else
#define ALTBUS_ACTIVE false
#endif

PaulstretchpluginAudioProcessor::BusesProperties PaulstretchpluginAudioProcessor::getDefaultLayout()
{
    auto props = PaulstretchpluginAudioProcessor::BusesProperties();
    auto plugtype = PluginHostType::getPluginLoadedAs();

    // common to all
    props = props.withInput  ("Main In",  AudioChannelSet::stereo(), true)
    .withOutput ("Main Out", AudioChannelSet::stereo(), true);


    // extra inputs
    if (plugtype == AudioProcessor::wrapperType_AAX) {
        // only one sidechain mono allowed, doesn't even work anyway
        props = props.withInput ("Aux 1 In", AudioChannelSet::mono(), ALTBUS_ACTIVE);
    }
    else {
        // throw in some input sidechains
        props = props.withInput  ("Aux 1 In",  AudioChannelSet::stereo(), ALTBUS_ACTIVE)
        .withInput  ("Aux 2 In",  AudioChannelSet::stereo(), ALTBUS_ACTIVE)
        .withInput  ("Aux 3 In",  AudioChannelSet::stereo(), ALTBUS_ACTIVE)
        .withInput  ("Aux 4 In",  AudioChannelSet::stereo(), ALTBUS_ACTIVE)
        .withInput  ("Aux 5 In",  AudioChannelSet::stereo(), ALTBUS_ACTIVE)
        .withInput  ("Aux 6 In",  AudioChannelSet::stereo(), ALTBUS_ACTIVE)
        .withInput  ("Aux 7 In",  AudioChannelSet::stereo(), ALTBUS_ACTIVE)
        .withInput  ("Aux 8 In",  AudioChannelSet::stereo(), ALTBUS_ACTIVE);
    }

    // outputs

    props = props.withOutput ("Aux 1 Out", AudioChannelSet::stereo(), ALTBUS_ACTIVE)
        .withOutput ("Aux 2 Out", AudioChannelSet::stereo(), ALTBUS_ACTIVE)
        .withOutput ("Aux 3 Out", AudioChannelSet::stereo(), ALTBUS_ACTIVE)
        .withOutput ("Aux 4 Out", AudioChannelSet::stereo(), ALTBUS_ACTIVE)
        .withOutput ("Aux 5 Out", AudioChannelSet::stereo(), ALTBUS_ACTIVE)
        .withOutput ("Aux 6 Out", AudioChannelSet::stereo(), ALTBUS_ACTIVE)
        .withOutput ("Aux 7 Out", AudioChannelSet::stereo(), ALTBUS_ACTIVE)
        .withOutput ("Aux 8 Out", AudioChannelSet::stereo(), ALTBUS_ACTIVE);

    return props;
}

//==============================================================================
PaulstretchpluginAudioProcessor::PaulstretchpluginAudioProcessor(bool is_stand_alone_offline)
	: AudioProcessor(getDefaultLayout()),
m_bufferingthread("pspluginprebufferthread"), m_is_stand_alone_offline(is_stand_alone_offline)
{
    DBG("Attempt proc const");

	m_filechoose_callback = [this](const FileChooser& chooser)
	{
		URL resu = chooser.getURLResult();
		//String pathname = resu.getFullPathName();
		//if (pathname.startsWith("/localhost"))
		//{
		//	pathname = pathname.substring(10);
		//	resu = File(pathname);
		//}
        if (!resu.isEmpty()) {
            m_propsfile->m_props_file->setValue("importfilefolder", resu.getLocalFile().getParentDirectory().getFullPathName());
            String loaderr = setAudioFile(resu);
            if (auto ed = dynamic_cast<PaulstretchpluginAudioProcessorEditor*>(getActiveEditor()); ed != nullptr)
            {
                ed->m_last_err = loaderr;
            }
        }
	};
	m_playposinfo.timeInSeconds = 0.0;
	
    m_free_filter_envelope = std::make_shared<breakpoint_envelope>();
	m_free_filter_envelope->SetName("Free filter");
	m_free_filter_envelope->AddNode({ 0.0,0.75 });
	m_free_filter_envelope->AddNode({ 1.0,0.75 });
	m_free_filter_envelope->set_reset_nodes(m_free_filter_envelope->get_all_nodes());

    DBG("recbuffer");

    m_recbuffer.setSize(2, 48000);
	m_recbuffer.clear();
	if (m_afm->getNumKnownFormats()==0)
		m_afm->registerBasicFormats();
	if (m_is_stand_alone_offline == false)
		m_thumb = std::make_unique<AudioThumbnail>(512, *m_afm, *m_thumbcache);

    DBG("making bool pars");

	m_sm_enab_pars[0] = new AudioParameterBool(ParameterID("enab_specmodule0", 1), "Enable harmonics", false);
	m_sm_enab_pars[1] = new AudioParameterBool(ParameterID("enab_specmodule1", 1), "Enable tonal vs noise", false);
	m_sm_enab_pars[2] = new AudioParameterBool(ParameterID("enab_specmodule2", 1), "Enable frequency shift", true);
	m_sm_enab_pars[3] = new AudioParameterBool(ParameterID("enab_specmodule3", 1), "Enable pitch shift", true);
	m_sm_enab_pars[4] = new AudioParameterBool(ParameterID("enab_specmodule4", 1), "Enable ratios", false);
	m_sm_enab_pars[5] = new AudioParameterBool(ParameterID("enab_specmodule5", 1), "Enable spread", false);
	m_sm_enab_pars[6] = new AudioParameterBool(ParameterID("enab_specmodule6", 1), "Enable filter", false);
	m_sm_enab_pars[7] = new AudioParameterBool(ParameterID("enab_specmodule7", 1), "Enable free filter", false);
	m_sm_enab_pars[8] = new AudioParameterBool(ParameterID("enab_specmodule8", 1), "Enable compressor", false);
	
    DBG("making stretch source");

	m_stretch_source = std::make_unique<StretchAudioSource>(2, m_afm,m_sm_enab_pars);
	
	m_stretch_source->setOnsetDetection(0.0);
	m_stretch_source->setLoopingEnabled(true);
	m_stretch_source->setFFTWindowingType(1);

    DBG("About to add parameters");


	addParameter(make_floatpar("mainvolume0", "Main volume", -24.0, 12.0, -3.0, 0.1, 1.0)); 
	addParameter(make_floatpar("stretchamount0", "Stretch amount", 0.1, 1024.0, 2.0, 0.1, 0.25)); 
	addParameter(make_floatpar("fftsize0", "FFT size", 0.0, 1.0, 0.7, 0.01, 1.0));
	addParameter(make_floatpar("pitchshift0", "Pitch shift", -24.0f, 24.0f, 0.0f, 0.1,1.0)); // 3
	addParameter(make_floatpar("freqshift0", "Frequency shift", -1000.0f, 1000.0f, 0.0f, 1.0, 1.0)); // 4
	addParameter(make_floatpar("playrange_start0", "Sound start", 0.0f, 1.0f, 0.0f, 0.0001,1.0)); // 5
	addParameter(make_floatpar("playrange_end0", "Sound end", 0.0f, 1.0f, 1.0f, 0.0001,1.0)); // 6
	addParameter(new AudioParameterBool(ParameterID("freeze0", 1), "Freeze", false)); // 7
	addParameter(make_floatpar("spread0", "Frequency spread", 0.0f, 1.0f, 0.0f, 0.001,1.0)); // 8
	addParameter(make_floatpar("compress0", "Compress", 0.0f, 1.0f, 0.0f, 0.001,1.0)); // 9
	addParameter(make_floatpar("loopxfadelen0", "Loop xfade length", 0.0f, 1.0f, 0.01f, 0.001, 1.0)); // 10
    addParameter(new AudioParameterInt(ParameterID("numharmonics0", 1), "Num harmonics", 1, 100, 10)); // 11
	addParameter(make_floatpar("harmonicsfreq0", "Harmonics base freq", 1.0, 5000.0, 128.0, 0.1, 0.5));
	addParameter(make_floatpar("harmonicsbw0", "Harmonics bandwidth", 0.1f, 200.0f, 25.0f, 0.01, 1.0)); // 13
	addParameter(new AudioParameterBool(ParameterID("harmonicsgauss0", 1), "Gaussian harmonics", false)); // 14
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
	addParameter(new AudioParameterFloat(ParameterID("filter_low_0", 1), "Filter low",
                                         NormalisableRange<float>(20.0f, 20000.0f, 
											 filt_convertFrom0To1Func, filt_convertTo0To1Func), 20.0f)); // 23
	addParameter(new AudioParameterFloat(ParameterID("filter_high_0", 1), "Filter high",
                                         NormalisableRange<float>(20.0f, 20000.0f, 
											 filt_convertFrom0To1Func,filt_convertTo0To1Func), 20000.0f));; // 24
	addParameter(make_floatpar("onsetdetect_0", "Onset detection", 0.0f, 1.0f, 0.0f, 0.01, 1.0)); // 25
	addParameter(new AudioParameterBool(ParameterID("capture_enabled0", 1), "Capture", false)); // 26
	m_outchansparam = new AudioParameterInt(ParameterID("numoutchans0", 1), "Num outs", 1, 32, 2); // 27
	addParameter(m_outchansparam); // 27
	addParameter(new AudioParameterBool(ParameterID("pause_enabled0", 1), "Pause", true)); // 28
	addParameter(new AudioParameterFloat(ParameterID("maxcapturelen_0", 1), "Max capture length", 1.0f, 120.0f, 10.0f)); // 29
	addParameter(new AudioParameterBool(ParameterID("passthrough0", 1), "Pass input through", false)); // 30
	addParameter(new AudioParameterBool(ParameterID("markdirty0", 1), "Internal (don't use)", false)); // 31
	m_inchansparam = new AudioParameterInt(ParameterID("numinchans0", 1), "Num ins", 1, 32, 2); // 32
	addParameter(m_inchansparam); // 32
	addParameter(new AudioParameterBool(ParameterID("bypass_stretch0", 1), "Bypass stretch", false)); // 33
	addParameter(new AudioParameterFloat(ParameterID("freefilter_shiftx_0", 1), "Free filter shift X", -1.0f, 1.0f, 0.0f)); // 34
	addParameter(new AudioParameterFloat(ParameterID("freefilter_shifty_0", 1), "Free filter shift Y", -1.0f, 1.0f, 0.0f)); // 35
	addParameter(new AudioParameterFloat(ParameterID("freefilter_scaley_0", 1), "Free filter scale Y", -1.0f, 1.0f, 1.0f)); // 36
	addParameter(new AudioParameterFloat(ParameterID("freefilter_tilty_0", 1), "Free filter tilt Y", -1.0f, 1.0f, 0.0f)); // 37
	addParameter(new AudioParameterInt(ParameterID("freefilter_randomybands0", 1), "Random bands", 2, 128, 16)); // 38
	addParameter(new AudioParameterInt(ParameterID("freefilter_randomyrate0", 1), "Random rate", 1, 32, 2)); // 39
	addParameter(new AudioParameterFloat(ParameterID("freefilter_randomyamount0", 1), "Random amount", 0.0, 1.0, 0.0)); // 40
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

	addParameter(new AudioParameterBool(ParameterID("loop_enabled0", 1), "Loop", true)); // 60

    //addParameter(new AudioParameterBool("rewind0", "Rewind", false)); // 61
    // have to add it this way to specify rewind as a Meta parameter, so that Apple auval will pass it
    addParameter(new AudioProcessorValueTreeState::Parameter (ParameterID("rewind0", 1),
                                                                         "Rewind",
                                                                         "",
                                                                         NormalisableRange<float>(0.0f, 1.0f),
                                                                         0.0f, // float defaultParameterValue,
                                                                         nullptr, //std::function<String (float)> valueToTextFunction,
                                                                         nullptr, // std::function<float (const String&)> textToValueFunction,
                                                                         true, // bool isMetaParameter,
                                                                         false, // bool isAutomatableParameter,
                                                                         false, // bool isDiscrete,
                                                                         AudioProcessorParameter::Category::genericParameter, // AudioProcessorParameter::Category parameterCategory,
                                                                         true));//bool isBoolean));

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
	addParameter(new AudioParameterFloat(ParameterID("dryplayrate0", 1), "Dry playrate",
		NormalisableRange<float>(0.1f, 8.0f,
			dprate_convertFrom0To1Func, dprate_convertTo0To1Func), 1.0f)); // 62

    addParameter(new AudioParameterBool(ParameterID("binauralbeats", 1), "BinauralBeats Enable", false)); // 63
    addParameter(new AudioParameterFloat(ParameterID("binauralbeatsmono", 1), "Binaural Beats Power", 0.0, 1.0, 0.5)); // 64
    //addParameter(new AudioParameterFloat(ParameterID("binauralbeatsfreq", 1), "BinauralBeats Freq", 0.0, 1.0, 0.5)); // 65
    addParameter(new AudioParameterFloat(ParameterID("binauralbeatsfreq", 1), "Binaural Beats Freq",
                                         NormalisableRange<float>(0.05f, 50.0f, 0.0f, 0.25f), 4.0f)); // 65
    addParameter(new AudioParameterChoice  (ParameterID("binauralbeatsmode", 1), "BinauralBeats Mode", { "Left-Right", "Right-Left", "Symmetric" }, 0)); // 66

    m_bbpar.free_edit.extreme_y.set_min(0.05f);
    m_bbpar.free_edit.extreme_y.set_max(50.0f);

	auto& pars = getParameters();
	for (const auto& p : pars)
		m_reset_pars.push_back(p->getValue());

    if (!m_is_stand_alone_offline) {
        setPreBufferAmount(2);
        startTimer(1, 40);
    }

#if (JUCE_IOS)
    m_defaultRecordDir = File::getSpecialLocation (File::userDocumentsDirectory).getFullPathName();
#elif (JUCE_ANDROID)
    auto parentDir = File::getSpecialLocation (File::userApplicationDataDirectory);
    parentDir = parentDir.getChildFile("Recordings");
    m_defaultRecordDir = parentDir.getFullPathName();
#else
    auto parentDir = File::getSpecialLocation (File::userMusicDirectory);
    parentDir = parentDir.getChildFile("PaulXStretch");
    m_defaultRecordDir = parentDir.getFullPathName();
#endif

    //m_defaultCaptureDir = parentDir.getChildFile("Captures").getFullPathName();
    
    m_show_technical_info = m_propsfile->m_props_file->getBoolValue("showtechnicalinfo", false);

    DBG("Constructed PS plugin");
}

PaulstretchpluginAudioProcessor::~PaulstretchpluginAudioProcessor()
{
    stopTimer(1);

	//Logger::writeToLog("PaulX AudioProcessor destroyed");
	if (m_thumb)
		m_thumb->removeAllChangeListeners();
	m_thumb = nullptr;
	m_bufferingthread.stopThread(3000);
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
        setFFTSize(*getFloatParameter(cpi_fftsize), true);
        startplay({ *getFloatParameter(cpi_soundstart),*getFloatParameter(cpi_soundend) },
                  m_cur_num_out_chans, m_curmaxblocksize, err);
        m_stretch_source->seekPercent(m_stretch_source->getLastSourcePositionPercent());

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
	storeToTreeProperties(paramtree, nullptr, getParameters(), { getBoolParameter(cpi_capture_trigger) });
    if (m_current_file != URL() && ignorefile == false)
	{
		paramtree.setProperty("importedfile", m_current_file.toString(false), nullptr);
#if JUCE_IOS
        // store bookmark data if necessary
        if (void * bookmark = getURLBookmark(m_current_file)) {
            const void * data = nullptr;
            size_t datasize = 0;
            if (urlBookmarkToBinaryData(bookmark, data, datasize)) {
                DBG("Audio file has bookmark, storing it in state, size: " << datasize);
                paramtree.setProperty("importedfile_bookmark", var(data, datasize), nullptr);
            } else {
                DBG("Bookmark is not valid!");
            }
        } 
#endif
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
		storeToTreeProperties(paramtree, nullptr, "playwhenhostrunning", m_play_when_host_plays, 
			"capturewhenhostrunning", m_capture_when_host_plays,"savecapturedaudio",m_save_captured_audio,
			"mutewhilecapturing",m_mute_while_capturing, "muteprocwhilecapturing",m_mute_processed_while_capturing);
	}
	storeToTreeProperties(paramtree, nullptr, "tabaindex", m_cur_tab_index);
	storeToTreeProperties(paramtree, nullptr, "waveviewrange", m_wave_view_range);
    ValueTree freefilterstate = m_free_filter_envelope->saveState(Identifier("freefilter_envelope"));
    paramtree.addChild(freefilterstate, -1, nullptr);

    storeToTreeProperties(paramtree, nullptr, "pluginwidth", mPluginWindowWidth);
    storeToTreeProperties(paramtree, nullptr, "pluginheight", mPluginWindowHeight);
    storeToTreeProperties(paramtree, nullptr, "jumpsliders", m_use_jumpsliders);
    storeToTreeProperties(paramtree, nullptr, "restoreplaystate", m_restore_playstate);
    storeToTreeProperties(paramtree, nullptr, "autofinishrecord", m_auto_finish_record);

    paramtree.setProperty("defRecordDir", m_defaultRecordDir, nullptr);
    paramtree.setProperty("defRecordFormat", (int)m_defaultRecordingFormat, nullptr);
    paramtree.setProperty("defRecordBitDepth", (int)m_defaultRecordingBitsPerSample, nullptr);


    return paramtree;
}

void PaulstretchpluginAudioProcessor::setStateFromTree(ValueTree tree)
{
	if (tree.isValid())
	{
        bool origpaused =  getBoolParameter(cpi_pause_enabled)->get();

		{
			ScopedLock locker(m_cs);
            ValueTree freefilterstate = tree.getChildWithName("freefilter_envelope");
            m_free_filter_envelope->restoreState(freefilterstate);
            m_load_file_with_state = tree.getProperty("loadfilewithstate", true);
			getFromTreeProperties(tree, "playwhenhostrunning", m_play_when_host_plays, 
				"capturewhenhostrunning", m_capture_when_host_plays,"mutewhilecapturing",m_mute_while_capturing,
				"savecapturedaudio",m_save_captured_audio, "muteprocwhilecapturing",m_mute_processed_while_capturing);
			getFromTreeProperties(tree, "tabaindex", m_cur_tab_index);
            getFromTreeProperties(tree, "pluginwidth", mPluginWindowWidth);
            getFromTreeProperties(tree, "pluginheight", mPluginWindowHeight);
            getFromTreeProperties(tree, "jumpsliders", m_use_jumpsliders);
            getFromTreeProperties(tree, "restoreplaystate", m_restore_playstate);
            getFromTreeProperties(tree, "autofinishrecord", m_auto_finish_record);

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
						new_order.push_back({ (SpectrumProcessType)index, old_order[index].m_enabled });
					}
					m_stretch_source->setSpectrumProcessOrder(new_order);
				}
			}
			getFromTreeProperties(tree, "waveviewrange", m_wave_view_range);
			getFromTreeProperties(tree, getParameters());

#if !(JUCE_IOS || JUCE_ANDROID)
            setDefaultRecordingDirectory(tree.getProperty("defRecordDir", m_defaultRecordDir));
#endif
            m_defaultRecordingFormat = (RecordFileFormat) (int) tree.getProperty("defRecordFormat", (int)m_defaultRecordingFormat);
            m_defaultRecordingBitsPerSample = (int) tree.getProperty("defRecordBitDepth", (int)m_defaultRecordingBitsPerSample);

        }
		int prebufamt = tree.getProperty("prebufamount", 2);
		if (prebufamt == -1)
			m_use_backgroundbuffering = false;
		else
			setPreBufferAmount(m_is_stand_alone_offline ? 0 : prebufamt);

        if (!m_restore_playstate) {
            // use previous paused value
            *(getBoolParameter(cpi_pause_enabled)) = origpaused;
        }

		if (m_load_file_with_state == true)
		{
            String fn = tree.getProperty("importedfile");
			if (fn.isNotEmpty())
			{
                URL url(fn);

                if (!url.isLocalFile()) {
                    // reconstruct just in case imported file string was not a URL
                    url = URL(File(fn));
                }

#if JUCE_IOS
                // check for bookmark
                auto bptr = tree.getPropertyPointer("importedfile_bookmark");
                if (bptr) {
                    if (auto * block = bptr->getBinaryData()) {
                        DBG("Has file bookmark");
                        void * bookmark = binaryDataToUrlBookmark(block->getData(), block->getSize());
                        setURLBookmark(url, bookmark);
                    }
                }
                else {
                    DBG("no url bookmark found");
                }
#endif
                setAudioFile(url);
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

void PaulstretchpluginAudioProcessor::setFFTSize(float size, bool force)
{
    if (fabsf(m_last_fftsizeparamval - size) > 0.00001f || force) {

        if (m_prebuffer_amount == 5)
            m_fft_size_to_use = pow(2, 7.0 + size * 14.5);
        else m_fft_size_to_use = pow(2, 7.0 + size * 10.0); // chicken out from allowing huge FFT sizes if not enough prebuffering
        int optim = optimizebufsize(m_fft_size_to_use);
        m_fft_size_to_use = optim;
        m_stretch_source->setFFTSize(optim, force);

        m_last_fftsizeparamval = size;
        //Logger::writeToLog(String(m_fft_size_to_use));
    }
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
    if (m_bufferingthread.isThreadRunning() == false) {
        //m_bufferingthread.setPriority(Thread::Priority::highest);
        m_bufferingthread.startThread(Thread::Priority::highest);
    }
	m_stretch_source->setNumOutChannels(numoutchans);
	m_stretch_source->setFFTSize(m_fft_size_to_use, true);
	m_stretch_source->setProcessParameters(&m_ppar, &m_bbpar);
	m_stretch_source->m_prebuffersize = bufamt;
	
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

void PaulstretchpluginAudioProcessor::updateStretchParametersFromPluginParameters(ProcessParameters & pars, BinauralBeatsParameters & bbpar)
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

    bbpar.stereo_mode = (BB_STEREO_MODE) getChoiceParameter(cpi_binauralbeats_mode)->getIndex();
    bbpar.mono = *getFloatParameter(cpi_binauralbeats_mono);
    //bbpar.free_edit.set_all_values( *getFloatParameter(cpi_binauralbeats_freq));
    auto * bbfreqp = getFloatParameter(cpi_binauralbeats_freq);
    float bbfreq = *bbfreqp;
    float bbratio = (bbfreq - bbfreqp->getNormalisableRange().getRange().getStart()) / bbfreqp->getNormalisableRange().getRange().getLength();
    if (bbpar.free_edit.get_posy(0) != bbratio) {
        bbpar.free_edit.set_posy(0, bbratio);
        bbpar.free_edit.set_posy(1, bbratio);
        bbpar.free_edit.update_curve(2);
    }
    //bbpar.mono = 0.5f;
    bbpar.free_edit.set_enabled(*getBoolParameter(cpi_binauralbeats));

}

void PaulstretchpluginAudioProcessor::saveCaptureBuffer()
{
	auto task = [this]()
	{
		int inchans = jmin(getMainBusNumInputChannels(), getIntParameter(cpi_num_inchans)->get());
		if (inchans < 1)
			return;
        
        std::unique_ptr<AudioFormat> audioFormat;
        String fextension;
        int bitsPerSample = std::min(32, m_defaultRecordingBitsPerSample);

        if (m_defaultRecordingFormat == FileFormatWAV) {
            audioFormat = std::make_unique<WavAudioFormat>();
            fextension = ".wav";
        }
        else {
            audioFormat = std::make_unique<FlacAudioFormat>();
            fextension = ".flac";
            bitsPerSample = std::min(24, bitsPerSample);
        }

        
        String outfn;
        String filename = String("pxs_") + Time::getCurrentTime().formatted("%Y-%m-%d_%H.%M.%S");
        filename = File::createLegalFileName(filename);

        if (m_capture_location.isEmpty()) {
            File capdir(m_defaultRecordDir);
            outfn = capdir.getChildFile("Captures").getNonexistentChildFile(filename, fextension).getFullPathName();
        }
        else {
			outfn = File(m_capture_location).getNonexistentChildFile(filename, fextension).getFullPathName();
        }
		File outfile(outfn);
		outfile.create();
		if (outfile.existsAsFile())
		{
			m_capture_save_state = 1;
			auto outstream = outfile.createOutputStream();
			auto writer = unique_from_raw(audioFormat->createWriterFor(outstream.get(), getSampleRateChecked(),
				inchans, bitsPerSample, {}, 0));
			if (writer != nullptr)
			{
                outstream.release(); // the writer takes ownership

				auto sourcebuffer = getStretchSource()->getSourceAudioBuffer();
				jassert(sourcebuffer->getNumChannels() == inchans);
				jassert(sourcebuffer->getNumSamples() > 0);
				
				writer->writeFromAudioSampleBuffer(*sourcebuffer, 0, sourcebuffer->getNumSamples());
				m_current_file = URL(outfile);
			}
			else
			{
				Logger::writeToLog("Could not create wav writer");
				//delete outstream;
			}
		}
		else
			Logger::writeToLog("Could not create output file");
		m_capture_save_state = 0;
	};
	m_threadpool->addJob(task);
}

String PaulstretchpluginAudioProcessor::offlineRender(OfflineRenderParams renderpars)
{
	File outputfiletouse = renderpars.outputfile.getNonexistentSibling();
	ValueTree state{ getStateTree(false, false) };
    // override this to always load file with state if possible
    state.setProperty("loadfilewithstate", true, nullptr);
	auto processor = std::make_shared<PaulstretchpluginAudioProcessor>(true);
	processor->setNonRealtime(true);
	processor->setStateFromTree(state);

    double outsr{ renderpars.outsr };
    if (outsr < 10.0) {
        outsr = processor->getStretchSource()->getInfileSamplerate();
        if (outsr < 10.0) {
            outsr = getSampleRateChecked();
        }
    }

    Logger::writeToLog(outputfiletouse.getFullPathName() + " " + String(outsr) + " " + String(renderpars.outputformat));
	int blocksize{ 1024 };
	int numoutchans = *processor->getIntParameter(cpi_num_outchans);
	auto sc = processor->getStretchSource();
	double t0 = *processor->getFloatParameter(cpi_soundstart);
	double t1 = *processor->getFloatParameter(cpi_soundend);
	sanitizeTimeRange(t0, t1);
	sc->setPlayRange({ t0,t1 }, true);

    DBG("play range: " << t0 << " " << t1);
    DBG("SC play range s: " << sc->getPlayRange().getStart() << "  e: " << sc->getPlayRange().getEnd());

    *(processor->getBoolParameter(cpi_pause_enabled)) = false;

    if (m_using_memory_buffer) {
        // copy it from the original
        processor->m_recbuffer.makeCopyOf(m_recbuffer);
        processor->m_using_memory_buffer = true;
    }

	sc->setMainVolume(*processor->getFloatParameter(cpi_main_volume));
	sc->setRate(*processor->getFloatParameter(cpi_stretchamount));
    sc->setPreviewDry(*processor->getBoolParameter(cpi_bypass_stretch));
	sc->setDryPlayrate(*processor->getFloatParameter(cpi_dryplayrate));
    sc->setPaused(false);

	processor->setFFTSize(*processor->getFloatParameter(cpi_fftsize), true);
	processor->updateStretchParametersFromPluginParameters(processor->m_ppar, processor->m_bbpar);
	processor->setPlayConfigDetails(2, numoutchans, outsr, blocksize);
	processor->prepareToPlay(outsr, blocksize);

    if (renderpars.numloops == 1) {
        // prevent any loop xfade getting into the output if only 1 loop selected
        *processor->getBoolParameter(cpi_looping_enabled) = false;
    }


    //sc->setProcessParameters(&processor->m_ppar);
    //sc->setFFTWindowingType(1);

    DBG("SC post play range s: " << sc->getPlayRange().getStart() << "  e: " << sc->getPlayRange().getEnd() << "  fft: " << sc->getFFTSize() << " ourdur: " << sc->getOutputDurationSecondsForRange(sc->getPlayRange(),sc->getFFTSize()));

	auto rendertask = [sc,processor,outputfiletouse, renderpars,blocksize,numoutchans, outsr,this]()
	{
		WavAudioFormat wavformat;
		auto outstream = outputfiletouse.createOutputStream();
		jassert(outstream != nullptr);
		int oformattouse{ 16 };
		bool clipoutput{ false };
		if (renderpars.outputformat == 1)
			oformattouse = 24;
		if (renderpars.outputformat == 2)
			oformattouse = 32;
		if (renderpars.outputformat == 3)
		{
			oformattouse = 32;
			clipoutput = true;
		}
		auto writer{ unique_from_raw(wavformat.createWriterFor(outstream.get(), outsr, numoutchans,
			oformattouse, StringPairArray(), 0)) };
		if (writer == nullptr)
		{
			//delete outstream;
			jassert(false);

            m_offline_render_state = 200;
            Logger::writeToLog("Render failed, could not open file!");
            if (renderpars.completionHandler) {
                renderpars.completionHandler(false, outputfiletouse);
            }

            return;
        } else {
            outstream.release(); // the writer takes ownership

            AudioBuffer<float> renderbuffer{ numoutchans, blocksize };
            MidiBuffer dummymidi;
            double outlensecs = sc->getOutputDurationSecondsForRange(sc->getPlayRange(),sc->getFFTSize());

            if (*processor->getBoolParameter(cpi_looping_enabled)) {
                outlensecs *= jmax(1, renderpars.numloops);
            }
            outlensecs = jmin(outlensecs, renderpars.maxoutdur);

            int64_t outlenframes = outlensecs * outsr;
            int64_t outcounter{ 0 };
            m_offline_render_state = 0;
            m_offline_render_cancel_requested = false;

            DBG("Starting rendering of " << outlenframes << " frames, " << outlensecs << " secs" << ", loops: " << renderpars.numloops << " play range s: " << sc->getPlayRange().getStart() << "  e: " << sc->getPlayRange().getEnd());

            while (outcounter < outlenframes)
            {
                if (m_offline_render_cancel_requested == true)
                    break;
                processor->processBlock(renderbuffer, dummymidi);
                int64 framesToWrite = std::min<int64>(blocksize, outlenframes - outcounter);
                writer->writeFromAudioSampleBuffer(renderbuffer, 0, framesToWrite);
                outcounter += blocksize;
                m_offline_render_state = 100.0 / outlenframes * outcounter;
            }
            m_offline_render_state = 200;

            if (renderpars.completionHandler) {
                renderpars.completionHandler(true, outputfiletouse);
            }
            Logger::writeToLog("Rendered ok!");
        }
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
	m_adsr.setSampleRate(sampleRate);
	m_cur_sr = sampleRate;
	m_curmaxblocksize = samplesPerBlock;
	m_input_buffer.setSize(getTotalNumInputChannels(), samplesPerBlock);
	setParameter(cpi_rewind, 0.0f);
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
		setFFTSize(*getFloatParameter(cpi_fftsize), true);
		m_stretch_source->setProcessParameters(&m_ppar, &m_bbpar);
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

    m_standalone = juce::PluginHostType::getPluginLoadedAs() == AudioProcessor::wrapperType_Standalone;
}

void PaulstretchpluginAudioProcessor::releaseResources()
{
	
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool PaulstretchpluginAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    ignoreUnused (layouts);
    return true;
  #else

     // support anything
    return true;

    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    if ( /* layouts.getMainOutputChannelSet() != AudioChannelSet::mono() && */
        layouts.getMainOutputChannelSet() != AudioChannelSet::stereo())
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

static void copyAudioBufferWrappingPosition(const AudioBuffer<float>& src, int numSamples, AudioBuffer<float>& dest, int destbufpos, int maxdestpos, float fademode)
{
    int useNumSamples = jmin(numSamples, src.getNumSamples());

	for (int i = 0; i < dest.getNumChannels(); ++i)
	{
		int channel_to_copy = i % src.getNumChannels();
		if (destbufpos + useNumSamples > maxdestpos)
		{
			int wrappos = (destbufpos + useNumSamples) % maxdestpos;
			int partial_len = useNumSamples - wrappos;

            if (fademode == 0.0f) {
                dest.copyFrom(i, destbufpos, src, channel_to_copy, 0, partial_len);
                dest.copyFrom(i, partial_len, src, channel_to_copy, 0, wrappos);
            } else {
                //DBG("recfade wrap: " << fademode);
                if (fademode > 0.0f) {
                    // fade in
                    dest.copyFromWithRamp(i, destbufpos, src.getReadPointer(channel_to_copy), partial_len, fademode > 0.0f ? 0.0f : 1.0f, fademode > 0.0f ? 1.0f : 0.0f);
                    dest.copyFrom(i, partial_len, src, channel_to_copy, 0, wrappos);
                } else {
                    // fade out
                    dest.copyFrom(i, destbufpos, src, channel_to_copy, 0, partial_len);
                    dest.copyFromWithRamp(i, partial_len, src.getReadPointer(channel_to_copy), wrappos, fademode > 0.0f ? 0.0f : 1.0f, fademode > 0.0f ? 1.0f : 0.0f);
                }
            }
		}
		else
		{
            if (fademode == 0.0f) {
                dest.copyFrom(i, destbufpos, src, channel_to_copy, 0, useNumSamples);
            } else {
                //DBG("recfade: " << fademode);
                dest.copyFromWithRamp(i, destbufpos, src.getReadPointer(channel_to_copy), useNumSamples, fademode > 0.0f ? 0.0f : 1.0f, fademode > 0.0f ? 1.0f : 0.0f);
            }
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
    bool passthruEnabled = getParameter(cpi_passthrough) > 0.5f;

    AudioPlayHead* phead = getPlayHead();
    bool seektostart = false;
	if (phead != nullptr)
	{
		phead->getCurrentPosition(m_playposinfo);

        if (m_playposinfo.isPlaying && (m_playposinfo.ppqPosition == 0.0 || m_playposinfo.timeInSamples == 0)) {
            seektostart = true;
        }
	}
    else {
		m_playposinfo.isPlaying = false;
    }

	ScopedNoDenormals noDenormals;
	double srtemp = getSampleRate();
	if (srtemp != m_cur_sr)
		m_cur_sr = srtemp;
	m_prebufsmoother.setSlope(0.9, srtemp / buffer.getNumSamples());
	m_smoothed_prebuffer_ready = m_prebufsmoother.process(m_buffering_source->getPercentReady());

    if (buffer.getNumSamples() > m_input_buffer.getNumSamples()) {
        // just in case
        m_input_buffer.setSize(totalNumInputChannels, buffer.getNumSamples(), false, false, true);
    }


	for (int i = 0; i < totalNumInputChannels; ++i)
		m_input_buffer.copyFrom(i, 0, buffer, i, 0, buffer.getNumSamples());
    for (int i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    float fadepassthru = 0.0f;
    if (!passthruEnabled) {
        if (m_lastpassthru != passthruEnabled)  {
            // ramp it down
            fadepassthru = -1.0f;
            for (int i = 0; i < totalNumInputChannels; ++i)
                buffer.applyGainRamp(i, 0, buffer.getNumSamples(), 1.0f, 0.0f);
        }
        else {
            for (int i = 0; i < totalNumInputChannels; ++i)
                buffer.clear (i, 0, buffer.getNumSamples());
        }
    }
    else if (passthruEnabled != m_lastpassthru) {
        // ramp it up
        fadepassthru = 1.0f;
        for (int i = 0; i < totalNumInputChannels; ++i)
            buffer.applyGainRamp(i, 0, buffer.getNumSamples(), 0.0f, 1.0f);
    }

    m_lastpassthru = passthruEnabled;

    float recfade = 0.0f;
    if (m_is_recording != m_is_recording_pending) {
        recfade = m_is_recording_pending ? 1.0f : -1.0f;
        m_is_recording = m_is_recording_pending;
    }

    if (m_is_recording && m_auto_finish_record && (m_rec_count + buffer.getNumSamples()) > m_max_reclen*getSampleRateChecked())
    {
        // finish recording automatically
        recfade = -1.0f;
        m_is_recording = m_is_recording_pending = false;
        DBG("Finish record automatically");
    }


	if (m_previewcomponent != nullptr)
	{
		m_previewcomponent->processBlock(getSampleRate(), buffer);
		return;
	}

    if (m_prebuffering_inited == false)
		return;

    if (m_is_recording == true || recfade != 0.0f)
	{
        if (m_playposinfo.isPlaying == false && m_capture_when_host_plays == true && !m_standalone) {
            if (!m_is_recording)
                m_is_recording_finished = true;
			return;
        }

		int recbuflenframes = m_max_reclen * getSampleRate();
		copyAudioBufferWrappingPosition(m_input_buffer, buffer.getNumSamples(), m_recbuffer, m_rec_pos, recbuflenframes, recfade);
		m_thumb->addBlock(m_rec_pos, m_input_buffer, 0, buffer.getNumSamples());
		m_rec_pos = (m_rec_pos + buffer.getNumSamples()) % recbuflenframes;
		m_rec_count += buffer.getNumSamples();

        if (!m_is_recording) {
            // to signal that it may be written, etc
            DBG("Signal finish");
            m_is_recording_finished = true;
        }

		if (m_rec_count<recbuflenframes)
			m_recorded_range = { 0, m_rec_count };
        if (m_mute_while_capturing == true && passthruEnabled) {
            if (recfade < 0.0f) {
                buffer.applyGainRamp(0, buffer.getNumSamples(), 1.0f, 0.0f);
            }
            else if (recfade > 0.0f) {
                buffer.applyGainRamp(0, buffer.getNumSamples(), 0.0f, 1.0f);
            }
            else {
                buffer.clear();
            }
        }

        if (m_mute_processed_while_capturing == true)
            return;
	}
	jassert(m_buffering_source != nullptr);
	jassert(m_bufferingthread.isThreadRunning());
	double t0 = *getFloatParameter(cpi_soundstart);
	double t1 = *getFloatParameter(cpi_soundend);
	sanitizeTimeRange(t0, t1);
	m_stretch_source->setPlayRange({ t0,t1 });

    float fadeproc = 0.0f;

	if (m_last_host_playing == false && m_playposinfo.isPlaying)
	{
        if (m_play_when_host_plays) {
            // should we even do this ever?
            if (seektostart)
                m_stretch_source->seekPercent(*getFloatParameter(cpi_soundstart));
            fadeproc = 1.0f; // fadein
        }
		m_last_host_playing = true;
	}
	else if (m_last_host_playing == true && m_playposinfo.isPlaying == false)
	{
		m_last_host_playing = false;
        if (m_play_when_host_plays) {
            fadeproc = -1.0f; // fadeout
        }
	}

    if (m_play_when_host_plays == true && m_playposinfo.isPlaying == false && !m_standalone && fadeproc == 0.0f)
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
	bool rew = getParameter(cpi_rewind) > 0.0f;
	if (rew != m_lastrewind)
	{
		if (rew == true)
		{
			setParameter(cpi_rewind, 0.0f);
			m_stretch_source->seekPercent(m_stretch_source->getPlayRange().getStart());
		}
		m_lastrewind = rew;
	}

	m_stretch_source->setMainVolume(*getFloatParameter(cpi_main_volume));
	m_stretch_source->setRate(*getFloatParameter(cpi_stretchamount));
	m_stretch_source->setPreviewDry(*getBoolParameter(cpi_bypass_stretch));
	m_stretch_source->setDryPlayrate(*getFloatParameter(cpi_dryplayrate));
	setFFTSize(*getFloatParameter(cpi_fftsize));
	
	updateStretchParametersFromPluginParameters(m_ppar, m_bbpar);

	m_stretch_source->setOnsetDetection(*getFloatParameter(cpi_onsetdetection));
	m_stretch_source->setLoopXFadeLength(*getFloatParameter(cpi_loopxfadelen));
	
	
	
	m_stretch_source->setFreezing(*getBoolParameter(cpi_freeze));
	m_stretch_source->setPaused(*getBoolParameter(cpi_pause_enabled));
	if (m_midinote_control == true)
	{
		MidiBuffer::Iterator midi_it(midiMessages);
		MidiMessage midi_msg;
		int midi_msg_pos;
		while (true)
		{
			if (midi_it.getNextEvent(midi_msg, midi_msg_pos) == false)
				break;
			if (midi_msg.isNoteOff() && midi_msg.getNoteNumber() == m_midinote_to_use)
			{
				m_adsr.noteOff();
				break;
			}
			if (midi_msg.isNoteOn())
			{
				m_midinote_to_use = midi_msg.getNoteNumber();
				m_adsr.setParameters({ 1.0,0.5,0.5,1.0 });
				m_adsr.noteOn();
				break;
			}

		}
	}
	if (m_midinote_control == true && m_midinote_to_use >= 0)
	{
		int note_offset = m_midinote_to_use - 60;
		m_ppar.pitch_shift.cents += 100.0*note_offset;
	}
	
	m_stretch_source->setProcessParameters(&m_ppar, &m_bbpar);
	AudioSourceChannelInfo aif(buffer);
	if (isNonRealtime() || m_use_backgroundbuffering == false)
	{
		m_stretch_source->getNextAudioBlock(aif);
	}
	else
	{
		m_buffering_source->getNextAudioBlock(aif);
	}

    // fade processing if necessary
    if (fadeproc != 0.0f) {
        buffer.applyGainRamp(0, buffer.getNumSamples(), fadeproc > 0.0f ? 0.0f : 1.0f, fadeproc > 0.0f ? 1.0f : 0.0f);
    }

	if (fadepassthru != 0.0f
        || (passthruEnabled && (!m_is_recording || !m_mute_while_capturing))
        || (recfade != 0.0f && m_mute_while_capturing))
	{
        if (recfade != 0.0f && m_mute_while_capturing) {
            // DBG("Invert recfade");
            fadepassthru = -recfade;
        }

		for (int i = 0; i < totalNumInputChannels; ++i)
		{
            if (fadepassthru != 0.0f) {
                buffer.addFromWithRamp(i, 0, m_input_buffer.getReadPointer(i), buffer.getNumSamples(), fadepassthru > 0.0f ? 0.0f : 1.0f, fadepassthru > 0.0f ? 1.0f : 0.0f);
            }
            else
                buffer.addFrom(i, 0, m_input_buffer, i, 0, buffer.getNumSamples());
		}
    }

	bool abnordetected = false;
	for (int i = 0; i < buffer.getNumChannels(); ++i)
	{
		for (int j = 0; j < buffer.getNumSamples(); ++j)
		{
			float sample = buffer.getSample(i,j);
			if (std::isnan(sample) || std::isinf(sample))
			{
				++m_abnormal_output_samples;
				abnordetected = true;
			}
		}
	}
	if (abnordetected)
	{
		buffer.clear();
	}
	else
	{
		if (m_midinote_control == true)
		{
			m_adsr.applyEnvelopeToBuffer(buffer, 0, buffer.getNumSamples());
		}
	}
	/*
	auto ed = dynamic_cast<PaulstretchpluginAudioProcessorEditor*>(getActiveEditor());
	if (ed != nullptr)
	{
		ed->m_sonogram.addAudioBlock(buffer);
	}
	*/

    // output to file writer if necessary
    if (m_writingPossible.load()) {
        const ScopedTryLock sl (m_writerLock);
        if (sl.isLocked())
        {
            if (m_activeMixWriter.load() != nullptr) {
                m_activeMixWriter.load()->write (buffer.getArrayOfReadPointers(), buffer.getNumSamples());
            }

            m_elapsedRecordSamples += buffer.getNumSamples();
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

void PaulstretchpluginAudioProcessor::setInputRecordingEnabled(bool b)
{
	ScopedLock locker(m_cs);
	int lenbufframes = getSampleRateChecked()*m_max_reclen;
	if (b == true)
	{
		m_using_memory_buffer = true;
		m_current_file = URL();
		int numchans = jmin(getMainBusNumInputChannels(), m_inchansparam->get());
		m_recbuffer.setSize(numchans, m_max_reclen*getSampleRateChecked()+4096,false,false,true);
		m_recbuffer.clear();
		m_rec_pos = 0;
		m_thumb->reset(m_recbuffer.getNumChannels(), getSampleRateChecked(), lenbufframes);
		m_recorded_range = Range<int64>();
		m_rec_count = 0;
        m_next_rec_count = getSampleRateChecked()*m_max_reclen;
        m_is_recording_pending = true;
	}
	else
	{
        if (m_is_recording == true) {

            m_is_recording_finished = false; // will be marked true when the recording is truly done
            m_is_recording_pending = false;
        }
	}
}

double PaulstretchpluginAudioProcessor::getInputRecordingPositionPercent()
{
	if (m_is_recording_pending==false)
		return 0.0;
	return 1.0 / m_recbuffer.getNumSamples()*m_rec_pos;
}

String PaulstretchpluginAudioProcessor::setAudioFile(const URL & url)
{
    // this handles any permissions stuff (needed on ios)
    std::unique_ptr<InputStream> wi (url.createInputStream (false));
    File file = url.getLocalFile();

    auto ai = unique_from_raw(m_afm->createReaderFor(file));
	if (ai != nullptr)
	{
		if (ai->numChannels > 8)
		{
			return "Too many channels in file "+ file.getFullPathName();
		}
		if (ai->bitsPerSample>32)
		{
			return "Too high bit depth in file " + file.getFullPathName();
		}
		if (m_thumb)
			m_thumb->setSource(new FileInputSource(file));


        // lets not lock
        //ScopedLock locker(m_cs);

        m_stretch_source->setAudioFile(url);

        //Range<double> currange{ *getFloatParameter(cpi_soundstart),*getFloatParameter(cpi_soundend) };
		//if (currange.contains(m_stretch_source->getInfilePositionPercent())==false)
			m_stretch_source->seekPercent(*getFloatParameter(cpi_soundstart));
		m_current_file = url;

#if JUCE_IOS
        if (void * bookmark = getURLBookmark(m_current_file)) {
            DBG("Loaded audio file has bookmark");
        }
#endif

        m_current_file_date = file.getLastModificationTime();
		m_using_memory_buffer = false;
		setDirty();
		return String();
	}
	return "Could not open file " + file.getFullPathName();
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
		bool capture = *getBoolParameter(cpi_capture_trigger);
		if (capture == false && m_max_reclen != *getFloatParameter(cpi_max_capture_len))
		{
			m_max_reclen = *getFloatParameter(cpi_max_capture_len);
			//Logger::writeToLog("Changing max capture len to " + String(m_max_reclen));
		}
		if (capture == true && m_is_recording_pending == false && !m_is_recording_finished)
		{
            DBG("start recording");
			setInputRecordingEnabled(true);
			return;
		}
		if (capture == false && m_is_recording_pending == true && !m_is_recording_finished)
		{
            DBG("stop recording");
			setInputRecordingEnabled(false);
			return;
		}

        bool loopcommit = false;

        if (m_is_recording_finished) {
            DBG("Recording is actually done, commit the finish");
            int lenbufframes = getSampleRateChecked()*m_max_reclen;
            finishRecording(lenbufframes);

            *getBoolParameter(cpi_capture_trigger) = false; // ensure it
        }
        else if (m_is_recording && loopcommit && m_rec_count > m_next_rec_count) {
            DBG("Recording commit loop: " << m_rec_count << " next: " << m_next_rec_count);
            int lenbufframes = getSampleRateChecked()*m_max_reclen;
            commitRecording(lenbufframes);

            m_next_rec_count += lenbufframes;
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

void PaulstretchpluginAudioProcessor::setAudioPreview(AudioFilePreviewComponent * afpc)
{
	ScopedLock locker(m_cs);
	m_previewcomponent = afpc;
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
			auto err = setAudioFile(URL(fn));
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

void PaulstretchpluginAudioProcessor::commitRecording(int lenrecording)
{
    m_current_file = URL();
    auto currpos = m_stretch_source->getLastSeekPos();
    m_stretch_source->setAudioBufferAsInputSource(&m_recbuffer, getSampleRateChecked(), lenrecording);
    //m_stretch_source->seekPercent(currpos);
    *getFloatParameter(cpi_soundstart) = 0.0f;
    *getFloatParameter(cpi_soundend) = jlimit<double>(0.01, 1.0, (1.0 / lenrecording) * m_rec_count);
}


void PaulstretchpluginAudioProcessor::finishRecording(int lenrecording, bool nosave)
{
    m_is_recording_finished = false;
	m_is_recording_pending = false;
	m_current_file = URL();
	m_stretch_source->setAudioBufferAsInputSource(&m_recbuffer, getSampleRateChecked(), lenrecording);
	*getFloatParameter(cpi_soundstart) = 0.0f;
	*getFloatParameter(cpi_soundend) = jlimit<double>(0.01, 1.0, (1.0 / lenrecording) * m_rec_count);
	if (nosave == false && m_save_captured_audio == true)
	{
		saveCaptureBuffer();
	}
}

bool PaulstretchpluginAudioProcessor::startRecordingToFile(File & file, RecordFileFormat fileformat)
{
    if (!m_recordingThread) {
        m_recordingThread = std::make_unique<TimeSliceThread>("Recording Thread");
        m_recordingThread->startThread();
    }

    stopRecordingToFile();

    bool ret = false;

    // Now create a WAV writer object that writes to our output stream...
    //WavAudioFormat audioFormat;
    std::unique_ptr<AudioFormat> audioFormat;
    std::unique_ptr<AudioFormat> wavAudioFormat;

    int qualindex = 0;

    int bitsPerSample = std::min(32, m_defaultRecordingBitsPerSample);

    if (getSampleRate() <= 0)
    {
        return false;
    }

    File usefile = file;

    if (fileformat == FileFormatDefault) {
        fileformat = m_defaultRecordingFormat;
    }


    m_totalRecordingChannels = getMainBusNumOutputChannels();
    if (m_totalRecordingChannels == 0) {
        m_totalRecordingChannels = 2;
    }

    if (fileformat == FileFormatFLAC && m_totalRecordingChannels > 8) {
        // flac doesn't support > 8 channels
        fileformat = FileFormatWAV;
    }

    if (fileformat == FileFormatFLAC || (fileformat == FileFormatAuto && file.getFileExtension().toLowerCase() == ".flac")) {
        audioFormat = std::make_unique<FlacAudioFormat>();
        bitsPerSample = std::min(24, bitsPerSample);
        usefile = file.withFileExtension(".flac");
    }
    else if (fileformat == FileFormatWAV || (fileformat == FileFormatAuto && file.getFileExtension().toLowerCase() == ".wav")) {
        audioFormat = std::make_unique<WavAudioFormat>();
        usefile = file.withFileExtension(".wav");
    }
    else if (fileformat == FileFormatOGG || (fileformat == FileFormatAuto && file.getFileExtension().toLowerCase() == ".ogg")) {
        audioFormat = std::make_unique<OggVorbisAudioFormat>();
        qualindex = 8; // 256k
        usefile = file.withFileExtension(".ogg");
    }
    else {
        m_lastError = TRANS("Could not find format for filename");
        DBG(m_lastError);
        return false;
    }

    bool userwriting = false;

    // Create an OutputStream to write to our destination file...
    usefile.deleteFile();

    if (auto fileStream = std::unique_ptr<FileOutputStream> (usefile.createOutputStream()))
    {
        if (auto writer = audioFormat->createWriterFor (fileStream.get(), getSampleRate(), m_totalRecordingChannels, bitsPerSample, {}, qualindex))
        {
            fileStream.release(); // (passes responsibility for deleting the stream to the writer object that is now using it)

            // Now we'll create one of these helper objects which will act as a FIFO buffer, and will
            // write the data to disk on our background thread.
            m_threadedMixWriter.reset (new AudioFormatWriter::ThreadedWriter (writer, *m_recordingThread, 65536));

            DBG("Started recording only mix file " << usefile.getFullPathName());

            file = usefile;
            ret = true;
        } else {
            m_lastError.clear();
            m_lastError << TRANS("Error creating writer for ") << usefile.getFullPathName();
            DBG(m_lastError);
        }
    } else {
        m_lastError.clear();
        m_lastError << TRANS("Error creating output file: ") << usefile.getFullPathName();
        DBG(m_lastError);
    }

    if (ret) {
        // And now, swap over our active writer pointers so that the audio callback will start using it..
        const ScopedLock sl (m_writerLock);
        m_elapsedRecordSamples = 0;
        m_activeMixWriter = m_threadedMixWriter.get();

        m_writingPossible.store(m_activeMixWriter);

        //DBG("Started recording file " << usefile.getFullPathName());
    }

    return ret;
}

bool PaulstretchpluginAudioProcessor::stopRecordingToFile()
{
    // First, clear this pointer to stop the audio callback from using our writer object..

    {
        const ScopedLock sl (m_writerLock);
        m_activeMixWriter = nullptr;
        m_writingPossible.store(false);
    }

    bool didit = false;

    if (m_threadedMixWriter) {

        // Now we can delete the writer object. It's done in this order because the deletion could
        // take a little time while remaining data gets flushed to disk, and we can't be blocking
        // the audio callback while this happens.
        m_threadedMixWriter.reset();

        DBG("Stopped recording mix file");
        didit = true;
    }

    return didit;
}

bool PaulstretchpluginAudioProcessor::isRecordingToFile()
{
    return (m_activeMixWriter.load() != nullptr);
}



AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PaulstretchpluginAudioProcessor();
}
