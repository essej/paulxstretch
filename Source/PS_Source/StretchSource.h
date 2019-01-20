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

#include "../JuceLibraryCode/JuceHeader.h"
#include "Input/AInputS.h"
#include "ProcessedStretch.h"
#include <mutex>
#include <array>
#include "../WDL/resample.h"

class StretchAudioSource final : public PositionableAudioSource
{
public:
	StretchAudioSource() {}
	StretchAudioSource(int initialnumoutchans, AudioFormatManager* afm, std::array<AudioParameterBool*, 9>& enab_pars);
	~StretchAudioSource();
	// Inherited via PositionableAudioSource
	void prepareToPlay(int samplesPerBlockExpected, double sampleRate) override;

	void releaseResources() override;

	void getNextAudioBlock(const AudioSourceChannelInfo & bufferToFill) override;

	void setNextReadPosition(int64 newPosition) override;

	int64 getNextReadPosition() const override;

	int64 getTotalLength() const override;

	bool isLooping() const override;

	String setAudioFile(File file);
	File getAudioFile();

    AudioBuffer<float>* getSourceAudioBuffer();
    
	void setNumOutChannels(int chans);
	int getNumOutChannels() { return m_num_outchans; }
	double getInfilePositionPercent();
	double getInfilePositionSeconds();
	double getInfileLengthSeconds();
	double getInfileSamplerate();
	void setRate(double rate);
	double getRate() 
	{ 
		return m_playrate; 
	}
	double getOutputSamplerate() const { return m_outsr; }
	void setProcessParameters(ProcessParameters* pars);
	const ProcessParameters& getProcessParameters();
	void setFFTSize(int size);
	int getFFTSize() { return m_process_fftsize; }
	
	double getFreezePos() const { return m_freeze_pos; }
	void setFreezing(bool b) { m_freezing = b; }
	bool isFreezing() { return m_freezing; }

	void setPaused(bool b);
	bool isPaused() const;

	void seekPercent(double pos);
	
	double getOutputDurationSecondsForRange(Range<double> range, int fftsize);
	
	void setOnsetDetection(double x);
	void setPlayRange(Range<double> playrange);
	Range<double> getPlayRange() { return m_playrange; }
	bool isLoopEnabled();
	bool hasReachedEnd();
    bool isResampling();
	int64_t getDiskReadSampleCount() const;
	std::vector<SpectrumProcess> getSpectrumProcessOrder();
	void setSpectrumProcessOrder(std::vector<SpectrumProcess> order);
	void setFFTWindowingType(int windowtype);
    int getFFTWindowingType() { return m_fft_window_type; }
    std::pair<Range<double>,Range<double>> getFileCachedRangesNormalized();
	
	void setFreeFilterEnvelope(shared_envelope env);

	void setClippingEnabled(bool b) { m_clip_output = b; }
	bool isLoopingEnabled();
	void setLoopingEnabled(bool b);
	void setMaxLoops(int64_t numloops) { m_maxloops = numloops; }
	void setAudioBufferAsInputSource(AudioBuffer<float>* buf, int sr, int len);
	void setMainVolume(double decibels);
	double getMainVolume() const { return m_main_volume; }
	//void setSpectralModulesEnabled(const std::array<AudioParameterBool*, 9>& params);
	void setSpectralModuleEnabled(int index, bool b);
	void setLoopXFadeLength(double lenseconds);
	double getLoopXFadeLengtj() const { return m_loopxfadelen; }
	void setPreviewDry(bool b);
	bool isPreviewingDry() const;
	void setDryPlayrate(double rate);
	double getDryPlayrate() const;
	int m_param_change_count = 0;
	double getLastSeekPos() const { return m_seekpos; }
	CriticalSection* getMutex() { return &m_cs; }
	int64_t getLastSourcePosition() const { return m_last_filepos; }
	int m_prebuffersize = 0;
	void setSpectralOrderPreset(int id);
	
private:
	CircularBuffer<float> m_stretchoutringbuf{ 1024 * 1024 };
	AudioBuffer<float> m_file_inbuf;
	LinearSmoothedValue<double> m_vol_smoother;
	std::unique_ptr<AInputS> m_inputfile;
	std::vector<std::shared_ptr<ProcessedStretch>> m_stretchers;
	
	std::function<void(StretchAudioSource*)> SourceEndedCallback;
	bool m_firstbuffer = false;
	bool m_output_has_begun = false;
	int m_num_outchans = 0;
	double m_outsr = 44100.0;
	int m_process_fftsize = 0;
    int m_fft_window_type = -1;
	double m_main_volume = 0.0;
	double m_loopxfadelen = 0.0;
	ProcessParameters m_ppar;
	
	double m_playrate = 1.0;
	double m_lastplayrate = 0.0;
	double m_onsetdetection = 0.0;
	double m_seekpos = 0.0;
	
	bool m_freezing = false;
	
	int m_pause_state = 0;
	Range<double> m_playrange{ 0.0,1.0 };
	int64_t m_rand_count = 0;
	
	bool m_stream_end_reached = false;
	int64_t m_output_silence_counter = 0;
	File m_curfile;
	int64_t m_maxloops = 0;
	std::unique_ptr<WDL_Resampler> m_resampler;
	std::vector<double> m_resampler_outbuf;
	CriticalSection m_cs;
	std::vector<SpectrumProcess> m_specproc_order;
	
	bool m_stop_play_requested = false;
	double m_freeze_pos = 0.0;
	int64_t m_output_counter = 0;
	int64_t m_output_length = 0;
	bool m_clip_output = true;
	void initObjects();
	shared_envelope m_free_filter_envelope;
	AudioFormatManager* m_afm = nullptr;
	struct
	{
		AudioBuffer<float> buffer;
		int state = 0; // 0 not active, 1 fill xfade buffer, 2 play xfade buffer
		int xfade_len = 0;
		int counter = 0;
		int requested_fft_size = 0;
		File requested_file;
	} m_xfadetask;
	int m_pause_fade_counter = 0;
	bool m_preview_dry = false;
	double m_dryplayrate = 1.0;
	AudioBuffer<float> m_drypreviewbuf;
	int64_t m_last_filepos = 0;
	void playDrySound(const AudioSourceChannelInfo & bufferToFill);
	int m_current_spec_order_preset = -1;
};
