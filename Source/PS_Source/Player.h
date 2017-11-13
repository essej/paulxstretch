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

#ifdef PLAYERCLASSSTILLPRESENT

#include <string>
#include "Input/AInputS.h"
#include "ProcessedStretch.h"
#include "BinauralBeats.h"
#include "Mutex.h"
#include "globals.h"

using StretchRef = std::shared_ptr<ProcessedStretch>;

class Player:public Thread{
public:
	Player();
	~Player();
	
	void startplay(String filename, REALTYPE startpos,REALTYPE rap, int fftsize,bool bypass,ProcessParameters *ppar,
		BinauralBeatsParameters *bbpar, double loop_start, double loop_end, int numoutchans);
	    //startpos is from 0 (start) to 1.0 (end of file)
	void stop();
	void pause();
	
	void freeze();
	void setrap(REALTYPE newrap);
	
	void seek(REALTYPE pos);
	
    
    
	void getaudiobuffer(int nsamples, float *out);//data este stereo
	
	enum ModeType{
	    MODE_PLAY,MODE_STOP,MODE_PREPARING,MODE_PAUSE
	};
	
	ModeType getmode();
	
	struct player_soundfile_info_t{
	    float position = 0.0f;//0 is for start, 1 for end
		double liveposition = 0.0;
		int playing=0;
	    int samplerate=0;
	    bool eof=false;
	}info;

	bool is_freeze(){
	    return freeze_mode;
	};
	void set_window_type(FFTWindow window);
	
	void set_volume(REALTYPE vol);
	void set_onset_detection_sensitivity(REALTYPE onset);
	
	void set_process_parameters(ProcessParameters *ppar,BinauralBeatsParameters *bbpar);
	
	std::unique_ptr<BinauralBeats> binaural_beats;
	void setPreBufferAmount(double x) { m_prebufferamount = jlimit(0.1,2.0, x); }
private:
	void run() override;
    
	std::unique_ptr<InputS> ai;
    
    std::vector<StretchRef> m_stretchers;

	std::vector<float> inbuf_i;
	int inbufsize;

	Mutex taskmutex,bufmutex;
	
	ModeType mode;
	
	enum TaskMode{
	    TASK_NONE, TASK_START, TASK_STOP,TASK_SEEK,TASK_RAP,TASK_PARAMETERS, TASK_ONSET
	};
	struct player_task_t {
	    TaskMode mode;

	    REALTYPE startpos=0.0;
	    REALTYPE rap;
	    int fftsize;
	    String filename;
		int m_num_outchans = 2;
	    bool bypass;
	    ProcessParameters *ppar;
	    BinauralBeatsParameters *bbpar;
        double m_loop_start = 0.0;
        double m_loop_end = 1.0;
	}newtask,task;
	
	struct{
	    float2dvector m_inbuf;
	}inbuf;
	
    
	struct player_outbuf_t{
	    int n;//how many buffers
	    float3dvector channeldatas;
	    int size;//size of one buffer
	    int computek,outk;//current buffer
	    int outpos;//the sample position in the current buffer (for out)
	    int nfresh;//how many buffers are fresh added and need to be played
		    //nfresh==0 for empty buffers, nfresh==n-1 for full buffers
        std::vector<float> in_position;//the position(for input samples inside the input file) of each buffers
	}outbuf;
	bool first_in_buf;
	
	void newtaskcheck();
	void computesamples();
	bool freeze_mode,bypass_mode,paused;
	REALTYPE volume,onset_detection_sensitivity;

	String current_filename;
	FFTWindow window_type;
	double m_prebufferamount = 0.2;
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Player)
};
#endif
