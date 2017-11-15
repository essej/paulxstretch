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
#include <math.h>
#include <stdlib.h>
#include "globals.h"
#include "PaulStretchControl.h"
#include "../WDL/resample.h"
#include <thread>

#ifdef WIN32
#undef min
#undef max
#endif

using namespace std;

extern std::unique_ptr<PropertiesFile> g_propsfile;

Control::Control(AudioFormatManager* afm) : m_afm(afm), m_bufferingthread("stretchbufferingthread")
{
	m_stretch_source = std::make_unique<StretchAudioSource>(2,m_afm);
	
	
	wavinfo.samplerate=44100;
	wavinfo.nsamples=0;
	
	process.bufsize=16384;
	process.stretch=4.0;
	process.onset_detection_sensitivity=0.0;

	seek_pos=0.0;
	window_type=W_HANN;
	
	volume=1.0;


	gui_sliders.fftsize_s=0.5;
	gui_sliders.stretch_s=0.5;
	gui_sliders.mode_s=0;
    
	
	
///#warning test
///	process.transient.enable=true;
};

Control::~Control()
{
	m_bufferingthread.stopThread(1000);
}
void Control::processAudio(AudioBuffer<float>& buf)
{
    jassert(m_buffering_source!=nullptr);
    jassert(m_bufferingthread.isThreadRunning());
    if (m_buffering_source != nullptr)
	{
		AudioSourceChannelInfo aif(buf);
		m_buffering_source->getNextAudioBlock(aif);
	}
}

	

void Control::setStretchAmount(double rate)
{
	m_stretch_source->setRate(rate);
}

void Control::setFFTSize(double size)
{
	if (m_prebuffer_amount == 5)
		m_fft_size_to_use = pow(2, 7.0 + size * 14.5);
	else m_fft_size_to_use = pow(2, 7.0 + size * 10.0); // chicken out from allowing huge FFT sizes if not enough prebuffering
	int optim = optimizebufsize(m_fft_size_to_use);
	m_fft_size_to_use = optim;
	m_stretch_source->setFFTSize(optim);
	//Logger::writeToLog(String(m_fft_size_to_use));
}

void Control::setOnsetDetection(double x)
{
	m_stretch_source->setOnsetDetection(x);
}

void Control::set_input_file(File file, std::function<void(String)> cb)
{
    auto task = [this,file,cb]()
    {
        
        auto ai = unique_from_raw(m_afm->createReaderFor(file));
        
        if (ai!=nullptr)
        {
            if (ai->numChannels > 32)
            {
                MessageManager::callAsync([cb,file](){ cb("Too many channels in file "+file.getFullPathName()); });
                return;
            }
            if (ai->bitsPerSample>32)
            {
                MessageManager::callAsync([cb,file](){ cb("Too high bit depth in file "+file.getFullPathName()); });
                return;
            }
            wavinfo.filename = file.getFullPathName();
            wavinfo.samplerate = ai->sampleRate;
            wavinfo.nsamples = ai->lengthInSamples;
            m_stretch_source->setAudioFile(file);
            MessageManager::callAsync([cb,file](){ cb(String()); });
            
    ///		if (process.transient.enable) {
    ///			pre_analyse_whole_audio(ai);
    ///		};

            
        } else
        {
            wavinfo.filename="";
            wavinfo.samplerate=0;
            wavinfo.nsamples=0;
            MessageManager::callAsync([cb,file](){ cb("Could not open file "+file.getFullPathName()); });
            
        };
    };
    //std::thread th(task);
    //th.detach();
	task();
};

String Control::get_input_filename(){
	return wavinfo.filename;
};

String Control::get_stretch_info(Range<double> playrange)
{
	double prate = m_stretch_source->getRate();
	double realduration = m_stretch_source->getOutputDurationSecondsForRange(playrange, m_fft_size_to_use);
	int64_t durintseconds = realduration;
	int64_t durintminutes = realduration / 60.0;
	int64_t durinthours = realduration / 3600.0;
	int64_t durintdays = realduration / (3600 * 24.0);
	String timestring;
	if (durintminutes < 1)
		timestring = String(realduration,3) +" seconds";
	if (durintminutes >= 1 && durinthours < 1)
		timestring = String(durintminutes) + " mins " + String(durintseconds % 60) + " secs";
	if (durinthours >= 1 && durintdays < 1)
		timestring = String(durinthours) + " hours " + String(durintminutes % 60) + " mins " + String(durintseconds % 60) + " secs";
	if (durintdays >= 1)
		timestring = String(durintdays) + " days " + String(durinthours % 24) + " hours " +
		String(durintminutes % 60) + " mins ";
	double fftwindowlenseconds = (double)m_fft_size_to_use / wavinfo.samplerate;
	double fftwindowfreqresol = (double)wavinfo.samplerate / 2 / m_fft_size_to_use;
	return "[Stretch " + String(prate, 1) + "x " + timestring + "] [FFT window "+String(fftwindowlenseconds,3)+" secs, frequency resolution "+
		String(fftwindowfreqresol,3)+" Hz]";
#ifdef OLDSTRETCHINFO
	const int size=200;
	char tmp[size];tmp[size-1]=0;

	if (wavinfo.nsamples==0) return "Stretch: ";


	double realduration = m_stretch_source->getOutputDuration();

	if (realduration>(365.25*86400.0*1.0e12)){//more than 1 trillion years
		double duration=(realduration/(365.25*86400.0*1.0e12));//my
		snprintf(tmp,size,"Stretch: %.7gx (%g trillion years)",process.stretch,duration);
		return tmp;
	};
	if (realduration>(365.25*86400.0*1.0e9)){//more than 1 billion years
		double duration=(realduration/(365.25*86400.0*1.0e9));//my
		snprintf(tmp,size,"Stretch: %.7gx (%g billion years)",process.stretch,duration);
		return tmp;
	};
	if (realduration>(365.25*86400.0*1.0e6)){//more than 1 million years
		double duration=(realduration/(365.25*86400.0*1.0e6));//my
		snprintf(tmp,size,"Stretch: %.7gx (%g million years)",process.stretch,duration);
		return tmp;
	};
	if (realduration>(365.25*86400.0*2000.0)){//more than two millenniums
		int duration=(int)(realduration/(365.25*86400.0));//years
		int years=duration%1000;
		int milleniums=duration/1000;

		char stryears[size];stryears[0]=0;
		if (years!=0){
			if (years==1) snprintf(stryears,size," 1 year");
			else snprintf(stryears,size," %d years",years);
		};
		snprintf(tmp,size,"Stretch: %.7gx (%d milleniums%s)",process.stretch,milleniums,stryears);
		return tmp;
	};
	if (realduration>(365.25*86400.0)){//more than 1 year
		int duration=(int) (realduration/3600.0);//hours
		int hours=duration%24;
		int days=(duration/24)%365;
		int years=duration/(365*24);

		char stryears[size];stryears[0]=0;
		if (years==1) snprintf(stryears,size,"1 year ");
		else snprintf(stryears,size,"%d years ",years);

		char strdays[size];strdays[0]=0;
		if (days>0){
			if (days==1) snprintf(strdays,size,"1 day");
			else snprintf(strdays,size,"%d days",days);
		};	
		if (years>=10) hours=0;
		char strhours[size];strhours[0]=0;
		if (hours>0){
			snprintf(strhours,size," %d h",hours);
		};	

		snprintf(tmp,size,"Stretch: %.7gx (%s%s%s)",process.stretch,stryears,strdays,strhours);
		return tmp;
	}else{//less than 1 year
		int duration=(int)(realduration);//seconds

		char strdays[size];strdays[0]=0;
		int days=duration/86400;
		if (days>0){
			if (days==1) snprintf(strdays,size,"1 day ");
			else snprintf(strdays,size,"%d days ",duration/86400);
		};	
		REALTYPE stretch=process.stretch;
		if (stretch>=1.0){
			stretch=((int) (stretch*100.0))*0.01;
		};
		snprintf(tmp,size,"Stretch: %.7gx (%s%.2d:%.2d:%.2d)",
				stretch,strdays,(duration/3600)%24,(duration/60)%60,duration%60);
		return tmp;
	};
	return "";
#endif
};

/*
string Control::get_fftresolution_info(){
	string resolution="Resolution: ";
	if (wavinfo.nsamples==0) return resolution;

	//todo: unctime and uncfreq are correct computed? Need to check later.
	REALTYPE unctime=process.bufsize/(REALTYPE)wavinfo.samplerate*sqrt(2.0);
	REALTYPE uncfreq=1.0/unctime*sqrt(2.0);
	char tmp[100];
	snprintf(tmp,100,"%.5g seconds",unctime);resolution+=tmp;
	snprintf(tmp,100," (%.5g Hz)",uncfreq);resolution+=tmp;
	return resolution;
}
*/

double Control::getPreBufferingPercent()
{
	if (m_buffering_source == nullptr)
		return 0.0;
	return m_buffering_source->getPercentReady();
}



void Control::startplay(bool /*bypass*/, bool /*realtime*/, Range<double> playrange, int numoutchans, String& err)
{
	m_stretch_source->setPlayRange(playrange, m_stretch_source->isLoopingEnabled());
	
	int bufamt = m_bufamounts[m_prebuffer_amount];
	
	if (m_buffering_source != nullptr && numoutchans != m_buffering_source->getNumberOfChannels())
		m_recreate_buffering_source = true;
	if (m_recreate_buffering_source == true)
	{
		m_buffering_source = std::make_unique<MyBufferingAudioSource>(m_stretch_source.get(),
			m_bufferingthread, false, bufamt, numoutchans, false);
		m_recreate_buffering_source = false;
	}
	if (m_bufferingthread.isThreadRunning() == false)
		m_bufferingthread.startThread();
	m_stretch_source->setNumOutChannels(numoutchans);
	m_stretch_source->setFFTSize(m_fft_size_to_use);
	update_process_parameters();
	m_last_outpos_pos = 0.0;
	m_last_in_pos = playrange.getStart()*m_stretch_source->getInfileLengthSeconds();
	m_buffering_source->prepareToPlay(1024, 44100.0);
	
	
//	sleep(100);
//	update_process_parameters();
};
void Control::stopplay()
{
    //m_adm->removeAudioCallback(&m_test_callback);
	m_bufferingthread.stopThread(1000);
};

void Control::set_seek_pos(REALTYPE x)
{
	seek_pos=x;
	m_stretch_source->seekPercent(x);
};

REALTYPE Control::get_seek_pos()
{
	return 0.0;
}

double Control::getLivePlayPosition()
{
#ifndef USEOLDPLAYCURSOR
	double outpos = m_audiocallback.m_outpos;
	double rate = 1.0 / m_stretch_source->getRate();
	double outputdiff = (outpos - m_last_outpos_pos);
	double fftlenseconds = (double)m_stretch_source->getFFTSize() / 44100.0;
	//Logger::writeToLog("output diff " + String(outputdiff)+" fft len "+String(fftlenseconds));
	//jassert(outputdiff >= 0.0 && outputdiff<0.5);
	jassert(rate > 0.0);
	double inlenseconds = m_stretch_source->getInfileLengthSeconds();
	if (inlenseconds < 0.0001)
		return 0.0;
	double inposseconds = m_stretch_source->getInfilePositionSeconds();
	double playposseconds = m_last_in_pos + (outputdiff*rate) - fftlenseconds;
	if (outputdiff*rate >= fftlenseconds || outputdiff < 0.0001)
	{
		//Logger::writeToLog("juuh "+String(inposseconds));
		m_last_in_pos = inposseconds;
		m_last_outpos_pos = outpos;
		return 1.0 / inlenseconds*(m_last_in_pos-fftlenseconds-getPreBufferAmountSeconds()*rate);
	}
	//Logger::writeToLog("jaah " + String(inposseconds));
	return 1.0 / inlenseconds*(playposseconds-getPreBufferAmountSeconds()*rate);
#else
	return m_stretch_source->getInfilePositionPercent();
#endif
}

bool Control::playing()
{
	return false;
}

void Control::set_stretch_controls(double stretch_s,int mode,double fftsize_s,double onset_detection_sensitivity)
{
	gui_sliders.stretch_s=stretch_s;
	gui_sliders.mode_s=mode;
	gui_sliders.fftsize_s=fftsize_s;

	double stretch=1.0;
	switch(mode){
		case 0:
			stretch_s=pow(stretch_s,1.2);
			stretch=pow(10.0,stretch_s*4.0);
			break;
		case 1:
			stretch_s=pow(stretch_s,1.5);
			stretch=pow(10.0,stretch_s*18.0);
			break;
		case 2:
			stretch=1.0/pow(10.0,stretch_s*2.0);
			break;
	};


	fftsize_s=pow(fftsize_s,1.5);
	int bufsize=(int)(pow(2.0,fftsize_s*12.0)*512.0);

	bufsize=optimizebufsize(bufsize);

	process.stretch=stretch;
	process.bufsize=bufsize;
	process.onset_detection_sensitivity=onset_detection_sensitivity;

};

double Control::get_stretch_control(double stretch,int mode)
{
	double result=1.0;
	switch(mode){
		case 0:
			if (stretch<1.0) return -1;
			stretch=(log(stretch)/log(10))*0.25;
			result=pow(stretch,1.0/1.2);
			break;
		case 1:
			if (stretch<1.0) return -1;
			stretch=(log(stretch)/log(10))/18.0;
			result=pow(stretch,1.0/1.5);
			break;
		case 2:
			if (stretch>1.0) return -1;
			result=2.0/(log(stretch)/log(10));
			break;
	};
	return result;
};


void Control::update_player_stretch()
{
	return;
	//player->setrap(process.stretch);
	//player->set_onset_detection_sensitivity(process.onset_detection_sensitivity);
};


int abs_val(int x){
	if (x<0) return -x;
	else return x;
};


void Control::setPreBufferAmount(int x)
{
	int temp = jlimit(0, 5, x);
	if (temp != m_prebuffer_amount)
	{
		m_prebuffer_amount = temp;
		m_recreate_buffering_source = true;
	}
}

double Control::getPreBufferAmountSeconds()
{
	return 1.0;
	
}

void Control::setAudioVisualizer(AudioVisualiserComponent * comp)
{
	//m_audiocallback.m_aviscomponent = comp;
}

void Control::setOutputAudioFileToRecord(File f)
{
	m_audio_out_file = f;
}

int Control::get_optimized_updown(int n,bool up){
	int orig_n=n;
	while(true){
		n=orig_n;

		while (!(n%11)) n/=11;
		while (!(n%7)) n/=7;

		while (!(n%5)) n/=5;
		while (!(n%3)) n/=3;
		while (!(n%2)) n/=2;
		if (n<2) break;
		if (up) orig_n++;
		else orig_n--;
		if (orig_n<4) return 4;
	};
	return orig_n;
};
int Control::optimizebufsize(int n){
	int n1=get_optimized_updown(n,false);
	int n2=get_optimized_updown(n,true);
	if ((n-n1)<(n2-n)) return n1;
	else return n2;
};



void Control::set_window_type(FFTWindow window){
	window_type=window;
	//if (player) player->set_window_type(window);
};

RenderInfoRef Control::Render2(RenderParameters renpars)
{
    RenderInfoRef rinfo = std::make_shared<RenderInfo>();
    auto bbparcopy = bbpar;
    auto processcopy = process;
    auto windowtypecopy = window_type;
    auto pparcopy = ppar;
    //auto volcopy = volume;
    auto ratecopy = m_stretch_source->getRate();
    auto fftsize = m_fft_size_to_use;
	AudioFormatManager* afm = m_afm;
    auto render_task = [rinfo,renpars, 
		bbparcopy, processcopy, windowtypecopy, pparcopy, fftsize,ratecopy,afm]()mutable->void
    {
        

		double t0 = Time::getMillisecondCounterHiRes();
        if (renpars.pos2 < renpars.pos1)
            std::swap(renpars.pos1, renpars.pos2);
        if (renpars.pos2-renpars.pos1<0.0001)
            renpars.pos2+=0.0001;
		auto ai = std::make_unique<AInputS>(afm);
        if (!ai->openAudioFile(renpars.inaudio))
        {
            rinfo->m_txt = "Error: Could not open audio file (or file format not recognized) :" + renpars.inaudio.getFileName();
            return;
        }
		if (renpars.sampleRate == 0)
			renpars.sampleRate = ai->info.samplerate;
		WavAudioFormat audioformat;
		renpars.outaudio = renpars.outaudio.getNonexistentSibling();
		auto outstream = renpars.outaudio.createOutputStream();
        int wavbits = 16;
        if (renpars.wavformat == 1)
            wavbits = 24;
        if (renpars.wavformat == 2)
            wavbits = 32;
        renpars.numoutchans = jlimit(2, g_maxnumoutchans, renpars.numoutchans);
		StringPairArray metadata;
		metadata.set(WavAudioFormat::bwavOriginator,"PaulStretch3");
		/*
		metadata.set("NumCuePoints", "2");
		metadata.set("Cue0Offset", "44100");
		metadata.set("Cue0Identifier", "0");
		metadata.set("Cue1Offset", "88200");
		metadata.set("Cue1Identifier", "1");
		*/
		AudioFormatWriter* writer = audioformat.createWriterFor(outstream, renpars.sampleRate, renpars.numoutchans, wavbits,
                                                                metadata, 0);
        if (writer == nullptr)
        {
            delete outstream;
            rinfo->m_txt = "Could not create output file";
            return;
        }
		
		auto stretchsource = std::make_unique<StretchAudioSource>(renpars.numoutchans,afm);
		if (wavbits == 2)
			stretchsource->setClippingEnabled(renpars.clipFloatOutput);
		else stretchsource->setClippingEnabled(true);
        stretchsource->setAudioFile(renpars.inaudio);
        stretchsource->setPlayRange({renpars.pos1,renpars.pos2}, renpars.numLoops>0);
        stretchsource->setRate(ratecopy);
		stretchsource->val_MainVolume.setValue(renpars.voldb);
        stretchsource->setNumOutChannels(renpars.numoutchans);
        
        stretchsource->setProcessParameters(&pparcopy);
        stretchsource->setFFTSize(fftsize);
        int bufsize = 4096;
		AudioBuffer<float> procbuf(renpars.numoutchans,bufsize);
        AudioSourceChannelInfo asinfo(procbuf);
        stretchsource->prepareToPlay(bufsize, renpars.sampleRate);
		double render_time_limit = renpars.maxrenderlen;
		int64_t outputsamplecount = 0;
		int64_t outputlen = ai->info.samplerate*stretchsource->getOutputDurationSecondsForRange({ renpars.pos1,renpars.pos2 }, fftsize);
		rinfo->m_progress_percent = 0.01;
		stretchsource->setMaxLoops(renpars.numLoops);
		while(outputsamplecount<render_time_limit*renpars.sampleRate)
        {
            if (rinfo->m_cancel == true)
            {
                rinfo->m_txt = "Cancelled";
                break;
            }
            stretchsource->getNextAudioBlock(asinfo);
            writer->writeFromAudioSampleBuffer(procbuf, 0, bufsize);
			outputsamplecount +=bufsize;
			rinfo->m_progress_percent = 1.0 / outputlen*outputsamplecount;
            if (stretchsource->hasReachedEnd())
			{
				Logger::writeToLog("StretchSource has reached end");
				break;
			}
            if (outputsamplecount>=render_time_limit*ai->info.samplerate)
            {
				rinfo->m_txt = "Render stopped at time limit";
                break;
            }
        }
        
        delete writer;
        double t1 = Time::getMillisecondCounterHiRes();
        if (rinfo->m_cancel == false)
        {
            rinfo->m_elapsed_time = t1 - t0;
            if (rinfo->m_txt.isEmpty())
				rinfo->m_txt = "Done in "+String((t1-t0)/1000.0,1)+" seconds";
        }
        else rinfo->m_txt = "Cancelled";
    };
    std::thread th([rinfo,render_task, renpars]()mutable
                   {
                       render_task();
                       MessageManager::callAsync([rinfo, renpars]()
                                                 {
                                                     renpars.completion_callback(rinfo);
                                                 });
                   });
    th.detach();
    return rinfo;
}

void Control::setPrebufferThreadPriority(int v)
{
    m_prebufthreadprior = jlimit(4,6,v);
}

string Control::getfftsizestr(int fftsize){
	const int size=100;
	char tmp[size];tmp[size-1]=0;
	if (fftsize<1024.0) snprintf(tmp,size-1,"%d",fftsize);
	else if (fftsize<(1024.0*1024.0)) snprintf(tmp,size-1,"%.4gK",fftsize/1024.0);
	else if (fftsize<(1024.0*1024.0*1024.0)) snprintf(tmp,size-1,"%.4gM",fftsize/(1024.0*1024.0));
	else snprintf(tmp,size-1,"%.7gG",fftsize/(1024.0*1024.0*1024.0));
	return tmp;
};

void Control::update_process_parameters()
{
	m_stretch_source->setProcessParameters(&ppar);
	//if (player) 
	//	player->set_process_parameters(&ppar,&bbpar);
};
