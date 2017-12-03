/*
  Copyright (C) 2009 Nasca Octavian Paul
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

#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include "ProcessedStretch.h"

ProcessedStretch::ProcessedStretch(REALTYPE rap_,int in_bufsize_,FFTWindow w,bool bypass_,REALTYPE samplerate_,int stereo_mode_)
	: Stretch(rap_,in_bufsize_,w,bypass_,samplerate_,stereo_mode_)
{
	
    
};

ProcessedStretch::~ProcessedStretch()
{
	
//	delete [] fbfreq;
};

void ProcessedStretch::set_parameters(ProcessParameters *ppar)
{
	pars=*ppar;
	//update_free_filter();
}
void ProcessedStretch::setBufferSize(int sz)
{
	Stretch::setBufferSize(sz);
	nfreq = bufsize;
	infreq = floatvector(nfreq);
	sumfreq = floatvector(nfreq);
	tmpfreq1 = floatvector(nfreq);
	tmpfreq2 = floatvector(nfreq);
	//fbfreq=new REALTYPE[nfreq];
	free_filter_freqs = floatvector(nfreq);
	for (int i = 0; i<nfreq; i++) {
		free_filter_freqs[i] = 1.0;
		//	fbfreq[i]=0.0;
	};
}


/*
void ProcessedStretch::copy(const realvector& freq1,realvector& freq2)
{
	for (int i=0;i<nfreq;i++) freq2[i]=freq1[i];
};
*/

void ProcessedStretch::copy(REALTYPE* freq1, REALTYPE* freq2)
{
	for (int i = 0; i<nfreq; i++) freq2[i] = freq1[i];
};

void ProcessedStretch::add(REALTYPE *freq2,REALTYPE *freq1,REALTYPE a){
	for (int i=0;i<nfreq;i++) freq2[i]+=freq1[i]*a;
};

void ProcessedStretch::mul(REALTYPE *freq1,REALTYPE a){
	for (int i=0;i<nfreq;i++) freq1[i]*=a;
};

void ProcessedStretch::zero(REALTYPE *freq1){
	for (int i=0;i<nfreq;i++) freq1[i]=0.0;
};

REALTYPE ProcessedStretch::get_stretch_multiplier(REALTYPE pos_percents){
	REALTYPE result=1.0;
	/*
	if (pars.stretch_multiplier.get_enabled()){
		result*=pars.stretch_multiplier.get_value(pos_percents);
	};
	*/
	///REALTYPE transient=pars.get_transient(pos_percents);
	///printf("\n%g\n",transient);
	///REALTYPE threshold=0.05;
	///REALTYPE power=1000.0;
	///transient-=threshold;
	///if (transient>0){
	///	transient*=power*(1.0+power);
	///	result/=(1.0+transient);
	///};
	///printf("tr=%g\n",result);
	return result;
};

void ProcessedStretch::process_spectrum(REALTYPE *freq)
{
	for (auto& e : m_spectrum_processes)
    {
		spectrum_copy(nfreq, freq, infreq.data());
		if (e == 0 && pars.harmonics.enabled)
			spectrum_do_harmonics(pars, tmpfreq1, nfreq, samplerate, infreq.data(), freq);
		if (e == 1 && pars.tonal_vs_noise.enabled)
			do_tonal_vs_noise(infreq.data(), freq);
		if (e == 2 && pars.freq_shift.enabled)
			do_freq_shift(infreq.data(), freq);
		if (e == 3 && pars.pitch_shift.enabled)
			do_pitch_shift(infreq.data(), freq, pow(2.0f, pars.pitch_shift.cents / 1200.0f));
		if (e == 4 && pars.octave.enabled)
			do_octave(infreq.data(), freq);
		if (e == 5 && pars.spread.enabled)
			do_spread(infreq.data(), freq);
		if (e == 6 && pars.filter.enabled)
			do_filter(infreq.data(), freq);
		if (e == 7 && pars.compressor.enabled)
			do_compressor(infreq.data(), freq);
	}

#ifdef USE_OLD_SPEC_PROC
    if (pars.harmonics.enabled) {
		copy(freq,infreq.data());
		do_harmonics(infreq.data(),freq);
	};

	if (pars.tonal_vs_noise.enabled){
		copy(freq,infreq.data());
		do_tonal_vs_noise(infreq.data(),freq);
	};

	if (pars.freq_shift.enabled) {
		copy(freq,infreq.data());
		do_freq_shift(infreq.data(),freq);
	};
	if (pars.pitch_shift.enabled) {
		copy(freq,infreq.data());
		do_pitch_shift(infreq.data(),freq,pow(2.0,pars.pitch_shift.cents/1200.0));
	};
	if (pars.octave.enabled){
		copy(freq,infreq.data());
		do_octave(infreq.data(),freq);
	};


	if (pars.spread.enabled){
		copy(freq,infreq.data());
		do_spread(infreq.data(),freq);
	};


	if (pars.filter.enabled){
		copy(freq,infreq.data());
		do_filter(infreq.data(),freq);
	};
	
	if (pars.free_filter.get_enabled()){
		copy(freq,infreq.data());
		do_free_filter(infreq.data(),freq);
	};

	if (pars.compressor.enabled){
		copy(freq,infreq.data());
		do_compressor(infreq.data(),freq);
	};
#endif
};

//void ProcessedStretch::process_output(REALTYPE *smps,int nsmps){
//};

void ProcessedStretch::do_harmonics(REALTYPE *freq1,REALTYPE *freq2){
	REALTYPE freq=pars.harmonics.freq;
	REALTYPE bandwidth=pars.harmonics.bandwidth;
	int nharmonics=pars.harmonics.nharmonics;

	if (freq<10.0) freq=10.0;

	REALTYPE *amp=tmpfreq1.data();
	for (int i=0;i<nfreq;i++) amp[i]=0.0;

	for (int nh=1;nh<=nharmonics;nh++){//for each harmonic
		REALTYPE bw_Hz;//bandwidth of the current harmonic measured in Hz
		REALTYPE bwi;
		REALTYPE fi;
		REALTYPE f=nh*freq;

		if (f>=samplerate/2) break;

		bw_Hz=(pow(2.0f,bandwidth/1200.0f)-1.0f)*f;
		bwi=bw_Hz/(2.0f*samplerate);
		fi=f/samplerate;

		REALTYPE sum=0.0f;
		REALTYPE max=0.0f;
		for (int i=1;i<nfreq;i++){//todo: optimize here
			REALTYPE hprofile;
			hprofile=profile((i/(REALTYPE)nfreq*0.5f)-fi,bwi);
			amp[i]+=hprofile;
			if (max<hprofile) max=hprofile;
			sum+=hprofile;
		};
	};

	REALTYPE max=0.0;
	for (int i=1;i<nfreq;i++){
		if (amp[i]>max) max=amp[i];
	};
	if (max<1e-8f) max=1e-8f;

	for (int i=1;i<nfreq;i++){
		//REALTYPE c,s;
		REALTYPE a=amp[i]/max;
		if (!pars.harmonics.gauss) a=(a<0.368f?0.0f:1.0f);
		freq2[i]=freq1[i]*a;
	};

};


void ProcessedStretch::do_freq_shift(REALTYPE *freq1,REALTYPE *freq2){
	zero(freq2);
	int ifreq=(int)(pars.freq_shift.Hz/(samplerate*0.5)*nfreq);
	for (int i=0;i<nfreq;i++){
		int i2=ifreq+i;
		if ((i2>0)&&(i2<nfreq)) freq2[i2]=freq1[i];
	};
};
void ProcessedStretch::do_pitch_shift(REALTYPE *freq1,REALTYPE *freq2,REALTYPE _rap){
	zero(freq2);
	if (_rap<1.0){//down
		for (int i=0;i<nfreq;i++){
			int i2=(int)(i*_rap);
			if (i2>=nfreq) break;
			freq2[i2]+=freq1[i];
		};
	};
	if (_rap>=1.0){//up
		_rap=1.0f/_rap;
		for (int i=0;i<nfreq;i++){
			freq2[i]=freq1[(int)(i*_rap)];
		};

	};
};
void ProcessedStretch::do_octave(REALTYPE *freq1,REALTYPE *freq2){
	zero(sumfreq.data());
	if (pars.octave.om2>1e-3){
		do_pitch_shift(freq1,tmpfreq1.data(),0.25);
		add(sumfreq.data(),tmpfreq1.data(),pars.octave.om2);
	};
	if (pars.octave.om1>1e-3){
		do_pitch_shift(freq1,tmpfreq1.data(),0.5);
		add(sumfreq.data(),tmpfreq1.data(),pars.octave.om1);
	};
	if (pars.octave.o0>1e-3){
		add(sumfreq.data(),freq1,pars.octave.o0);
	};
	if (pars.octave.o1>1e-3){
		do_pitch_shift(freq1,tmpfreq1.data(),2.0);
		add(sumfreq.data(),tmpfreq1.data(),pars.octave.o1);
	};
	if (pars.octave.o15>1e-3){
		do_pitch_shift(freq1,tmpfreq1.data(),3.0);
		add(sumfreq.data(),tmpfreq1.data(),pars.octave.o15);
	};
	if (pars.octave.o2>1e-3){
		do_pitch_shift(freq1,tmpfreq1.data(),4.0);
		add(sumfreq.data(),tmpfreq1.data(),pars.octave.o2);
	};

	REALTYPE sum=0.01f+pars.octave.om2+pars.octave.om1+pars.octave.o0+pars.octave.o1+pars.octave.o15+pars.octave.o2;
	if (sum<0.5f) sum=0.5f;
	for (int i=0;i<nfreq;i++) freq2[i]=sumfreq[i]/sum;    
};

void ProcessedStretch::do_filter(REALTYPE *freq1,REALTYPE *freq2){
	REALTYPE low=0,high=0;
	if (pars.filter.low<pars.filter.high){//sort the low/high freqs
		low=pars.filter.low;
		high=pars.filter.high;
	}else{
		high=pars.filter.low;
		low=pars.filter.high;
	};
	int ilow=(int) (low/samplerate*nfreq*2.0f);
	int ihigh=(int) (high/samplerate*nfreq*2.0f);
	REALTYPE dmp=1.0;
	REALTYPE dmprap=1.0f-pow(pars.filter.hdamp*0.5f,4.0f);
	for (int i=0;i<nfreq;i++){
		REALTYPE a=0.0f;
		if ((i>=ilow)&&(i<ihigh)) a=1.0f;
		if (pars.filter.stop) a=1.0f-a;
		freq2[i]=freq1[i]*a*dmp;
		dmp*=dmprap+1e-8f;
	};
};

void ProcessedStretch::update_free_filter()
{
	/*
	pars.free_filter.update_curve();
	if (pars.free_filter.get_enabled()) {
		for (int i=0;i<nfreq;i++){
			REALTYPE freq=(REALTYPE)i/(REALTYPE) nfreq*samplerate*0.5f;
			free_filter_freqs[i]=pars.free_filter.get_value(freq);
		};
	}else{
		for (int i=0;i<nfreq;i++){
			free_filter_freqs[i]=1.0f;
		};
	};
	*/
};

void ProcessedStretch::do_free_filter(REALTYPE *freq1,REALTYPE *freq2){
	for (int i=0;i<nfreq;i++){
		freq2[i]=freq1[i]*free_filter_freqs[i];
	};
};

void ProcessedStretch::do_spread(REALTYPE *freq1,REALTYPE *freq2){
	spread(freq1,freq2,pars.spread.bandwidth);
};

void ProcessedStretch::spread(REALTYPE *freq1,REALTYPE *freq2,REALTYPE spread_bandwidth){
	//convert to log spectrum
	REALTYPE minfreq=20.0f;
	REALTYPE maxfreq=0.5f*samplerate;

	REALTYPE log_minfreq=log(minfreq);
	REALTYPE log_maxfreq=log(maxfreq);
		
	for (int i=0;i<nfreq;i++){
		REALTYPE freqx=i/(REALTYPE) nfreq;
		REALTYPE x=exp(log_minfreq+freqx*(log_maxfreq-log_minfreq))/maxfreq*nfreq;
		REALTYPE y=0.0f;
		int x0=(int)floor(x); if (x0>=nfreq) x0=nfreq-1;
		int x1=x0+1; if (x1>=nfreq) x1=nfreq-1;
		REALTYPE xp=x-x0;
		if (x<nfreq){
			y=freq1[x0]*(1.0f-xp)+freq1[x1]*xp;
		};
		tmpfreq1[i]=y;
	};

	//increase the bandwidth of each harmonic (by smoothing the log spectrum)
	int n=2;
	REALTYPE bandwidth=spread_bandwidth;
	REALTYPE a=1.0f-pow(2.0f,-bandwidth*bandwidth*10.0f);
	a=pow(a,8192.0f/nfreq*n);

	for (int k=0;k<n;k++){                                                  
		tmpfreq1[0]=0.0f;
		for (int i=1;i<nfreq;i++){                                       
			tmpfreq1[i]=tmpfreq1[i-1]*a+tmpfreq1[i]*(1.0f-a);
		};                                                              
		tmpfreq1[nfreq-1]=0.0f;                                               
		for (int i=nfreq-2;i>0;i--){                                     
			tmpfreq1[i]=tmpfreq1[i+1]*a+tmpfreq1[i]*(1.0f-a);                    
		};                                                              
	};                                                                      

	freq2[0]=0;
	REALTYPE log_maxfreq_d_minfreq=log(maxfreq/minfreq);
	for (int i=1;i<nfreq;i++){
		REALTYPE freqx=i/(REALTYPE) nfreq;
		REALTYPE x=log((freqx*maxfreq)/minfreq)/log_maxfreq_d_minfreq*nfreq;
		REALTYPE y=0.0;
		if ((x>0.0)&&(x<nfreq)){
			int x0=(int)floor(x); if (x0>=nfreq) x0=nfreq-1;
			int x1=x0+1; if (x1>=nfreq) x1=nfreq-1;
			REALTYPE xp=x-x0;
			y=tmpfreq1[x0]*(1.0f-xp)+tmpfreq1[x1]*xp;
		};
		freq2[i]=y;
	};


};


void ProcessedStretch::do_compressor(REALTYPE *freq1,REALTYPE *freq2){
	REALTYPE rms=0.0;
	for (int i=0;i<nfreq;i++) rms+=freq1[i]*freq1[i];
	rms=sqrt(rms/nfreq)*0.1f;
	if (rms<1e-3f) rms=1e-3f;

	REALTYPE _rap=pow(rms,-pars.compressor.power);
	for (int i=0;i<nfreq;i++) freq2[i]=freq1[i]*_rap;
};

void ProcessedStretch::do_tonal_vs_noise(REALTYPE *freq1,REALTYPE *freq2){
	spread(freq1,tmpfreq1.data(),pars.tonal_vs_noise.bandwidth);

	if (pars.tonal_vs_noise.preserve>=0.0){
		REALTYPE mul=(pow(10.0f,pars.tonal_vs_noise.preserve)-1.0f);
		for (int i=0;i<nfreq;i++) {
			REALTYPE x=freq1[i];
			REALTYPE smooth_x=tmpfreq1[i]+1e-6f;

			REALTYPE result=0.0f;
			result=x-smooth_x*mul;
			if (result<0.0f) result=0.0f;
			freq2[i]=result;
		};
	}else{
		REALTYPE mul=(pow(5.0f,1.0f+pars.tonal_vs_noise.preserve)-1.0f);
		for (int i=0;i<nfreq;i++) {
			REALTYPE x=freq1[i];
			REALTYPE smooth_x=tmpfreq1[i]+1e-6f;

			REALTYPE result=0.0f;
			result=x-smooth_x*mul+0.1f*mul;
			if (result<0.0f) result=x;
			else result=0.0f;

			freq2[i]=result;
		};
	};

};

std::vector<SpectrumProcess> make_spectrum_processes()
{
	std::vector<SpectrumProcess> m_spectrum_processes;
	m_spectrum_processes.emplace_back("Harmonics",0);
	m_spectrum_processes.emplace_back("Tonal vs Noise",1);
	m_spectrum_processes.emplace_back("Frequency shift",2);
	m_spectrum_processes.emplace_back("Pitch shift",3);
	m_spectrum_processes.emplace_back("Octaves mix",4);
	m_spectrum_processes.emplace_back("Spread",5);
	m_spectrum_processes.emplace_back("Filter",6);
	m_spectrum_processes.emplace_back("Compressor",7);
	return m_spectrum_processes;
}
