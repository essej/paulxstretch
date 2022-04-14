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

#pragma once

#include "PS_Source/PaulStretchControl.h"

#include "../JuceLibraryCode/JuceHeader.h"
#include "jcdp_envelope.h"
#include <array>

class MyThumbCache;
class AudioFilePreviewComponent;

const int cpi_main_volume = 0;
const int cpi_stretchamount = 1;
const int cpi_fftsize = 2;
const int cpi_pitchshift = 3;
const int cpi_frequencyshift = 4;
const int cpi_soundstart = 5;
const int cpi_soundend = 6;
const int cpi_freeze = 7;
const int cpi_spreadamount = 8;
const int cpi_compress = 9;
const int cpi_loopxfadelen = 10;
const int cpi_numharmonics = 11;
const int cpi_harmonicsfreq = 12;
const int cpi_harmonicsbw = 13;
const int cpi_harmonicsgauss = 14;
const int cpi_octavesm2 = 15;
const int cpi_octavesm1 = 16;
const int cpi_octaves0 = 17;
const int cpi_octaves1 = 18;
const int cpi_octaves15 = 19;
const int cpi_octaves2 = 20;
const int cpi_tonalvsnoisebw = 21;
const int cpi_tonalvsnoisepreserve = 22;
const int cpi_filter_low = 23;
const int cpi_filter_high = 24;
const int cpi_onsetdetection = 25;
const int cpi_capture_trigger = 26;
const int cpi_num_outchans = 27;
const int cpi_pause_enabled = 28;
const int cpi_max_capture_len = 29;
const int cpi_passthrough = 30;
const int cpi_markdirty = 31;
const int cpi_num_inchans = 32;
const int cpi_bypass_stretch = 33;
const int cpi_freefilter_shiftx = 34;
const int cpi_freefilter_shifty = 35;
const int cpi_freefilter_scaley = 36;
const int cpi_freefilter_tilty = 37;
const int cpi_freefilter_randomy_numbands = 38;
const int cpi_freefilter_randomy_rate = 39;
const int cpi_freefilter_randomy_amount = 40;
const int cpi_enable_spec_module0 = 41;
const int cpi_enable_spec_module1 = 42;
const int cpi_enable_spec_module2 = 43;
const int cpi_enable_spec_module3 = 44;
const int cpi_enable_spec_module4 = 45;
const int cpi_enable_spec_module5 = 46;
const int cpi_enable_spec_module6 = 47;
const int cpi_enable_spec_module7 = 48;
const int cpi_enable_spec_module8 = 49;
const int cpi_octaves_extra1 = 50;
const int cpi_octaves_extra2 = 51;
const int cpi_octaves_ratio0 = 52;
const int cpi_octaves_ratio1 = 53;
const int cpi_octaves_ratio2 = 54;
const int cpi_octaves_ratio3 = 55;
const int cpi_octaves_ratio4 = 56;
const int cpi_octaves_ratio5 = 57;
const int cpi_octaves_ratio6 = 58;
const int cpi_octaves_ratio7 = 59;
const int cpi_looping_enabled = 60;
const int cpi_rewind = 61;
const int cpi_dryplayrate = 62;

class MyThreadPool : public ThreadPool
{
public:
	MyThreadPool() : ThreadPool(2) {}
};

class MyPropertiesFile
{
public:
    MyPropertiesFile()
    {
        
        PropertiesFile::Options poptions;
        poptions.applicationName = "PaulXStretch3";
        poptions.folderName = "PaulXStretch3";
        poptions.commonToAllUsers = false;
        poptions.doNotSave = false;
        poptions.storageFormat = PropertiesFile::storeAsXML;
        poptions.millisecondsBeforeSaving = 1000;
        poptions.ignoreCaseOfKeyNames = false;
        poptions.processLock = nullptr;
        poptions.filenameSuffix = ".xml";
        poptions.osxLibrarySubFolder = "Application Support";
        m_props_file = std::make_unique<PropertiesFile>(poptions);
        
    }
    std::unique_ptr<PropertiesFile> m_props_file;
};

class PaulstretchpluginAudioProcessorEditor;

struct OfflineRenderParams
{
	OfflineRenderParams(File ofile, double osr, int oformat, double omaxdur, int onumloops, CallOutBox* ocb=nullptr, std::function<void(bool,File file)> completion=nullptr) :
		outputfile(ofile), outsr(osr), maxoutdur(omaxdur), numloops(onumloops), outputformat(oformat), cbox(ocb), completionHandler(completion)
	{}
	File outputfile;
	double outsr = 44100.0;
	double maxoutdur = 3600.0;
	int numloops = 1;
	int outputformat = 0; // 0=16 bit pcm, 1=24 bit pcm, 2=32 bit float, 3=32 bit float clipped
	CallOutBox* cbox = nullptr;
    std::function<void(bool,File file)> completionHandler;
};

class PaulstretchpluginAudioProcessor  : public AudioProcessor, 
	public MultiTimer, public VSTCallbackHandler, public AudioProcessorParameter::Listener
{
public:
	using EditorType = PaulstretchpluginAudioProcessorEditor;
    PaulstretchpluginAudioProcessor(bool is_stand_alone_offline=false);
	~PaulstretchpluginAudioProcessor();

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

	void processBlock (AudioSampleBuffer&, MidiBuffer&) override;
    //void processBlock (AudioBuffer<double>&, MidiBuffer&) override;
    //bool supportsDoublePrecisionProcessing() const override { return true; }
    //==============================================================================
    AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect () const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const String getProgramName (int index) override;
    void changeProgramName (int index, const String& newName) override;

	void parameterValueChanged(int parameterIndex, float newValue) override;
	void parameterGestureChanged(int parameterIndex, bool gestureIsStarting) override;


    //==============================================================================
    void getStateInformation (MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;
	AudioParameterFloat* getFloatParameter(int index)
	{
		return dynamic_cast<AudioParameterFloat*>(getParameters()[index]);
	}
	AudioParameterInt* getIntParameter(int index)
	{
		return dynamic_cast<AudioParameterInt*>(getParameters()[index]);
	}
	AudioParameterBool* getBoolParameter(int index)
	{
		return dynamic_cast<AudioParameterBool*>(getParameters()[index]);
	}
    void setLastPluginBounds(juce::Rectangle<int> bounds) { mPluginWindowWidth = bounds.getWidth(); mPluginWindowHeight = bounds.getHeight();}
    juce::Rectangle<int> getLastPluginBounds() const { return juce::Rectangle<int>(0,0,mPluginWindowWidth, mPluginWindowHeight); }


	void setDirty();
	void setRecordingEnabled(bool b);
	bool isRecordingEnabled() { return m_is_recording; }
	double getRecordingPositionPercent();
	String setAudioFile(const URL& url);
	URL getAudioFile() { return m_current_file; }
	Range<double> getTimeSelection();
	SharedResourcePointer<AudioFormatManager> m_afm;
    SharedResourcePointer<MyPropertiesFile> m_propsfile;
	StretchAudioSource* getStretchSource() { return m_stretch_source.get(); }
	double getPreBufferingPercent();
	void timerCallback(int id) override;
	double getSampleRateChecked();
	int m_abnormal_output_samples = 0;
	AudioPlayHead::CurrentPositionInfo m_playposinfo;
	bool m_play_when_host_plays = false;
	bool m_capture_when_host_plays = false;
	bool m_mute_while_capturing = false;
    bool m_mute_processed_while_capturing = false;
    bool m_use_backgroundbuffering = true;
	void resetParameters();
    void setPreBufferAmount(int x);
	int getPreBufferAmount();
	bool m_load_file_with_state = true;
	ValueTree getStateTree(bool ignoreoptions, bool ignorefile);
	void setStateFromTree(ValueTree tree);
	String offlineRender(OfflineRenderParams renderpars);
	std::atomic<int> m_offline_render_state{ -1 };
	std::atomic<bool> m_offline_render_cancel_requested{ false };
	std::atomic<int> m_capture_save_state{ 0 };
	bool m_state_dirty = false;
	std::unique_ptr<AudioThumbnail> m_thumb;
	bool m_show_technical_info = false;
	Range<double> m_wave_view_range;
    int m_prepare_count = 0;
    shared_envelope m_free_filter_envelope;
	bool m_import_dlg_open = false;
	void setAudioPreview(AudioFilePreviewComponent* afpc);
	CriticalSection& getCriticalSection() { return m_cs; }
	pointer_sized_int handleVstPluginCanDo(int32 index,
		pointer_sized_int value,
		void* ptr,
		float opt) override;

	pointer_sized_int handleVstManufacturerSpecific(int32 index,
		pointer_sized_int value,
		void* ptr,
		float opt) override;
	int m_cur_tab_index = 0;
	
	bool m_save_captured_audio = true;
	String m_capture_location;
	bool m_midinote_control = false;
	std::function<void(const FileChooser&)> m_filechoose_callback;
private:
	bool m_prebuffering_inited = false;
	AudioBuffer<float> m_recbuffer;
	double m_max_reclen = 10.0;
	int m_rec_pos = 0;
	int m_rec_count = 0;
	Range<int> m_recorded_range;
	void finishRecording(int lenrecorded);
	bool m_using_memory_buffer = true;
	int m_cur_num_out_chans = 2;
	CriticalSection m_cs;
	URL m_current_file;
    Time m_current_file_date;
	bool m_is_recording = false;
	TimeSliceThread m_bufferingthread;
	std::unique_ptr<StretchAudioSource> m_stretch_source;
	std::unique_ptr<MyBufferingAudioSource> m_buffering_source;
	int m_prebuffer_amount = 1;
	bool m_recreate_buffering_source = true;
	double m_smoothed_prebuffer_ready = 0.0;
	SignalSmoother m_prebufsmoother;
	int m_fft_size_to_use = 1024;
	double m_last_outpos_pos = 0.0;
	double m_last_in_pos = 0.0;
	std::vector<int> m_bufamounts{ 4096,8192,16384,32768,65536,262144 };
	ProcessParameters m_ppar;
    int mPluginWindowWidth = 820;
    int mPluginWindowHeight = 710;

	void setFFTSize(double size);
	void startplay(Range<double> playrange, int numoutchans, int maxBlockSize, String& err);
	SharedResourcePointer<MyThumbCache> m_thumbcache;
	AudioParameterInt* m_outchansparam = nullptr;
	AudioParameterInt* m_inchansparam = nullptr;
	int m_curmaxblocksize = 0;
	double m_cur_sr = 0.0;
	bool m_last_host_playing = false;
	AudioBuffer<float> m_input_buffer;
	std::vector<float> m_reset_pars;
	int m_cur_program = 0;
	void setParameters(const std::vector<double>& pars);
	float m_cur_playrangeoffset = 0.0;
	void updateStretchParametersFromPluginParameters(ProcessParameters& pars);
	std::array<AudioParameterBool*, 9> m_sm_enab_pars;
	bool m_lastrewind = false;
	AudioFilePreviewComponent* m_previewcomponent = nullptr;
	void saveCaptureBuffer();
	SharedResourcePointer<MyThreadPool> m_threadpool;
	int m_midinote_to_use = -1;
	ADSR m_adsr;
	bool m_is_stand_alone_offline = false;
	//==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PaulstretchpluginAudioProcessor)
};
