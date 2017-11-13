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
#include "Input/AInputS.h"
#include "ProcessedStretch.h"
#include "BinauralBeats.h"
#include <mutex>
#include "../WDL/resample.h"

class StretchAudioSource final : public PositionableAudioSource
{
public:
	StretchAudioSource() {}
	StretchAudioSource(int initialnumoutchans, AudioFormatManager* afm);
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

	void setNumOutChannels(int chans);
	int getNumOutChannels() { return m_num_outchans; }
	double getInfilePositionPercent();
	double getInfilePositionSeconds();
	double getInfileLengthSeconds();
	void setRate(double rate);
	double getRate() { return m_playrate; }
	void setProcessParameters(ProcessParameters* pars);
	ProcessParameters getProcessParameters();
	void setFFTSize(int size);
	int getFFTSize() { return m_process_fftsize; }
	
	double getFreezePos() const { return m_freeze_pos; }
	void setFreezing(bool b) { m_freezing = b; }
	bool isFreezing() { return m_freezing; }

	void seekPercent(double pos);
	
	double getOutputDurationSecondsForRange(Range<double> range, int fftsize);
	
	void setOnsetDetection(double x);
	void setPlayRange(Range<double> playrange, bool isloop);
	Range<double> getPlayRange() { return m_playrange; }
	bool isLoopEnabled();
	bool hasReachedEnd();
    bool isResampling();
	std::vector<int> getSpectrumProcessOrder();
	void setSpectrumProcessOrder(std::vector<int> order);
	void setFFTWindowingType(int windowtype);
    int getFFTWindowingType() { return m_fft_window_type; }
    std::pair<Range<double>,Range<double>> getFileCachedRangesNormalized();
	Value val_MainVolume;
    Value val_XFadeLen;
	ValueTree getStateTree();
	void setStateTree(ValueTree state);
	void setClippingEnabled(bool b) { m_clip_output = b; }
	bool isLoopingEnabled();
	void setLoopingEnabled(bool b);
	void setMaxLoops(int64_t numloops) { m_maxloops = numloops; }
	void setAudioBufferAsInputSource(AudioBuffer<float>* buf, int sr, int len);
private:
	CircularBuffer<float> m_stretchoutringbuf{ 1024 * 1024 };
	AudioBuffer<float> m_file_inbuf;
	LinearSmoothedValue<double> m_vol_smoother;
	std::unique_ptr<AInputS> m_inputfile;
	std::vector<std::shared_ptr<ProcessedStretch>> m_stretchers;
	std::unique_ptr<BinauralBeats> m_binaubeats;
	float2dvector m_inbufs;
	std::function<void(StretchAudioSource*)> SourceEndedCallback;
	bool m_firstbuffer = false;
	bool m_output_has_begun = false;
	int m_num_outchans = 0;
	double m_outsr = 44100.0;
	int m_process_fftsize = 0;
    int m_fft_window_type = -1;
	ProcessParameters m_ppar;
	BinauralBeatsParameters	m_bbpar;
	double m_playrate = 1.0;
	double m_lastplayrate = 0.0;
	double m_onsetdetection = 0.0;
	double m_seekpos = 0.0;
	double m_lastinpos = 0.0;
	bool m_freezing = false;
	Range<double> m_playrange{ 0.0,1.0 };
	
	bool m_stream_end_reached = false;
	int64_t m_output_silence_counter = 0;
	File m_curfile;
	int64_t m_maxloops = 0;
	std::unique_ptr<WDL_Resampler> m_resampler;
	std::vector<double> m_resampler_outbuf;
	std::mutex m_mutex;
	std::vector<int> m_specproc_order;
	bool m_stop_play_requested = false;
	double m_freeze_pos = 0.0;
	int64_t m_output_counter = 0;
	int64_t m_output_length = 0;
	bool m_clip_output = true;
	void initObjects();
	AudioFormatManager* m_afm = nullptr;
};

class MultiStretchAudioSource final : public PositionableAudioSource
{
public:
	MultiStretchAudioSource() {}
	MultiStretchAudioSource(int initialnumoutchans, AudioFormatManager* afm);
	~MultiStretchAudioSource();
	
	void prepareToPlay(int samplesPerBlockExpected, double sampleRate) override;

	void releaseResources() override;

	void getNextAudioBlock(const AudioSourceChannelInfo & bufferToFill) override;

	void setNextReadPosition(int64 newPosition) override;

	int64 getNextReadPosition() const override;

	int64 getTotalLength() const override;

	bool isLooping() const override;

	String setAudioFile(File file);
	File getAudioFile();

	void setNumOutChannels(int chans);
	double getInfilePositionPercent();
	void setRate(double rate);
	double getRate();
	void setProcessParameters(ProcessParameters* pars);
	void setFFTSize(int size);
	int getFFTSize();

	void seekPercent(double pos);
	double getInfilePositionSeconds();
	double getInfileLengthSeconds();

	double getOutputDurationSecondsForRange(Range<double> range, int fftsize);

	void setOnsetDetection(double x);
	void setPlayRange(Range<double> playrange, bool isloop);
	bool isLoopingEnabled();
	void setLoopingEnabled(bool b);
	bool hasReachedEnd();
	bool isResampling();
	std::vector<int> getSpectrumProcessOrder();
	void setSpectrumProcessOrder(std::vector<int> order);
    void setFFTWindowingType(int windowtype);
    std::pair<Range<double>, Range<double>> getFileCachedRangesNormalized();
	void setFreezing(bool b) { m_freezing = b; }
	bool isFreezing() { return m_freezing; }
	
	Value val_MainVolume;
	Value val_XFadeLen;
	//ValueTree getStateTree();
	//void setStateTree(ValueTree state);
	void setAudioBufferAsInputSource(AudioBuffer<float>* buf, int sr, int len);
private:
	std::vector<std::shared_ptr<StretchAudioSource>> m_stretchsources;
	bool m_is_in_switch{ false };
	bool m_is_playing = false;
	bool m_freezing = false;
	LinearSmoothedValue<double> m_xfadegain;
	StretchAudioSource* getActiveStretchSource() const;
	void switchActiveSource();
	int m_blocksize = 0;
	double m_samplerate = 44100.0;
	int m_numoutchans = 2;
	AudioBuffer<float> m_processbuffers[2];
	std::mutex m_mutex;
    double m_switchxfadelen = 1.0;
	AudioFormatManager* m_afm = nullptr;
};
