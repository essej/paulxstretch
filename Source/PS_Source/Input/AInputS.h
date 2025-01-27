// SPDX-License-Identifier: GPLv3-or-later WITH Appstore-exception
// Copyright (C) 2006-2011 Nasca Octavian Paul
// Author: Nasca Octavian Paul

#pragma once

#include <string>

#include "../JuceLibraryCode/JuceHeader.h"

#include "InputS.h"
#include <mutex>

inline double ramp(int64_t pos, int64_t totallen, int64_t rampinlen, int64_t rampoutlen)
{
	if (totallen < rampinlen + rampoutlen)
		return 1.0;
	if (pos < rampinlen)
		return 1.0 / rampinlen*pos;
	if (pos >= totallen - rampoutlen)
		return 1.0 - 1.0 / rampoutlen*(pos - totallen + rampoutlen);
	return 1.0;
}

class AInputS final : public InputS
{
public:
    AInputS(AudioFormatManager* mana) : m_manager(mana)
	{
		m_readbuf.setSize(2, 65536*2);
		m_readbuf.clear();
		m_crossfadebuf.setSize(2, 44100);
		m_crossfadebuf.clear();
        PlayRangeEndCallback=[](AInputS*){};
	}
    ~AInputS() {}

	void setAudioBuffer(AudioBuffer<float>* buf, int samplerate, int len)
	{
		ScopedLock locker(m_mutex);
        m_afreader = nullptr;
		m_using_memory_buffer = true;
		m_readbuf = *buf;
		info.nchannels = buf->getNumChannels();
		info.nsamples = len;
		info.samplerate = samplerate;
		m_currentsample = 0;
		m_loop_enabled = true;
		m_crossfadebuf.setSize(info.nchannels, m_crossfadebuf.getNumSamples());
		m_cached_file_range = { 0,len };
		seek(m_activerange.getStart(), true);
		updateXFadeCache();
	}
    virtual AudioBuffer<float>* getAudioBuffer() override
    {
        if (m_using_memory_buffer)
            return &m_readbuf;
        return nullptr;
    }
	bool openAudioFile(const URL & url) override
    {
		m_silenceoutputted = 0;
		AudioFormatReader* reader = m_manager->createReaderFor(url.getLocalFile());
        if (reader)
        {
			ScopedLock locker(m_mutex);
            m_using_memory_buffer = false;
			m_afreader = std::unique_ptr<AudioFormatReader>(reader);
			if (m_activerange.isEmpty())
				m_activerange = { 0.0,1.0 };
			m_currentsample = m_activerange.getStart()*info.nsamples;
            info.samplerate = (int)m_afreader->sampleRate;
            info.nchannels = m_afreader->numChannels;
            info.nsamples = m_afreader->lengthInSamples;
			if (m_readbuf.getNumChannels() < info.nchannels)
			{
				m_readbuf.setSize(info.nchannels, m_readbuf.getNumSamples());
				m_crossfadebuf.setSize(info.nchannels, m_crossfadebuf.getNumSamples());
			}
			updateXFadeCache();
			m_readbuf.clear();
            return true;
        }
        return false;
    }
	void close() override
    {
		m_afreader = nullptr;
		m_currentsample = 0;
		info.nchannels = 0;
		info.nsamples = 0;
		info.samplerate = 0;
    }
	int readNextBlock(AudioBuffer<float>& abuf, int nsmps, int numchans) override
	{
		ScopedLock locker(m_mutex);
        if (m_afreader == nullptr && m_using_memory_buffer == false)
        {
            jassert(false);
            return 0;
        }
		int inchans = 0;
		if (m_afreader)
			inchans = m_afreader->numChannels;
		else inchans = m_readbuf.getNumChannels();
		int64_t subsect_t0 = 0;
		int64_t subsect_t1 = 0;
		int64_t subsectlen = 0;
		int xfadelen = 0;
		auto updatesamplepositions = [&,this]()
		{
			subsect_t0 = (int64_t)(m_activerange.getStart()*info.nsamples);
			subsect_t1 = (int64_t)(m_activerange.getEnd()*info.nsamples);
			subsectlen = subsect_t1 - subsect_t0;
			xfadelen = m_xfadelen;
			if (xfadelen >= subsectlen)
				xfadelen = int(subsectlen - 2);
		};
		updatesamplepositions();
		auto getSampleLambda=[this](int64_t pos, int ch)
		{
			if (m_cached_file_range.contains(pos))
                return m_readbuf.getSample(ch, int(pos - m_cached_file_range.getStart()));
            else
            {
                Range<int64_t> activerange((int64_t)(m_activerange.getStart()*info.nsamples), 
					(int64_t)(m_activerange.getEnd()*info.nsamples+1));
                Range<int64_t> possiblerange(pos, pos + m_readbuf.getNumSamples() + 0);
                m_cached_file_range = activerange.getIntersectionWith(possiblerange);
                m_afreader->read(&m_readbuf, 0, (int)m_cached_file_range.getLength(), pos, true, true);
				m_disk_read_count += m_cached_file_range.getLength()*m_afreader->numChannels;
				return m_readbuf.getSample(ch, int(pos - m_cached_file_range.getStart()));
            }
        };
		auto getCrossFadedSampleLambda=[this,&getSampleLambda](int64_t playpos, int chan, int64_t subt0, int64_t subt1, int xfadelen)
		{
			if (m_loop_enabled == false && playpos >= subt1)
			{
				return 0.0f;
			}
			if (playpos >= subt0 && playpos <= subt1 - xfadelen)
				return getSampleLambda(playpos, chan);
			if (playpos > (subt1 - xfadelen) && playpos<subt1)
			{
				int64_t fadeindex = playpos - subt1 + xfadelen;
				double fadeoutgain = 1.0 - (1.0 / (xfadelen - 0))*fadeindex;
				float s0 = (float)(getSampleLambda(playpos, chan)*fadeoutgain);
				double fadeingain = (1.0 / (xfadelen - 0))*fadeindex;
                int64_t playpos2 = playpos - subt1 + xfadelen;
                jassert(playpos2>=0 && playpos2<=xfadelen);
                float s1 = (float)(m_crossfadebuf.getSample(chan, (int)playpos2)*fadeingain);
				return s0 + s1;
			}
			++m_silenceoutputted;
			return 0.0f;
		};
		float* const* smps = abuf.getArrayOfWritePointers();
		int readinc = 1;
		if (m_reverseplay)
			readinc = -1;
		for (int i = 0; i < nsmps; ++i)
		{
			float seekfadegain = 1.0f;
			if (m_seekfade.state == 1)
			{
				//Logger::writeToLog("Seek requested to pos " + String(m_seekfade.requestedpos));
				m_seekfade.state = 2;
			}
			if (m_seekfade.state == 2)
			{
				seekfadegain = 1.0 - (1.0 / m_seekfade.length*m_seekfade.counter);
				++m_seekfade.counter;
				if (m_seekfade.counter >= m_seekfade.length)
				{
					//Logger::writeToLog("Doing seek " + String(m_seekfade.requestedpos));
					m_seekfade.counter = 0;
					m_seekfade.state = 3;
					if (m_seekfade.requestedrange.isEmpty() == false)
					{
						setActiveRangeImpl(m_seekfade.requestedrange);
						updatesamplepositions();
						if (m_activerange.contains(getCurrentPositionPercent()) == false)
							seekImpl(m_activerange.getStart());
					}

				}
			}
			if (m_seekfade.state == 3)
			{
				seekfadegain = 1.0 / m_seekfade.length*m_seekfade.counter;
				++m_seekfade.counter;
				if (m_seekfade.counter >= m_seekfade.length)
				{
					//Logger::writeToLog("Seek cycle finished");
					m_seekfade.counter = 0;
					m_seekfade.state = 0;
					m_seekfade.requestedpos = 0.0;
					m_seekfade.requestedrange = Range<double>();
				}
			}
			jassert(seekfadegain >= 0.0f && seekfadegain<=1.0f);
			if (inchans == 1 && numchans > 0)
			{
				float sig = getCrossFadedSampleLambda(m_currentsample, 0, subsect_t0, subsect_t1, xfadelen);
				for (int j = 0; j < numchans; ++j)
				{
					smps[j][i] = sig*seekfadegain;
				}
			}
			else if (inchans > 1 && numchans > 1)
			{
				for (int j = 0; j < numchans; ++j)
				{
					int inchantouse = j % inchans;
					smps[j][i] = seekfadegain*getCrossFadedSampleLambda(m_currentsample, inchantouse, 
						subsect_t0, subsect_t1,xfadelen);
				}

			}
			m_currentsample += readinc;
			if (m_loop_enabled == true)
			{
				if (m_reverseplay == false && m_currentsample >= subsect_t1)
				{
					m_currentsample = subsect_t0+xfadelen;
					++m_loopcount;
				} 
				else if (m_reverseplay == true && m_currentsample < subsect_t0)
				{
					m_currentsample = subsect_t1 - 1;
				}
			} else
            {
				if (m_reverseplay == false && m_currentsample == subsect_t1)
					PlayRangeEndCallback(this);
				else if (m_reverseplay == true && m_currentsample == subsect_t0)
					PlayRangeEndCallback(this);
            }
		}
		
		return nsmps;
	}
	void seekImpl(double pos)
	{
		if (m_using_memory_buffer == true)
		{
			jassert(m_readbuf.getNumSamples() > 0 && m_afreader == nullptr);
			m_loopcount = 0;
			m_silenceoutputted = 0;
			m_cache_misses = 0;
			m_currentsample = (int64_t)(pos*m_readbuf.getNumSamples());
			m_currentsample = jlimit<int64_t>(0, m_readbuf.getNumSamples(), m_currentsample);
			m_cached_file_range = { 0,m_readbuf.getNumSamples() };
			return;
		}
		//jassert(m_afreader!=nullptr);
		if (m_afreader == nullptr)
			return;
		m_loopcount = 0;
		m_silenceoutputted = 0;
		m_cache_misses = 0;
		m_currentsample = (int64_t)(pos*m_afreader->lengthInSamples);
		m_currentsample = jlimit<int64_t>(0, m_afreader->lengthInSamples, m_currentsample);
		//Logger::writeToLog("Seeking to " + String(m_currentsample));
		//if (m_cached_file_range.contains(info.currentsample)==false)
		m_cached_file_range = Range<int64_t>();
		updateXFadeCache();
		//m_cached_crossfade_range = Range<int64_t>();
	}
	void seek(double pos, bool immediate) override //0=start,1.0=end
    {
		ScopedLock locker(m_mutex);
		if (immediate == true)
		{
			seekImpl(pos);
			return;
		}
		if (m_seekfade.state == 0)
		{
			m_seekfade.state = 1;
			m_seekfade.counter = 0;
		}
		m_seekfade.length = 16384;
		m_seekfade.requestedpos = pos;
		
	}

	std::pair<Range<double>,Range<double>> getCachedRangesNormalized()
	{
		if (m_afreader == nullptr)
			return {};
		return { { jmap<double>((double)m_cached_file_range.getStart(),0,(double)info.nsamples,0.0,1.0),
			jmap<double>((double)m_cached_file_range.getEnd(), 0, (double)info.nsamples, 0.0, 1.0) },
			{ jmap<double>((double)m_cached_crossfade_range.getStart(),0,(double)info.nsamples,0.0,1.0),
			jmap<double>((double)m_cached_crossfade_range.getEnd(), 0, (double)info.nsamples, 0.0, 1.0) } };
	}
	int getNumCacheMisses() { return m_cache_misses; }
    void updateXFadeCache()
    {
        if (m_xfadelen>m_crossfadebuf.getNumSamples())
            m_crossfadebuf.setSize(info.nchannels,m_xfadelen);
        if (m_afreader != nullptr && m_using_memory_buffer == false)
			m_afreader->read(&m_crossfadebuf, 0, m_xfadelen, (int64_t)(m_activerange.getStart()*info.nsamples), true, true);
		if (m_afreader == nullptr && m_using_memory_buffer == true)
		{
			for (int i=0;i<info.nchannels;++i)
				m_crossfadebuf.copyFrom(i, 0, m_readbuf, i, (int64_t)(m_activerange.getStart()*info.nsamples), m_xfadelen);
		}
		m_cached_crossfade_range =
            Range<int64_t>((int64_t)(m_activerange.getStart()*info.nsamples),(int64_t)(m_activerange.getStart()*info.nsamples+m_xfadelen));
    }
	void setActiveRangeImpl(Range<double> rng)
	{
		if (rng.getEnd() < rng.getStart())
			rng = { 0.0,1.0 };
		if (rng.isEmpty())
			rng = { 0.0,1.0 };
		m_activerange = rng;
		m_loopcount = 0;
		updateXFadeCache();
	}
	void setActiveRange(Range<double> rng) override
	{
		ScopedLock locker(m_mutex);
		
		/*
		if (rng.contains(getCurrentPositionPercent()))
		{
			setActiveRangeImpl(rng);
			return;
		}
		*/
		m_seekfade.requestedrange = rng;
		if (m_seekfade.state == 0)
		{
			m_seekfade.counter = 0;
			m_seekfade.state = 1;
		}
		else
		{
			m_seekfade.state = 0;
			setActiveRangeImpl(rng);
		}
		m_seekfade.length = 2048;
    }
	void setLoopEnabled(bool b) override
	{
		m_loop_enabled = b;
		m_loopcount = 0;
        updateXFadeCache();
	}
    void setXFadeLenSeconds(double len)
    {
        if (info.samplerate==0)
            return;
        len = jlimit(0.0,1.0,len);
        int temp = (int)(len*info.samplerate);
        if (m_xfadelen!=temp)
        {
            m_xfadelen = temp;
            updateXFadeCache();
        }
    }
	Range<int64_t> getActiveRangeFrames()
	{
		if (info.nsamples == 0)
			return Range<int64_t>();
		return Range<int64_t>((int64_t)(m_activerange.getStart()*info.nsamples), (int64_t)(m_activerange.getEnd()*info.nsamples));
	}
	void setReversePlay(bool b)
	{
		m_reverseplay = b;
	}
	bool isReversed() { return m_reverseplay; }
	int64_t getLoopCount() { return m_loopcount; }
	
private:
	std::function<void(AInputS*)> PlayRangeEndCallback;
	std::unique_ptr<AudioFormatReader> m_afreader;
	AudioBuffer<float> m_readbuf;
	AudioBuffer<float> m_crossfadebuf;
	Range<int64_t> m_cached_file_range;
	Range<int64_t> m_cached_crossfade_range;
	int m_cache_misses = 0;
	int m_fade_in = 512;
	int m_fade_out = 512;
	int m_xfadelen = 0;
	bool m_reverseplay = false;
	int64_t m_loopcount = 0;
	bool m_using_memory_buffer = true;
	AudioFormatManager* m_manager = nullptr;
    CriticalSection m_mutex;
	struct
	{
		int state = 0; // 0 inactive, 1 seek requested, 2 fade out, 3 fade in
		int counter = 0;
		int length = 44100;
		double requestedpos = 0.0;
		Range<double> requestedrange;
	} m_seekfade;
};
