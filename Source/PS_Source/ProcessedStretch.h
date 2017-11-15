/*
  Copyright (C) 2011 Nasca Octavian Paul
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

#include "FreeEdit.h"
#include "Stretch.h"

struct ProcessParameters
{
	ProcessParameters()
	{
		pitch_shift.enabled=false;
		pitch_shift.cents=0;

		octave.enabled=false;
		octave.om2=octave.om1=octave.o1=octave.o15=octave.o2=0.0f;
		octave.o0=1.0f;

		freq_shift.enabled=false;
		freq_shift.Hz=0;

		compressor.enabled=false;
		compressor.power=0.0f;

		filter.enabled=false;
		filter.stop=false;
		filter.low=0.0f;
		filter.high=22000.0f;
		filter.hdamp=0.0f;

		harmonics.enabled=false;
		harmonics.freq=440.0f;
		harmonics.bandwidth=25.0f;
		harmonics.nharmonics=10;
		harmonics.gauss=false;

		spread.enabled=false;
		spread.bandwidth=0.3f;
		
		tonal_vs_noise.enabled=false;
		tonal_vs_noise.preserve=0.5f;
		tonal_vs_noise.bandwidth=0.9f;
	};
	~ProcessParameters(){
	};
	
	
	struct{
		bool enabled;
		int cents;
	}pitch_shift;

	struct{
		bool enabled;
		REALTYPE om2,om1,o0,o1,o15,o2;
	}octave;

	struct{
		bool enabled;
		int Hz;
	}freq_shift;

	struct{
		bool enabled;
		REALTYPE power;
	}compressor;

	struct{
		bool enabled;
		REALTYPE low,high;
		REALTYPE hdamp;
		bool stop;
	}filter;    

	struct{
		bool enabled;
		REALTYPE freq;
		REALTYPE bandwidth;
		int nharmonics;
		bool gauss;
	}harmonics;

	struct{
		bool enabled;
		REALTYPE bandwidth;
	}spread;

	struct{
		bool enabled;
		REALTYPE preserve;
		REALTYPE bandwidth;
	}tonal_vs_noise;

	//FreeEdit free_filter;
	//FreeEdit stretch_multiplier;
	/*
	auto getMembers() const
	{
		return std::make_tuple(pitch_shift.enabled,
			pitch_shift.cents,
			octave.enabled,
			octave.o0,
			octave.o1,
			octave.o15,
			octave.o2,
			octave.om1,
			octave.om2,
			spread.enabled,
			spread.bandwidth,
			tonal_vs_noise.enabled,
			tonal_vs_noise.bandwidth,
			tonal_vs_noise.preserve,
			freq_shift.enabled,
			freq_shift.Hz,
			compressor.enabled,
			compressor.power,
			harmonics.bandwidth,
			harmonics.enabled,
			harmonics.freq,
			harmonics.gauss,
			harmonics.nharmonics,
			filter.enabled,
			filter.hdamp,
			filter.high,
			filter.low,
			filter.stop);
	}
	bool operator == (const ProcessParameters& other) const
	{
		return getMembers() == other.getMembers();
	}
	*/
	bool operator == (const ProcessParameters& other) const noexcept
	{
		return pitch_shift.enabled == other.pitch_shift.enabled &&
			pitch_shift.cents == other.pitch_shift.cents &&
			octave.enabled == other.octave.enabled &&
			octave.o0 == other.octave.o0 &&
			octave.o1 == other.octave.o1 &&
			octave.o15 == other.octave.o15 &&
			octave.o2 == other.octave.o2 &&
			octave.om1 == other.octave.om1 &&
			octave.om2 == other.octave.om2 &&
			spread.enabled == other.spread.enabled &&
			spread.bandwidth == other.spread.bandwidth &&
			tonal_vs_noise.enabled == other.tonal_vs_noise.enabled &&
			tonal_vs_noise.bandwidth == other.tonal_vs_noise.bandwidth &&
			tonal_vs_noise.preserve == other.tonal_vs_noise.preserve &&
			freq_shift.enabled == other.freq_shift.enabled &&
			freq_shift.Hz == other.freq_shift.Hz &&
			compressor.enabled == other.compressor.enabled &&
			compressor.power == other.compressor.power &&
			harmonics.bandwidth == other.harmonics.bandwidth &&
			harmonics.enabled == other.harmonics.enabled &&
			harmonics.freq == other.harmonics.freq &&
			harmonics.gauss == other.harmonics.gauss &&
			harmonics.nharmonics == other.harmonics.nharmonics &&
			filter.enabled == other.filter.enabled &&
			filter.hdamp == other.filter.hdamp &&
			filter.high == other.filter.high &&
			filter.low == other.filter.low &&
			filter.stop == other.filter.stop;
	}
};

class SpectrumProcess
{
public:
    SpectrumProcess() {}
    SpectrumProcess(String name, int index) :
    m_name(name), m_index(index) {}
    String m_name;
	int m_index = -1;
private:
};

std::vector<SpectrumProcess> make_spectrum_processes();

class ProcessedStretch final : public Stretch
{
public:
    
    //stereo_mode: 0=mono,1=left,2=right
    ProcessedStretch(REALTYPE rap_,int in_bufsize_,FFTWindow w=W_HAMMING,bool bypass_=false,REALTYPE samplerate_=44100.0f,int stereo_mode=0);
    ~ProcessedStretch();
    void set_parameters(ProcessParameters *ppar);
	std::vector<int> m_spectrum_processes;
	void setBufferSize(int sz) override;
private:
    REALTYPE get_stretch_multiplier(REALTYPE pos_percents) override;
//		void process_output(REALTYPE *smps,int nsmps);
    void process_spectrum(REALTYPE *freq) override;
    void do_harmonics(REALTYPE *freq1,REALTYPE *freq2);
    void do_pitch_shift(REALTYPE *freq1,REALTYPE *freq2,REALTYPE rap);
    void do_freq_shift(REALTYPE *freq1,REALTYPE *freq2);
    void do_octave(REALTYPE *freq1,REALTYPE *freq2);
    void do_filter(REALTYPE *freq1,REALTYPE *freq2);
    void do_free_filter(REALTYPE *freq1,REALTYPE *freq2);
    void do_compressor(REALTYPE *freq1,REALTYPE *freq2);
    void do_spread(REALTYPE *freq1,REALTYPE *freq2);
    void do_tonal_vs_noise(REALTYPE *freq1,REALTYPE *freq2);

    //void copy(const realvector& freq1,realvector& freq2);
    void copy(REALTYPE* freq1, REALTYPE* freq2);
    void add(REALTYPE *freq2,REALTYPE *freq1,REALTYPE a=1.0);
    void mul(REALTYPE *freq1,REALTYPE a);
    void zero(REALTYPE *freq1);
    void spread(REALTYPE *freq1,REALTYPE *freq2,REALTYPE spread_bandwidth);

    void update_free_filter();
    int nfreq=0;

    std::vector<REALTYPE> free_filter_freqs;
    ProcessParameters pars;
    
    std::vector<REALTYPE> infreq,sumfreq,tmpfreq1,tmpfreq2;
    
		//REALTYPE *fbfreq;
};
