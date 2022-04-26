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

#if PS_USE_VDSP_FFT
#define VIMAGE_H // crazy hack needed
#include <Accelerate/Accelerate.h>
//#include <vForce.h>
#endif

#include "Stretch.h"
#include <stdlib.h>
#include <math.h>


FFT::FFT(int nsamples_, bool no_inverse)
{
    nsamples=nsamples_;
	if (nsamples%2!=0) {
		nsamples+=1;
		Logger::writeToLog("WARNING: Odd sample size on FFT::FFT() "+String(nsamples));
	};
	smp.resize(nsamples);
	for (int i = 0; i < nsamples; i++) 
		smp[i] = 0.0;
	freq.resize(nsamples/2+1);
	for (int i=0;i<nsamples/2+1;i++) 
		freq[i]=0.0;
	window.data.resize(nsamples);
	for (int i=0;i<nsamples;i++) 
		window.data[i]=0.707f;
	window.type=W_RECTANGULAR;

	data.resize(nsamples,true);
	bool allow_long_planning = false; // g_propsfile->getBoolValue("fftw_allow_long_planning", false);
    //double t0 = Time::getMillisecondCounterHiRes();
#if PS_USE_VDSP_FFT
    int maxlog2N = 1;
    while ((1 << maxlog2N) < nsamples)
        ++maxlog2N;
    log2N = maxlog2N;
    planfft = vDSP_create_fftsetup(maxlog2N, kFFTRadix2);

    m_workReal.resize(nsamples,false);
    m_workImag.resize(nsamples,false);

    //Logger::writeToLog("fftsize: "  + String(nsamples) + " log2N: " + String(log2N));
#elif PS_USE_PFFFT

    planpffft = pffft_new_setup(nsamples, PFFFT_REAL);

    m_work.resize(2*nsamples,false);

    //Logger::writeToLog("fftsize: "  + String(nsamples) + " log2N: " + String(log2N));

#else
    if (allow_long_planning)
    {
        //fftwf_plan_with_nthreads(2);
        planfftw=fftwf_plan_r2r_1d(nsamples,data.data(),data.data(),FFTW_R2HC,FFTW_MEASURE);
        if (no_inverse == false)
			planifftw=fftwf_plan_r2r_1d(nsamples,data.data(),data.data(),FFTW_HC2R,FFTW_MEASURE);
    } else
    {
        //fftwf_plan_with_nthreads(2);
        planfftw=fftwf_plan_r2r_1d(nsamples,data.data(),data.data(),FFTW_R2HC,FFTW_ESTIMATE);
        //fftwf_plan_with_nthreads(2);
        if (no_inverse == false)
			planifftw=fftwf_plan_r2r_1d(nsamples,data.data(),data.data(),FFTW_HC2R,FFTW_ESTIMATE);
    }
#endif

    //double t1 = Time::getMillisecondCounterHiRes();
    //Logger::writeToLog("Creating FFTW3 plans took "+String(t1-t0)+ "ms");
	static int seed = 0;
	m_randgen = std::mt19937(seed);
	++seed;
};

FFT::~FFT()
{
#if PS_USE_VDSP_FFT
    vDSP_destroy_fftsetup((FFTSetup)planfft);
#elif PS_USE_PFFFT
    if (planpffft) {
        pffft_destroy_setup(planpffft);
    }
#else
    fftwf_destroy_plan(planfftw);
	if (planifftw!=nullptr)
		fftwf_destroy_plan(planifftw);
#endif
};

void FFT::smp2freq()
{
#if PS_USE_VDSP_FFT

    const int halfsamples = nsamples / 2;

    COMPLEX_SPLIT A;
    A.realp = m_workReal.data();
    A.imagp = m_workImag.data();

    // apply window
    //vDSP_vmul(gInFIFO, 1, mWindow, 1, gFFTworksp, 1, fftFrameSize);

    //convert real input to even-odd

    vDSP_ctoz((COMPLEX*)smp.data(), 2, &A, 1, halfsamples);
    //memset(ioData->mBuffers[0].mData, 0, ioData->mBuffers[0].mDataByteSize);

    // forward fft
    vDSP_fft_zrip((FFTSetup)planfft, &A, 1, log2N, FFT_FORWARD);

    // result is in split packed complex A.realp[0] is DC, A.imagp[0] is NY, so we zero NY before doing mag^2
    A.imagp[0] = 0.0f;

    //DebugLogC("post fft: %g %g\n", A.realp[fftFrameSize/4], A.imagp[fftFrameSize/4 + 1]);

    // forward scale
    const float scale = 0.5f;
    vDSP_vsmul(A.realp, 1, &scale, A.realp, 1, halfsamples);
    vDSP_vsmul(A.imagp, 1, &scale, A.imagp, 1, halfsamples);

    // Absolute square (equivalent to mag^2)
    vDSP_zvmags(&A, 1, freq.data(), 1, halfsamples);

    // take square root
    //vvsqrtf(freq.data(), freq.data(), &halfsamples);

    for (int i=1; i < halfsamples;i++)
    {
        freq[i]=sqrt(freq[i]);
    }

    freq[0] = 0.0;

#elif PS_USE_PFFFT
    const int halfsamples = nsamples / 2;
    auto * databuf = data.data();

    pffft_transform_ordered(planpffft, smp.data(), databuf, m_work.data(), PFFFT_FORWARD);

    data[1] = 0.0f;

    // compute magnitude

    FloatVectorOperations::multiply(databuf, databuf, nsamples);
    
    for (int k=1, l=2; k < halfsamples; ++k, l+=2) {
        freq[k] = sqrt(databuf[l] + databuf[l+1]);
    }

    freq[0] = 0.0;

#else

    for (int i=0;i<nsamples;i++)
        data[i]=smp[i];
    fftwf_execute(planfftw);


    for (int i=1;i<nsamples/2;i++)
    {

        REALTYPE c=data[i];
        REALTYPE s=data[nsamples-i];

        freq[i]=sqrt(c*c+s*s);
    };
    freq[0]=0.0;

#endif
};

void FFT::freq2smp()
{
	const REALTYPE inv_2p15_2pi=1.0f/16384.0f*(float)c_PI;

#if PS_USE_VDSP_FFT
    const int halfsamples = nsamples / 2;

    COMPLEX_SPLIT A;
    A.realp = m_workReal.data();
    A.imagp = m_workImag.data();


    for (int i=1;i<nsamples/2;i++)
    {
        unsigned int rand = m_randdist(m_randgen);
        REALTYPE phase = rand*inv_2p15_2pi;
        m_workReal[i] = freq[i]*cos(phase);
        m_workImag[i] = freq[i]*sin(phase);

    };
    m_workReal[0] = m_workImag[0] = 0.0;

    // inverse fft
    vDSP_fft_zrip((FFTSetup)planfft, &A, 1, log2N, FFT_INVERSE);

    // unpack
    vDSP_ztoc(&A, 1, (COMPLEX*)data.data(), 2, halfsamples);

    // scale
    float scale = 1.f / nsamples;
    vDSP_vsmul(data.data(), 1, &scale, smp.data(), 1, nsamples);


#elif PS_USE_PFFFT
    const int halfsamples = nsamples / 2;
    auto * databuf = data.data();

    for (int i=1; i < halfsamples; ++i)
    {
        unsigned int rand = m_randdist(m_randgen);
        REALTYPE phase=rand*inv_2p15_2pi;
        data[i*2] = freq[i]*cos(phase);
        data[i*2+1] = freq[i]*sin(phase);

    };
    data[0] = data[1] = 0.0;

    pffft_transform_ordered(planpffft, databuf, smp.data(), m_work.data(), PFFFT_BACKWARD);

    // post scale
    float scale = 1.f / nsamples;
    FloatVectorOperations::multiply(smp.data(), scale, nsamples);

#else


    for (int i=1;i<nsamples/2;i++)
    {
        unsigned int rand = m_randdist(m_randgen);
        REALTYPE phase=rand*inv_2p15_2pi;
        data[i]=freq[i]*cos(phase);
        data[nsamples-i]=freq[i]*sin(phase);

    };
    data[0]=data[nsamples/2+1]=data[nsamples/2]=0.0;

    fftwf_execute(planifftw);

    // post scale
    float scale = 1.f / nsamples;
    FloatVectorOperations::multiply(smp.data(), data.data(), scale, nsamples);

#endif
};

void FFT::applywindow(FFTWindow type)
{
	if (window.type!=type){
		window.type=type;
		switch (type){
			case W_RECTANGULAR:
				for (int i=0;i<nsamples;i++) window.data[i]=0.707f;
				break;
			case W_HAMMING:
				for (int i=0;i<nsamples;i++) window.data[i]=(float)(0.53836-0.46164*cos(2.0*c_PI*i/(nsamples+1.0)));
				break;
			case W_HANN:
				for (int i=0;i<nsamples;i++) window.data[i]=(float)(0.5*(1.0-cos(2*c_PI*i/(nsamples-1.0))));
				break;
			case W_BLACKMAN:
				for (int i=0;i<nsamples;i++) window.data[i]=(float)(0.42-0.5*cos(2*c_PI*i/(nsamples-1.0))+0.08*cos(4*c_PI*i/(nsamples-1.0)));
				break;
			case W_BLACKMAN_HARRIS:
				for (int i=0;i<nsamples;i++) window.data[i]=(float)(0.35875-0.48829*cos(2*c_PI*i/(nsamples-1.0))+0.14128*cos(4*c_PI*i/(nsamples-1.0))-0.01168*cos(6*c_PI*i/(nsamples-1.0)));
				break;

		};
	};

    //for (int i=0;i<nsamples;i++) smp[i]*=window.data[i];
    FloatVectorOperations::multiply(smp.data(), window.data.data(), nsamples);
}

Stretch::Stretch(REALTYPE rap_,int /*bufsize_*/,FFTWindow w,bool bypass_,REALTYPE samplerate_,int /*stereo_mode_*/)
{
	freezing=false;
	onset_detection_sensitivity=0.0;

	samplerate=samplerate_;
	rap=rap_;
	bypass = bypass_;
	
	remained_samples=0.0;
	window_type=w;
	require_new_buffer=false;
	c_pos_percents=0.0;
	extra_onset_time_credit=0.0;
	skip_samples=0;
};

Stretch::~Stretch()
{
};

void Stretch::set_rap(REALTYPE newrap){
	//if ((rap>=1.0)&&(newrap>=1.0)) 
	rap=newrap;
};
		
void Stretch::do_analyse_inbuf(REALTYPE *smps){
	//get the frequencies

    FloatVectorOperations::copy(infft->smp.data(), old_smps.data(), bufsize);
    FloatVectorOperations::copy(infft->smp.data()+bufsize, smps, bufsize);
    FloatVectorOperations::copy(old_freq.data(), infft->freq.data(), bufsize);
    /*
    for (int i=0;i<bufsize;i++) {
		infft->smp[i]=old_smps[i];
		infft->smp[i+bufsize]=smps[i];

		old_freq[i]=infft->freq[i];
	};
     */

	infft->applywindow(window_type);
	infft->smp2freq();
};

void Stretch::do_next_inbuf_smps(REALTYPE *smps){
    FloatVectorOperations::copy(very_old_smps.data(), old_smps.data(), bufsize);
    FloatVectorOperations::copy(old_smps.data(), new_smps.data(), bufsize);
    FloatVectorOperations::copy(new_smps.data(), smps, bufsize);

    /*
    for (int i=0;i<bufsize;i++) {
		very_old_smps[i]=old_smps[i];
		old_smps[i]=new_smps[i];
		new_smps[i]=smps[i];
	};
     */
};

REALTYPE Stretch::do_detect_onset(){
	REALTYPE result=0.0;
	if (onset_detection_sensitivity>1e-3){
		REALTYPE os=0.0,osinc=0.0;
		REALTYPE osincold=1e-5f;
		int maxk=1+(int)(bufsize*500.0/(samplerate*0.5));
		int k=0;
		for (int i=0;i<bufsize;i++) {
			osinc+=infft->freq[i]-old_freq[i];
			osincold+=old_freq[i];
			if (k>=maxk) {
				k=0;
				os+=osinc/osincold;
				osinc=0;
			};
			k++;
		};
		os+=osinc;
		if (os<0.0) os=0.0;
		//if (os>1.0) os=1.0;

		REALTYPE os_strength=(float)(pow(20.0,1.0-onset_detection_sensitivity)-1.0);
		REALTYPE os_strength_h=os_strength*0.75f;
		if (os>os_strength_h){
			result=(os-os_strength_h)/(os_strength-os_strength_h);
			if (result>1.0f) result=1.0f;
		};

		if (result>1.0f) result=1.0f;
	};
	return result;
};

void Stretch::setBufferSize(int bufsize_)
{
	if (bufsize == 0 || bufsize_ != bufsize)
	{
		bufsize = bufsize_;

		if (bufsize < 8) bufsize = 8;

		out_buf = floatvector(bufsize);
		old_freq = floatvector(bufsize);

		very_old_smps = floatvector(bufsize);
		new_smps = floatvector(bufsize);
		old_smps = floatvector(bufsize);

		old_out_smps = floatvector(bufsize * 2);
		infft = std::make_unique<FFT>(bufsize * 2);
		fft = std::make_unique<FFT>(bufsize * 2);
		outfft = std::make_unique<FFT>(bufsize * 2);
	}
	jassert(infft != nullptr && fft != nullptr && outfft != nullptr);
	fill_container(outfft->smp, 0.0f);
	for (int i = 0; i<bufsize * 2; i++) {
		old_out_smps[i] = 0.0;
	};
	for (int i = 0; i<bufsize; i++) {
		old_freq[i] = 0.0f;
		new_smps[i] = 0.0f;
		old_smps[i] = 0.0f;
		very_old_smps[i] = 0.0f;
	};
}

REALTYPE Stretch::process(REALTYPE *smps,int nsmps)
{
	jassert(bufsize > 0);
	REALTYPE onset=0.0;
	if (bypass){
        //for (int i=0;i<bufsize;i++) out_buf[i]=smps[i];
        FloatVectorOperations::copy(out_buf.data(), smps, bufsize);
		return 0.0;
	};

	if (smps!=NULL){
		if ((nsmps!=0)&&(nsmps!=bufsize)&&(nsmps!=get_max_bufsize())){
			printf("Warning wrong nsmps on Stretch::process() %d,%d\n",nsmps,bufsize);
			return 0.0;
		};
		if (nsmps!=0){//new data arrived: update the frequency components
			do_analyse_inbuf(smps);		
			if (nsmps==get_max_bufsize()) {
				for (int k=bufsize;k<get_max_bufsize();k+=bufsize) do_analyse_inbuf(smps+k);
			};
			if (onset_detection_sensitivity>1e-3) onset=do_detect_onset();
		};


		//move the buffers	
		if (nsmps!=0){//new data arrived: update the frequency components
			do_next_inbuf_smps(smps);		
			if (nsmps==get_max_bufsize()) {
				for (int k=bufsize;k<get_max_bufsize();k+=bufsize) do_next_inbuf_smps(smps+k);

			};
		};
	
		//construct the input fft
		int start_pos=(int)(floor(remained_samples*bufsize));	
		if (start_pos>=bufsize) start_pos=bufsize-1;

        FloatVectorOperations::copy(fft->smp.data(), very_old_smps.data() + start_pos, bufsize-start_pos);
        FloatVectorOperations::copy(fft->smp.data() + (bufsize - start_pos), old_smps.data() , bufsize);
        FloatVectorOperations::copy(fft->smp.data() + (2*bufsize - start_pos), new_smps.data() , start_pos);

        /*
		for (int i=0;i<bufsize-start_pos;i++) fft->smp[i]=very_old_smps[i+start_pos];
		for (int i=0;i<bufsize;i++) fft->smp[i+bufsize-start_pos]=old_smps[i];
		for (int i=0;i<start_pos;i++) fft->smp[i+2*bufsize-start_pos]=new_smps[i];
         */

		//compute the output spectrum
		fft->applywindow(window_type);
		fft->smp2freq();

        //for (int i=0;i<bufsize;i++) outfft->freq[i]=fft->freq[i];
        FloatVectorOperations::copy(outfft->freq.data(), fft->freq.data(), bufsize);



		//for (int i=0;i<bufsize;i++) outfft->freq[i]=infft->freq[i]*remained_samples+old_freq[i]*(1.0-remained_samples);


		process_spectrum(outfft->freq.data());

		outfft->freq2smp();

		//make the output buffer
		REALTYPE tmp=(float)(1.0/(float) bufsize*c_PI);
		REALTYPE hinv_sqrt2=0.853553390593f;//(1.0+1.0/sqrt(2))*0.5;

		REALTYPE ampfactor=2.0f;

		//remove the resulted unwanted amplitude modulation (caused by the interference of N and N+1 windowed buffer and compute the output buffer
		for (int i=0;i<bufsize;i++) {
			REALTYPE a=(float)((0.5+0.5*cos(i*tmp)));
			REALTYPE out=(float)(outfft->smp[i+bufsize]*(1.0-a)+old_out_smps[i]*a);
			out_buf[i]=(float)(out*(hinv_sqrt2-(1.0-hinv_sqrt2)*cos(i*2.0*tmp))*ampfactor);
		};

		//copy the current output buffer to old buffer
        //for (int i=0;i<bufsize*2;i++) old_out_smps[i]=outfft->smp[i];
        FloatVectorOperations::copy(old_out_smps.data(), outfft->smp.data(), 2*bufsize);

	};

	if (!freezing){
		long double used_rap=rap*get_stretch_multiplier(c_pos_percents);	

		long double r=1.0/used_rap;
		if (extra_onset_time_credit>0){
			REALTYPE credit_get=(float)(0.5*r);//must be smaller than r
			extra_onset_time_credit-=credit_get;
			if (extra_onset_time_credit<0.0) extra_onset_time_credit=0.0;
			r-=credit_get;
		};

		//long double old_remained_samples_test=remained_samples;
		remained_samples+=r;
		//int result=0;
		if (remained_samples>=1.0){
			skip_samples=(int)(floor(remained_samples-1.0)*bufsize);
			remained_samples=remained_samples-floor(remained_samples);
			require_new_buffer=true;
		}else{
			require_new_buffer=false;
		};
	};
//	long double rf_test=remained_samples-old_remained_samples_test;//this value should be almost like "rf" (for most of the time with the exception of changing the "ri" value) for extremely long stretches (otherwise the shown stretch value is not accurate)
	//for stretch up to 10^18x "long double" must have at least 64 bits in the fraction part (true for gcc compiler on x86 and macosx)
	return onset;	
};

void Stretch::set_onset_detection_sensitivity(REALTYPE detection_sensitivity) 
{
	onset_detection_sensitivity = detection_sensitivity;
	if (detection_sensitivity<1e-3) extra_onset_time_credit = 0.0;
}

void Stretch::here_is_onset(REALTYPE onset){
	if (freezing) return;
	if (onset>0.5){
		require_new_buffer=true;
		extra_onset_time_credit+=1.0-remained_samples;
		remained_samples=0.0;
		skip_samples=0;
	};
};

int Stretch::get_nsamples(REALTYPE current_pos_percents){
	if (bypass) return bufsize;
	if (freezing) return 0;
	c_pos_percents=current_pos_percents;
	return require_new_buffer?bufsize:0;
};

int Stretch::get_nsamples_for_fill(){
	return get_max_bufsize();
};

int Stretch::get_skip_nsamples(){
	if (freezing||bypass) return 0;
	return skip_samples;
};

REALTYPE Stretch::get_stretch_multiplier(REALTYPE /*pos_percents*/){
	return 1.0;
};

