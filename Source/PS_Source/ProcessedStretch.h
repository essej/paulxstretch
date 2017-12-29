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

inline REALTYPE profile(REALTYPE fi, REALTYPE bwi) {
	REALTYPE x = fi / bwi;
	x *= x;
	if (x>14.71280603) return 0.0;
	return exp(-x);///bwi;

};

inline void spectrum_copy(int nfreq, REALTYPE* freq1, REALTYPE* freq2)
{
	for (int i = 0; i<nfreq; i++) freq2[i] = freq1[i];
};


inline void spectrum_spread(int nfreq, double samplerate, 
	std::vector<REALTYPE>& tmpfreq1,
	REALTYPE *freq1, REALTYPE *freq2, REALTYPE spread_bandwidth) {
	//convert to log spectrum
	REALTYPE minfreq = 20.0f;
	REALTYPE maxfreq = 0.5f*samplerate;

	REALTYPE log_minfreq = log(minfreq);
	REALTYPE log_maxfreq = log(maxfreq);

	for (int i = 0; i<nfreq; i++) {
		REALTYPE freqx = i / (REALTYPE)nfreq;
		REALTYPE x = exp(log_minfreq + freqx * (log_maxfreq - log_minfreq)) / maxfreq * nfreq;
		REALTYPE y = 0.0f;
		int x0 = (int)floor(x); if (x0 >= nfreq) x0 = nfreq - 1;
		int x1 = x0 + 1; if (x1 >= nfreq) x1 = nfreq - 1;
		REALTYPE xp = x - x0;
		if (x<nfreq) {
			y = freq1[x0] * (1.0f - xp) + freq1[x1] * xp;
		};
		tmpfreq1[i] = y;
	};

	//increase the bandwidth of each harmonic (by smoothing the log spectrum)
	int n = 2;
	REALTYPE bandwidth = spread_bandwidth;
	REALTYPE a = 1.0f - pow(2.0f, -bandwidth * bandwidth*10.0f);
	a = pow(a, 8192.0f / nfreq * n);

	for (int k = 0; k<n; k++) {
		tmpfreq1[0] = 0.0f;
		for (int i = 1; i<nfreq; i++) {
			tmpfreq1[i] = tmpfreq1[i - 1] * a + tmpfreq1[i] * (1.0f - a);
		};
		tmpfreq1[nfreq - 1] = 0.0f;
		for (int i = nfreq - 2; i>0; i--) {
			tmpfreq1[i] = tmpfreq1[i + 1] * a + tmpfreq1[i] * (1.0f - a);
		};
	};

	freq2[0] = 0;
	REALTYPE log_maxfreq_d_minfreq = log(maxfreq / minfreq);
	for (int i = 1; i<nfreq; i++) {
		REALTYPE freqx = i / (REALTYPE)nfreq;
		REALTYPE x = log((freqx*maxfreq) / minfreq) / log_maxfreq_d_minfreq * nfreq;
		REALTYPE y = 0.0;
		if ((x>0.0) && (x<nfreq)) {
			int x0 = (int)floor(x); if (x0 >= nfreq) x0 = nfreq - 1;
			int x1 = x0 + 1; if (x1 >= nfreq) x1 = nfreq - 1;
			REALTYPE xp = x - x0;
			y = tmpfreq1[x0] * (1.0f - xp) + tmpfreq1[x1] * xp;
		};
		freq2[i] = y;
	};


};


inline void spectrum_do_compressor(const ProcessParameters& pars, int nfreq, REALTYPE *freq1, REALTYPE *freq2) {
	REALTYPE rms = 0.0;
	for (int i = 0; i<nfreq; i++) rms += freq1[i] * freq1[i];
	rms = sqrt(rms / nfreq)*0.1f;
	if (rms<1e-3f) rms = 1e-3f;

	REALTYPE _rap = pow(rms, -pars.compressor.power);
	for (int i = 0; i<nfreq; i++) freq2[i] = freq1[i] * _rap;
};

inline void spectrum_do_tonal_vs_noise(const ProcessParameters& pars, int nfreq, double samplerate,
	std::vector<REALTYPE>& tmpfreq1,
	REALTYPE *freq1, REALTYPE *freq2) {
	spectrum_spread(nfreq, samplerate, tmpfreq1, freq1, tmpfreq1.data(), pars.tonal_vs_noise.bandwidth);

	if (pars.tonal_vs_noise.preserve >= 0.0) {
		REALTYPE mul = (pow(10.0f, pars.tonal_vs_noise.preserve) - 1.0f);
		for (int i = 0; i<nfreq; i++) {
			REALTYPE x = freq1[i];
			REALTYPE smooth_x = tmpfreq1[i] + 1e-6f;

			REALTYPE result = 0.0f;
			result = x - smooth_x * mul;
			if (result<0.0f) result = 0.0f;
			freq2[i] = result;
		};
	}
	else {
		REALTYPE mul = (pow(5.0f, 1.0f + pars.tonal_vs_noise.preserve) - 1.0f);
		for (int i = 0; i<nfreq; i++) {
			REALTYPE x = freq1[i];
			REALTYPE smooth_x = tmpfreq1[i] + 1e-6f;

			REALTYPE result = 0.0f;
			result = x - smooth_x * mul + 0.1f*mul;
			if (result<0.0f) result = x;
			else result = 0.0f;

			freq2[i] = result;
		};
	};

};

inline void spectrum_do_harmonics(const ProcessParameters& pars, std::vector<REALTYPE>& tmpfreq1, int nfreq, double samplerate, REALTYPE *freq1, REALTYPE *freq2) {
	REALTYPE freq = pars.harmonics.freq;
	REALTYPE bandwidth = pars.harmonics.bandwidth;
	int nharmonics = pars.harmonics.nharmonics;

	if (freq<10.0) freq = 10.0;

	REALTYPE *amp = tmpfreq1.data();
	for (int i = 0; i<nfreq; i++) amp[i] = 0.0;

	for (int nh = 1; nh <= nharmonics; nh++) {//for each harmonic
		REALTYPE bw_Hz;//bandwidth of the current harmonic measured in Hz
		REALTYPE bwi;
		REALTYPE fi;
		REALTYPE f = nh * freq;

		if (f >= samplerate / 2) break;

		bw_Hz = (pow(2.0f, bandwidth / 1200.0f) - 1.0f)*f;
		bwi = bw_Hz / (2.0f*samplerate);
		fi = f / samplerate;

		REALTYPE sum = 0.0f;
		REALTYPE max = 0.0f;
		for (int i = 1; i<nfreq; i++) {//todo: optimize here
			REALTYPE hprofile;
			hprofile = profile((i / (REALTYPE)nfreq*0.5f) - fi, bwi);
			amp[i] += hprofile;
			if (max<hprofile) max = hprofile;
			sum += hprofile;
		};
	};

	REALTYPE max = 0.0;
	for (int i = 1; i<nfreq; i++) {
		if (amp[i]>max) max = amp[i];
	};
	if (max<1e-8f) max = 1e-8f;

	for (int i = 1; i<nfreq; i++) {
		//REALTYPE c,s;
		REALTYPE a = amp[i] / max;
		if (!pars.harmonics.gauss) a = (a<0.368f ? 0.0f : 1.0f);
		freq2[i] = freq1[i] * a;
	};

};

inline void spectrum_add(int nfreq, REALTYPE *freq2, REALTYPE *freq1, REALTYPE a) {
	for (int i = 0; i<nfreq; i++) freq2[i] += freq1[i] * a;
};

inline void spectrum_zero(int nfreq,REALTYPE *freq1) {
	for (int i = 0; i<nfreq; i++) freq1[i] = 0.0;
};

inline void spectrum_do_freq_shift(const ProcessParameters& pars, int nfreq, double samplerate, REALTYPE *freq1, REALTYPE *freq2) {
	spectrum_zero(nfreq, freq2);
	int ifreq = (int)(pars.freq_shift.Hz / (samplerate*0.5)*nfreq);
	for (int i = 0; i<nfreq; i++) {
		int i2 = ifreq + i;
		if ((i2>0) && (i2<nfreq)) freq2[i2] = freq1[i];
	};
};

inline void spectrum_do_pitch_shift(const ProcessParameters& pars, int nfreq, REALTYPE *freq1, REALTYPE *freq2, REALTYPE _rap) {
	spectrum_zero(nfreq,freq2);
	if (_rap<1.0) {//down
		for (int i = 0; i<nfreq; i++) {
			int i2 = (int)(i*_rap);
			if (i2 >= nfreq) break;
			freq2[i2] += freq1[i];
		};
	};
	if (_rap >= 1.0) {//up
		_rap = 1.0f / _rap;
		for (int i = 0; i<nfreq; i++) {
			freq2[i] = freq1[(int)(i*_rap)];
		};

	};
};

inline void spectrum_do_octave(const ProcessParameters& pars, int nfreq, double samplerate, 
	std::vector<REALTYPE>& sumfreq, 
	std::vector<REALTYPE>& tmpfreq1,
	REALTYPE *freq1, REALTYPE *freq2) {
	spectrum_zero(nfreq,sumfreq.data());
	if (pars.octave.om2>1e-3) {
		spectrum_do_pitch_shift(pars,nfreq, freq1, tmpfreq1.data(), 0.25);
		spectrum_add(nfreq, sumfreq.data(), tmpfreq1.data(), pars.octave.om2);
	};
	if (pars.octave.om1>1e-3) {
		spectrum_do_pitch_shift(pars,nfreq, freq1, tmpfreq1.data(), 0.5);
		spectrum_add(nfreq,sumfreq.data(), tmpfreq1.data(), pars.octave.om1);
	};
	if (pars.octave.o0>1e-3) {
		spectrum_add(nfreq,sumfreq.data(), freq1, pars.octave.o0);
	};
	if (pars.octave.o1>1e-3) {
		spectrum_do_pitch_shift(pars,nfreq, freq1, tmpfreq1.data(), 2.0);
		spectrum_add(nfreq,sumfreq.data(), tmpfreq1.data(), pars.octave.o1);
	};
	if (pars.octave.o15>1e-3) {
		spectrum_do_pitch_shift(pars,nfreq, freq1, tmpfreq1.data(), 3.0);
		spectrum_add(nfreq,sumfreq.data(), tmpfreq1.data(), pars.octave.o15);
	};
	if (pars.octave.o2>1e-3) {
		spectrum_do_pitch_shift(pars, nfreq, freq1, tmpfreq1.data(), 4.0);
		spectrum_add(nfreq,sumfreq.data(), tmpfreq1.data(), pars.octave.o2);
	};

	REALTYPE sum = 0.01f + pars.octave.om2 + pars.octave.om1 + pars.octave.o0 + pars.octave.o1 + pars.octave.o15 + pars.octave.o2;
	if (sum<0.5f) sum = 0.5f;
	for (int i = 0; i<nfreq; i++) freq2[i] = sumfreq[i] / sum;
};

inline void spectrum_do_filter(const ProcessParameters& pars, int nfreq, double samplerate, REALTYPE *freq1, REALTYPE *freq2) {
	REALTYPE low = 0, high = 0;
	if (pars.filter.low<pars.filter.high) {//sort the low/high freqs
		low = pars.filter.low;
		high = pars.filter.high;
	}
	else {
		high = pars.filter.low;
		low = pars.filter.high;
	};
	int ilow = (int)(low / samplerate * nfreq*2.0f);
	int ihigh = (int)(high / samplerate * nfreq*2.0f);
	REALTYPE dmp = 1.0;
	REALTYPE dmprap = 1.0f - pow(pars.filter.hdamp*0.5f, 4.0f);
	for (int i = 0; i<nfreq; i++) {
		REALTYPE a = 0.0f;
		if ((i >= ilow) && (i<ihigh)) a = 1.0f;
		if (pars.filter.stop) a = 1.0f - a;
		freq2[i] = freq1[i] * a*dmp;
		dmp *= dmprap + 1e-8f;
	};
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
    

    //void copy(const realvector& freq1,realvector& freq2);
    void copy(REALTYPE* freq1, REALTYPE* freq2);
    void add(REALTYPE *freq2,REALTYPE *freq1,REALTYPE a=1.0);
    void mul(REALTYPE *freq1,REALTYPE a);
    void zero(REALTYPE *freq1);
    

    void update_free_filter();
    int nfreq=0;

    std::vector<REALTYPE> free_filter_freqs;
    ProcessParameters pars;
    
    std::vector<REALTYPE> infreq,sumfreq,tmpfreq1,tmpfreq2;
    
		//REALTYPE *fbfreq;
};
