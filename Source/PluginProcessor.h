/*
  ==============================================================================

    This file was auto-generated!

    It contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "PS_Source/PaulStretchControl.h"

//==============================================================================
/**
*/
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
	std::unique_ptr<AudioFormatManager> m_afm;
	std::unique_ptr<Control> m_control;
private:
	
	
	bool m_ready_to_play = false;
	AudioBuffer<float> m_recbuffer;
	double m_max_reclen = 5;
	bool m_is_recording = false;
	int m_rec_pos = 0;
	void finishRecording(int lenrecorded);
	bool m_using_memory_buffer = false;
	int m_cur_num_out_chans = 2;
	std::mutex m_mutex;
	File m_current_file;
	template<typename F>
	void callGUI(F&& f, bool async)
	{
		auto ed = dynamic_cast<PaulstretchpluginAudioProcessorEditor*>(getActiveEditor());
		if (ed)
		{
			if (async == false)
				f(ed);
			else
				MessageManager::callAsync([ed,f]() { f(ed); });
		}
	}
	//==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PaulstretchpluginAudioProcessor)
};
