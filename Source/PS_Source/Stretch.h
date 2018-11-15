/*
  Copyright (C) 2006-2011 Nasca Octavian Paul
  Author: Nasca Octavian Paul

  Author/Copyright (C) 2017 Xenakios
 
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

#include "globals.h"

#include "fftw3.h"


#include "../JuceLibraryCode/JuceHeader.h"
#include <random>
#include <type_traits>

template<typename T>
class FFTWBuffer
{
public:
    FFTWBuffer()
    {
        static_assert(std::is_floating_point<T>::value,"FFTWBuffer only works with floating point types");
    }
    ~FFTWBuffer()
    {
        freeimpl(m_buf);
    }
    void resize(int size, bool clear)
    {
        // come on, zero size doesn't make sense!
        jassert(size>0);
        if (size==m_size && clear==false)
            return;
        if (m_buf)
            freeimpl(m_buf);
        mallocimpl(m_buf,size);
        
        if (clear)
            for (int i=0;i<size;++i)
                m_buf[i]=T();
        m_size = size;
    }
	
	T& operator[](int index) 
	{ 
		jassert(index >= 0 && index < m_size); 
		return m_buf[index]; 
	}
	const T& operator[](int index) const 
	{ 
		jassert(index >= 0 && index < m_size); 
		return m_buf[index];
	}
	
    T* data()
    {
        // callers to this will likely just blow themselves up if they get a nullptr back
        jassert(m_buf!=nullptr);
        return m_buf;
    }
    int getSize() { return m_size; }
    FFTWBuffer(FFTWBuffer&& other) : m_buf(other.m_buf), m_size(other.m_size) 
	{
		other.m_buf = nullptr;
		other.m_size = 0;
	}
	FFTWBuffer& operator = (FFTWBuffer&& other)
	{
		std::swap(other.m_buf, m_buf);
		std::swap(other.m_size, m_size);
		return *this;
	}
	// These buffers probably shouldn't be copied anywhere, so just disallow that for now
	FFTWBuffer(const FFTWBuffer&) = delete;
	FFTWBuffer& operator = (const FFTWBuffer&) = delete;
private:
    T* m_buf = nullptr;
    int m_size = 0;
    void mallocimpl(T*& buf,int size)
    {
        if constexpr (std::is_same<T,float>::value)
			buf = (float*)fftwf_malloc(size*sizeof(float));
		else
			buf = (double*)fftw_malloc(size * sizeof(double));
    }
	void freeimpl(T*& buf)
    {
        if (buf!=nullptr)
        {
			if constexpr (std::is_same<T, float>::value)
				fftwf_free(buf);
			else
				fftw_free(buf);
			buf = nullptr;
        }
    }
};

enum FFTWindow{W_RECTANGULAR,W_HAMMING,W_HANN,W_BLACKMAN,W_BLACKMAN_HARRIS};

class FFT
{//FFT class that considers phases as random
	public:
		FFT(int nsamples_, bool no_inverse=false);//samples must be even
		~FFT();
		void smp2freq();//input is smp, output is freq (phases are discarded)
		void freq2smp();//input is freq,output is smp (phases are random)
		void applywindow(FFTWindow type);
		std::vector<REALTYPE> smp;//size of samples/2
		std::vector<REALTYPE> freq;//size of samples
		std::vector<unsigned int> phases;
		
		int nsamples=0;

		void setPhaseRefreshRate(int rate);

	private:

		fftwf_plan planfftw,planifftw;
        FFTWBuffer<REALTYPE> data;
		int m_phaserefreshcounter = 0;
		int m_phaserefreshrate = 1;
		void updatePhases();
		struct{
			std::vector<REALTYPE> data;
			FFTWindow type;
		}window;

        std::mt19937 m_randgen;
        std::uniform_int_distribution<unsigned int> m_randdist{0,32767};
    
		
};

class Stretch
{
	public:
		Stretch(REALTYPE rap_,int in_bufsize_,FFTWindow w=W_HAMMING,bool bypass_=false,REALTYPE samplerate_=44100,int stereo_mode_=0);
		//in_bufsize is also a half of a FFT buffer (in samples)
		virtual ~Stretch();

		int get_max_bufsize(){
			return bufsize*3;
		};
		int get_bufsize(){
			return bufsize;
		};
		virtual void setBufferSize(int sz);
		REALTYPE get_onset_detection_sensitivity(){
			return onset_detection_sensitivity;
		};

		REALTYPE process(REALTYPE *smps,int nsmps);//returns the onset value
		void set_freezing(bool new_freezing){
			freezing=new_freezing;
		};
		bool isFreezing() { return freezing; }
		void setPhaseRefreshRate(int rate);
		std::vector<REALTYPE> out_buf;//pot sa pun o variabila "max_out_bufsize" si asta sa fie marimea lui out_buf si pe out_bufsize sa il folosesc ca marime adaptiva

		int get_nsamples(REALTYPE current_pos_percents);//how many samples are required 
		int get_nsamples_for_fill();//how many samples are required to be added for a complete buffer refill (at start of the song or after seek)
		int get_skip_nsamples();//used for shorten

		void set_rap(REALTYPE newrap);//set the current stretch value

		void set_onset_detection_sensitivity(REALTYPE detection_sensitivity);;
		void here_is_onset(REALTYPE onset);
		virtual void setSampleRate(REALTYPE sr) { samplerate = jlimit(1000.0f, 384000.0f, sr); }
		REALTYPE getSampleRate() { return samplerate; }
		FFTWindow window_type;
	protected:
		int bufsize=0;

		virtual void process_spectrum(REALTYPE *){};
		virtual REALTYPE get_stretch_multiplier(REALTYPE pos_percents);
		REALTYPE samplerate=0.0f;
		
	private:

		void do_analyse_inbuf(REALTYPE *smps);
		void do_next_inbuf_smps(REALTYPE *smps);
		REALTYPE do_detect_onset();

//		REALTYPE *in_pool;//de marimea in_bufsize
		REALTYPE rap,onset_detection_sensitivity;
		std::vector<REALTYPE> old_out_smps;
		std::vector<REALTYPE> old_freq;
		std::vector<REALTYPE> new_smps,old_smps,very_old_smps;

		std::unique_ptr<FFT> infft,outfft;
		std::unique_ptr<FFT> fft;
		long double remained_samples;//0..1
		long double extra_onset_time_credit;
		REALTYPE c_pos_percents;
		int skip_samples;
		bool require_new_buffer;
		bool bypass,freezing;
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Stretch)
};



