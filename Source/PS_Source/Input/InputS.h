/*
  Copyright (C) 2006-2011 Nasca Octavian Paul
  Author: Nasca Octavian Paul

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
		virtual bool openAudioFile(File file)=0;
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
		virtual void seek(double pos)=0;//0=start,1.0=end

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
	virtual bool hasEnded()
	{
		return m_currentsample >= info.nsamples*m_activerange.getEnd();
	}
	
protected:
	volatile int64_t m_currentsample = 0;
	int m_silenceoutputted = 0;
	bool m_loop_enabled = false;
	Range<double> m_activerange{ 0.0,1.0 };
	
private:
	int skipbufsize;
	AudioBuffer<float> skipbuf;
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(InputS)
};
