/*
  ==============================================================================

    This file was auto-generated!

    It contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "PS_Source/PaulStretchControl.h"

class MyThumbCache;


class PaulstretchpluginAudioProcessor  : public AudioProcessor
{
public:
    //==============================================================================
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
	void setRecordingEnabled(bool b);
	bool isRecordingEnabled() { return m_is_recording; }
	double getRecordingPositionPercent();
	String setAudioFile(File f);
	File getAudioFile() { return m_current_file; }
	Range<double> getTimeSelection();
	SharedResourcePointer<AudioFormatManager> m_afm;
	StretchAudioSource* getStretchSource() { return m_stretch_source.get(); }
	double getPreBufferingPercent();
private:
	
	
	bool m_ready_to_play = false;
	AudioBuffer<float> m_recbuffer;
	double m_max_reclen = 10.0;
	bool m_is_recording = false;
	int m_rec_pos = 0;
	void finishRecording(int lenrecorded);
	bool m_using_memory_buffer = true;
	int m_cur_num_out_chans = 2;
	std::mutex m_mutex;
	File m_current_file;
    

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
	void setPreBufferAmount(int x);
	void setFFTSize(double size);
	void startplay(Range<double> playrange, int numoutchans, String& err);
	SharedResourcePointer<MyThumbCache> m_thumbcache;
	//==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PaulstretchpluginAudioProcessor)
};
