/*
  Copyright (C) 2006-2009 Nasca Octavian Paul
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

#ifdef PLAYERCLASSSTILLPRESENT

#include <stdlib.h>
#include <stdio.h>
//#include <unistd.h>
#include <math.h>
#include "Player.h"
#include "globals.h"

extern std::unique_ptr<AudioFormatManager> g_audioformatmanager;

static int player_count=0;

Player::Player() : Thread("psplayerthread"){
	player_count++;
	if (player_count>1) {
		printf("Error: Player class multiples instances.\n");
		exit(1);
	};

	newtask.mode=TASK_NONE;
	newtask.startpos=0.0;
	newtask.rap=1.0;
	newtask.fftsize=4096;

	task=newtask;
	mode=MODE_STOP;

	outbuf.n=0;
	
	outbuf.size=0;
	outbuf.computek=0;
	outbuf.outk=0;
	outbuf.outpos=0;
	outbuf.nfresh=0;
	
	
	info.position=0;
	freeze_mode=false;
	bypass_mode=false;
	first_in_buf=true;
	window_type=W_HANN;
	

	paused=false;

	info.playing=false;
	info.samplerate=44100;
	info.eof=true;
	volume=1.0;
	onset_detection_sensitivity=0.0;
};

Player::~Player(){
	player_count--;

	stop();
};

Player::ModeType Player::getmode(){
	return mode;
};

void Player::startplay(String filename, REALTYPE startpos,REALTYPE rap, int fftsize,bool bypass,
	ProcessParameters *ppar,BinauralBeatsParameters *bbpar, double loop_start, double loop_end, int numoutchans)
{
	info.playing=false;
	info.eof=false;
	bypass_mode=bypass;
	if (bypass) freeze_mode=false;
	paused=false;
	taskmutex.lock();
	newtask.mode=TASK_START;
	newtask.filename=filename;
	newtask.startpos=startpos;
	newtask.m_num_outchans = numoutchans;
	newtask.rap=rap;
	newtask.fftsize=fftsize;
	newtask.bypass=bypass;
	newtask.ppar=ppar;
	newtask.bbpar=bbpar;
    newtask.m_loop_start = loop_start;
    newtask.m_loop_end = loop_end;
	taskmutex.unlock();
};

void Player::stop(){
	//pun 0 la outbuf
	info.playing=false;
	/*    taskmutex.lock();
		  newtask.mode=TASK_STOP;
		  taskmutex.unlock();*/
};
void Player::pause(){
	paused=!paused;
};

void Player::seek(REALTYPE pos){
	taskmutex.lock();
	newtask.mode=TASK_SEEK;
	newtask.startpos=pos;
	taskmutex.unlock();
};
void Player::freeze(){
	freeze_mode=!freeze_mode;
	if (bypass_mode) freeze_mode=false;
};

void Player::setrap(REALTYPE newrap){
	taskmutex.lock();
	newtask.mode=TASK_RAP;
	newtask.rap=newrap;
	taskmutex.unlock();
};

void Player::set_process_parameters(ProcessParameters *ppar,BinauralBeatsParameters *bbpar){
	taskmutex.lock();
	newtask.mode=TASK_PARAMETERS;
	newtask.ppar=ppar;
	newtask.bbpar=bbpar;
	taskmutex.unlock();
};


void Player::set_window_type(FFTWindow window){
	window_type=window;
};

void Player::set_volume(REALTYPE vol){
	volume=vol;
};
void Player::set_onset_detection_sensitivity(REALTYPE onset){
	onset_detection_sensitivity=onset;
};

void Player::getaudiobuffer(int nsamples, float *out)
{
	int numoutchans = task.m_num_outchans;
	if (mode==MODE_PREPARING){
		for (int i=0;i<nsamples*numoutchans;i++)
		{
			out[i]=0.0;
		};
		return;
	};
	if (paused){
		for (int i=0;i<nsamples*numoutchans;i++)
		{
			out[i]=0.0;
		};
		return;
	};

	bufmutex.lock();
	if ((outbuf.n==0)||(outbuf.nfresh==0)){
		bufmutex.unlock();
		for (int i=0;i<nsamples*numoutchans;i++)
		{
			out[i]=0.0;
		};
		return;
	};
	int k=outbuf.outk,pos=outbuf.outpos;

	//     printf("%d in_pos=%g\n",info.eof,outbuf.in_position[k]);
	if (info.eof) mode=MODE_STOP;
	else info.position=outbuf.in_position[k];
	for (int i=0;i<nsamples;i++)
	{
		for (int j=0;j<numoutchans;++j)
			out[i*numoutchans+j]=jlimit(-1.0f,1.0f, outbuf.channeldatas[j][k][pos]*volume);
		pos++;
		if (pos>=outbuf.size) 
		{
			pos=0;
			k++;
			if (k>=outbuf.n) k=0;

			outbuf.nfresh--;
			//printf("%d %d\n",k,outbuf.nfresh);

			if (outbuf.nfresh<0){//Underflow
				outbuf.nfresh=0;
				for (int j=i;j<nsamples;j++)
				{
					for (int ch=0;ch<numoutchans;++ch)
						out[j*numoutchans+ch]=0.0;
				};
				break;
			};

		};
	};
	outbuf.outk=k;
	outbuf.outpos=pos;
	bufmutex.unlock();


};


void Player::run(){
	while(1)
    {
		if (threadShouldExit())
            break;
        newtaskcheck();

		if (mode==MODE_STOP) sleep(10);
		if ((mode==MODE_PLAY)||(mode==MODE_PREPARING)){
			computesamples();
		};
		task.mode=TASK_NONE;

	};
};

void Player::newtaskcheck(){
	TaskMode newmode=TASK_NONE;
	taskmutex.lock();
	if (task.mode!=newtask.mode) {
		newmode=newtask.mode;
		task=newtask;
	};
	newtask.mode=TASK_NONE;
	taskmutex.unlock();

	if (newmode==TASK_START){
		if (current_filename!=task.filename)
		{
			current_filename=task.filename;
			//task.startpos=0.0;
		};
		ai = std::make_unique<AInputS>(g_audioformatmanager.get());
		if (ai->openAudioFile(task.filename))
		{
			info.samplerate=ai->info.samplerate;
			mode=MODE_PREPARING;
			ai->seek(task.startpos);
            if (task.m_loop_start>=0.0 && task.m_loop_end>task.m_loop_start)
            {
                ai->m_loop_start = task.m_loop_start;
                ai->m_loop_end = task.m_loop_end;
                ai->m_loop_enabled = true;
            } else
            {
                ai->m_loop_enabled = false;
            }
			bufmutex.lock();
            m_stretchers.resize(task.m_num_outchans);
            for (int i=0;i<m_stretchers.size();++i)
            {
                m_stretchers[i]=std::make_shared<ProcessedStretch>(task.rap,task.fftsize,window_type,task.bypass,ai->info.samplerate,i+1);
                m_stretchers[i]->set_parameters(task.ppar);
            }
            binaural_beats=std::make_unique<BinauralBeats>(ai->info.samplerate);

			if (binaural_beats) 
				binaural_beats->pars=*(task.bbpar);

			inbufsize=m_stretchers[0]->get_max_bufsize();
			
            inbuf.m_inbuf.resize(task.m_num_outchans);
            for (int i=0;i<inbuf.m_inbuf.size();++i)
                inbuf.m_inbuf[i].resize(inbufsize);
			for (int i=0;i<inbufsize;i++)
                for (int j=0;j<inbuf.m_inbuf.size();++j)
                    inbuf.m_inbuf[j][i]=0.0;

			
			inbuf_i.resize(inbufsize*task.m_num_outchans);
			for (int i=0;i<inbufsize*task.m_num_outchans;i++)
			{
				inbuf_i[i]=0;
			};
			first_in_buf=true;

			outbuf.size=m_stretchers[0]->get_bufsize();

			int min_samples=ai->info.samplerate*m_prebufferamount;
			int hwbufsize = 1024;
			int n=2*hwbufsize/outbuf.size;
			if (n<3) n=3;//min 3 buffers
			if (n<(min_samples/outbuf.size)) n=(min_samples/outbuf.size);//the internal buffers sums "min_samples" amount
			outbuf.n=n;
			outbuf.nfresh=0;
            outbuf.channeldatas.resize(task.m_num_outchans);
            for (int i=0;i<task.m_num_outchans;++i)
                outbuf.channeldatas[i].resize(outbuf.n);
            outbuf.computek=0;
			outbuf.outk=0;
			outbuf.outpos=0;
			outbuf.in_position.resize(outbuf.n);
			for (int j=0;j<outbuf.n;j++){
                for (int k=0;k<task.m_num_outchans;++k)
                {
                    outbuf.channeldatas[k][j].resize(outbuf.size);
                    for (int i=0;i<outbuf.size;i++)
                        outbuf.channeldatas[k][j][i]=0.0;
                }
                
				outbuf.in_position[j]=0.0;
			};

			bufmutex.unlock();	    	    

		};
	};
	if (newmode==TASK_SEEK){
		if (ai) ai->seek(task.startpos);
		first_in_buf=true;
	};
	if (newmode==TASK_RAP)
    {
        for (int i=0;i<m_stretchers.size();++i)
            m_stretchers[i]->set_rap(task.rap);
		
	};
	if (newmode==TASK_PARAMETERS){
        for (int i=0;i<m_stretchers.size();++i)
            m_stretchers[i]->set_parameters(task.ppar);
        if (binaural_beats) binaural_beats->pars=*(task.bbpar);
	};
};

void Player::computesamples(){
	bufmutex.lock();
	
	bool exitnow=(outbuf.n==0);
	if (outbuf.nfresh>=(outbuf.n-1)) exitnow=true;//buffers are full

	bufmutex.unlock();
	if (exitnow) {
		if (mode==MODE_PREPARING) {
			info.playing=true;
			mode=MODE_PLAY;
		};
		sleep(10);
		return;
	};
	int outchs = task.m_num_outchans;
	bool eof=false;
	if (!ai) 
		eof=true;
	else if (ai->eof) eof=true;
	if (eof)
	{
		
		for (int i=0;i<inbufsize*outchs;i++)
		{
			inbuf_i[i] = 0.0;
		};
		outbuf.nfresh++;
		bufmutex.lock();
		for (int i=0;i<outbuf.size;i++)
		{
			for (int j=0;j<outchs;++j)
				outbuf.channeldatas[j][outbuf.computek][i]=0.0;
		};
		outbuf.computek++;
		if (outbuf.computek>=outbuf.n){
			outbuf.computek=0;
		};
		bufmutex.unlock();
		info.eof=true;
		return;
	};  

	bool result=true;
	float in_pos_100=(REALTYPE) ai->info.currentsample/(REALTYPE)ai->info.nsamples*100.0;
	info.liveposition = in_pos_100/100.0;
	int readsize=m_stretchers[0]->get_nsamples(in_pos_100);


	if (first_in_buf)
        readsize=m_stretchers[0]->get_nsamples_for_fill();
    else if (freeze_mode) readsize=0;
	if (readsize) 
		result=(ai->read(readsize,outchs,inbuf_i.data())==(readsize));
	if (result)
	{
		float in_pos=(REALTYPE) ai->info.currentsample/(REALTYPE)ai->info.nsamples;
		
		if (ai->eof) in_pos=0.0;

		for (int i=0;i<readsize;i++)
        {
            for (int j=0;j<inbuf.m_inbuf.size();++j)
                inbuf.m_inbuf[j][i]=inbuf_i[i*outchs+j];
			
		};
        REALTYPE max_onset = std::numeric_limits<REALTYPE>::min();
        for (int i=0;i<m_stretchers.size();++i)
        {
            m_stretchers[i]->window_type=window_type;
            REALTYPE s_onset=onset_detection_sensitivity;
            m_stretchers[i]->set_onset_detection_sensitivity(s_onset);
            bool freezing=freeze_mode&&(!first_in_buf);
            m_stretchers[i]->set_freezing(freezing);
            
            REALTYPE onset_amt=m_stretchers[i]->process(inbuf.m_inbuf[i].data(),readsize);
            max_onset = std::max(onset_amt,max_onset);
        }
        for (int i=0;i<m_stretchers.size();++i)
            m_stretchers[i]->here_is_onset(max_onset);
		binaural_beats->process(m_stretchers[0]->out_buf.data(),m_stretchers[1]->out_buf.data(),
			m_stretchers[0]->get_bufsize(),in_pos_100);
		//	stretchl->process_output(stretchl->out_buf,stretchl->out_bufsize);
		//	stretchr->process_output(stretchr->out_buf,stretchr->out_bufsize);
		int nskip=m_stretchers[0]->get_skip_nsamples();
		if (nskip>0) ai->skip(nskip);


		first_in_buf=false;
		bufmutex.lock();

        for (int ch=0;ch<outchs;++ch)
        {
            for (int i=0;i<outbuf.size;i++)
            {
                REALTYPE out_sample=m_stretchers[ch]->out_buf[i];
                outbuf.channeldatas[ch][outbuf.computek][i]=out_sample;
            }
        }
		outbuf.in_position[outbuf.computek]=in_pos;
		outbuf.computek++;
		if (outbuf.computek>=outbuf.n){
			outbuf.computek=0;
		};
		bufmutex.unlock();
		outbuf.nfresh++;
	}else{
		info.eof=true;
		mode=MODE_STOP;
		stop();
	};
};

#endif
