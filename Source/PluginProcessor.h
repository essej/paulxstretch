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

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "PS_Source/PaulStretchControl.h"
#include "jcdp_envelope.h"

class MyThumbCache;

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
const int cpi_capture_enabled = 26;
const int cpi_num_outchans = 27;
const int cpi_pause_enabled = 28;
const int cpi_max_capture_len = 29;
const int cpi_passthrough = 30;
const int cpi_markdirty = 31;
const int cpi_num_inchans = 32;
const int cpi_bypass_stretch = 33;

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

class PaulstretchpluginAudioProcessor  : public AudioProcessor, 
	public MultiTimer
{
public:
	using EditorType = PaulstretchpluginAudioProcessorEditor;
    PaulstretchpluginAudioProcessor();
    ~PaulstretchpluginAudioProcessor();

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (AudioSampleBuffer&, MidiBuffer&) override;

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
	void setDirty();
	void setRecordingEnabled(bool b);
	bool isRecordingEnabled() { return m_is_recording; }
	double getRecordingPositionPercent();
	String setAudioFile(File f);
	File getAudioFile() { return m_current_file; }
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
    bool m_use_backgroundbuffering = true;
	void resetParameters();
    void setPreBufferAmount(int x);
	int getPreBufferAmount();
	bool m_load_file_with_state = true;
	ValueTree getStateTree(bool ignoreoptions, bool ignorefile);
	void setStateFromTree(ValueTree tree);
	String offlineRender(File outputfile);
	std::atomic<int> m_offline_render_state{ -1 };
	std::atomic<bool> m_offline_render_cancel_requested{ false };
	bool m_state_dirty = false;
	std::unique_ptr<AudioThumbnail> m_thumb;
	bool m_show_technical_info = false;
	Range<double> m_wave_view_range;
    int m_prepare_count = 0;
    std::shared_ptr<breakpoint_envelope> m_free_filter_envelope;
private:
	
	
	bool m_prebuffering_inited = false;
	AudioBuffer<float> m_recbuffer;
	double m_max_reclen = 10.0;
	bool m_is_recording = false;
	int m_rec_pos = 0;
	void finishRecording(int lenrecorded);
	bool m_using_memory_buffer = true;
	int m_cur_num_out_chans = 2;
	CriticalSection m_cs;
	File m_current_file;
    Time m_current_file_date;

	TimeSliceThread m_bufferingthread;
	std::unique_ptr<StretchAudioSource> m_stretch_source;
	std::unique_ptr<MyBufferingAudioSource> m_buffering_source;
	int m_prebuffer_amount = 1;
	bool m_recreate_buffering_source = true;
	
	int m_fft_size_to_use = 1024;
	double m_last_outpos_pos = 0.0;
	double m_last_in_pos = 0.0;
	std::vector<int> m_bufamounts{ 4096,8192,16384,32768,65536,262144 };
	ProcessParameters m_ppar;
	
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
	
	//==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PaulstretchpluginAudioProcessor)
};
