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

#include "globals.h"
#include "Input/AInputS.h"
#include "ProcessedStretch.h"
#include "BinauralBeats.h"
#include "StretchSource.h"
#include "../JuceLibraryCode/JuceHeader.h"
#include "../ps3_BufferingAudioSource.h"

class RenderInfo
{
public:
	double m_progress_percent = 0.0;
	double m_elapsed_time = 0.0;
	std::atomic<bool> m_cancel{ false };
	String m_txt;
};

using RenderInfoRef = std::shared_ptr<RenderInfo>;

class RenderParameters
{
public:
	File inaudio;
	File outaudio;
	double pos1 = 0.0;
	double pos2 = 1.0;
	int64_t numLoops = 0;
	int wavformat = 2;
	int sampleRate = 0;
	int numoutchans = 2;
	double minrenderlen = 1.0;
	double maxrenderlen = 1000000.0;
	double voldb = 0.0;
	bool clipFloatOutput = true;
	std::function<void(RenderInfoRef)> completion_callback;
};

class Control
{
public:
	Control(AudioFormatManager* afm);
	~Control();
	void processAudio(AudioBuffer<float>& buf);
	void startplay(bool bypass, bool realtime, Range<double> playrange, int numoutchans, String& err);
	void stopplay();
	void set_seek_pos(REALTYPE x);
	REALTYPE get_seek_pos();
	double getLivePlayPosition();
	bool playing();
	
	void setStretchAmount(double rate);
	void setFFTSize(double size);
	int getFFTSize() { return m_fft_size_to_use; }
	void setOnsetDetection(double x);
	StretchAudioSource* getStretchAudioSource() { return m_stretch_source.get(); }
    void set_input_file(File filename, std::function<void(String)> cb);//return non empty String if the file cannot be opened
	String get_input_filename();
	String get_stretch_info(Range<double> playrange);
	double getPreBufferingPercent();
	double get_stretch()
	{
		return process.stretch;
	};
	double get_onset_detection_sensitivity()
	{
		return process.onset_detection_sensitivity;
	};
		
	void set_stretch_controls(double stretch_s,int mode,double fftsize_s,double onset_detection_sensitivity);//*_s sunt de la 0.0 la 1.0
	double get_stretch_control(double stretch,int mode);
	void update_player_stretch();

	void set_window_type(FFTWindow window);
	///	void pre_analyse_whole_audio(InputS *ai);
	RenderInfoRef Render2(RenderParameters renpars);
	
	ProcessParameters ppar;
	BinauralBeatsParameters	bbpar;
	
	void update_process_parameters();//pt. player
	struct{
		double fftsize_s,stretch_s;
		int mode_s;
	}gui_sliders;	   
	FFTWindow window_type;
    
	void setPreBufferAmount(int x);
	int getPreBufferAmount() { return m_prebuffer_amount; }
	double getPreBufferAmountSeconds();
	bool isResampling() 
	{
		return m_stretch_source->isResampling();
	}
	void setAudioVisualizer(AudioVisualiserComponent* comp);
	void setOutputAudioFileToRecord(File f);
    
		
    void setPrebufferThreadPriority(int v);
    int getPrebufferThreadPriority() { return m_prebufthreadprior; }
private:
	REALTYPE volume;
    int m_prebufthreadprior = 4;
    int get_optimized_updown(int n,bool up);
    int optimizebufsize(int bufsize);
    std::string getfftsizestr(int fftsize);

    struct control_process_t{
        int bufsize;
        double stretch;
        double onset_detection_sensitivity;
    }process;

    struct control_infileinfo_t {
        int samplerate=0;
        int64_t nsamples=0;
        String filename;
        
    }wavinfo;//input
    REALTYPE seek_pos;
	AudioFormatManager* m_afm = nullptr;
    TimeSliceThread m_bufferingthread;
	std::unique_ptr<StretchAudioSource> m_stretch_source;
	std::unique_ptr<MyBufferingAudioSource> m_buffering_source;
	int m_prebuffer_amount = 1;
	bool m_recreate_buffering_source = true;
	File m_audio_out_file;
    int m_fft_size_to_use = 1024;
	double m_last_outpos_pos = 0.0;
	double m_last_in_pos = 0.0;
	std::vector<int> m_bufamounts{ 4096,8192,16384,32768,65536,262144 };
};
