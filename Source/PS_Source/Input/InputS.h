// SPDX-License-Identifier: GPLv3-or-later WITH Appstore-exception
// Copyright (C) 2006-2011 Nasca Octavian Paul
// Author: Nasca Octavian Paul


#pragma once

#include "../globals.h"

#include "../JuceLibraryCode/JuceHeader.h"

class InputS
{
public:
		InputS()
        {
			skipbufsize=1024;
			skipbuf.setSize(1, skipbufsize);
		};

		virtual ~InputS()
        {
			
		};
		[[nodiscard]] virtual bool openAudioFile(const URL & url)=0;
		virtual void close()=0;

		virtual int readNextBlock(AudioBuffer<float>& abuf, int smps, int numchans)=0;
		void skip(int nsmps)
		{
			while ((nsmps>0)&&(m_silenceoutputted<1))
			{
				int readsize=(nsmps<skipbufsize)?nsmps:skipbufsize;
				if (skipbuf.getNumSamples() < readsize)
					skipbuf.setSize(1, readsize);
				readNextBlock(skipbuf,readsize,1);
				nsmps-=readsize;
			};
		};
		virtual void seek(double pos, bool immediate)=0;//0=start,1.0=end

		struct {
			int64_t nsamples=0;
			int nchannels=0;
			int samplerate=0;
			
		} info;
		
    
    int getSilenceOutputtedAfterActiveRange()
	{
		return m_silenceoutputted;
	}
	Range<double> getActiveRange() { return m_activerange; }
	double getLengthSeconds()
	{
		if (info.nsamples == 0 || info.samplerate==0)
			return 0.0;
		return (double)info.nsamples / info.samplerate;
	}
	virtual void setActiveRange(Range<double> rng) = 0;
	bool isLooping() { return m_loop_enabled; }
	virtual void setLoopEnabled(bool b) = 0;
	int64_t getCurrentPosition() { return m_currentsample; }
	double getCurrentPositionPercent() { return 1.0 / info.nsamples*m_currentsample; }
	virtual bool hasEnded()
	{
		return m_currentsample >= info.nsamples*m_activerange.getEnd();
	}
    virtual AudioBuffer<float>* getAudioBuffer()=0;
	int64_t getDiskReadSampleCount() { return m_disk_read_count; }
protected:
	volatile int64_t m_currentsample = 0;
	int m_silenceoutputted = 0;
	bool m_loop_enabled = false;
	Range<double> m_activerange{ 0.0,1.0 };
	int64_t m_disk_read_count = 0;
private:
	int skipbufsize;
	AudioBuffer<float> skipbuf;
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(InputS)
};
