// SPDX-License-Identifier: GPLv3-or-later WITH Appstore-exception
// Copyright (C) 2017 Xenakios
// Copyright (C) 2022 Jesse Chappell


#include "StretchSource.h"
#include <set>

#ifdef WIN32
#include <ppl.h>
//#define USE_PPL_TO_PROCESS_STRETCHERS
#undef min
#undef max
#endif

std::vector<SpectrumProcessType> g_specorderpresets[3] = {
	{SPT_Harmonics,SPT_PitchShift,SPT_FreqShift,SPT_Spread,SPT_TonalVsNoise,SPT_Filter,SPT_FreeFilter,SPT_RatioMix,SPT_Compressor},
	{SPT_PitchShift,SPT_Harmonics,SPT_FreqShift,SPT_Spread,SPT_TonalVsNoise,SPT_Filter,SPT_FreeFilter,SPT_RatioMix,SPT_Compressor},
	{SPT_RatioMix,SPT_PitchShift,SPT_Harmonics,SPT_FreqShift,SPT_Spread,SPT_TonalVsNoise,SPT_Filter,SPT_FreeFilter,SPT_Compressor}
};

StretchAudioSource::StretchAudioSource(int initialnumoutchans, 
	AudioFormatManager* afm,
	std::array<AudioParameterBool*,9>& enab_pars) : m_afm(afm)
{
	m_resampler = std::make_unique<WDL_Resampler>(4*65536);
	m_resampler_outbuf.resize(1024*1024);
	m_inputfile = std::make_unique<AInputS>(m_afm);
	for (int i = 0; i < enab_pars.size(); ++i)
	{
		m_specproc_order.emplace_back((SpectrumProcessType)i, enab_pars[i]);
	}
	//m_specproc_order = { {0,false} , { 1, false} ,{2,true},{3,true},{4,true},{5,false},{6,true},{7,true},{8,false} };
	setNumOutChannels(initialnumoutchans);
	m_xfadetask.buffer.setSize(8, 65536);
	m_xfadetask.buffer.clear();

}

StretchAudioSource::~StretchAudioSource()
{
	
}

void StretchAudioSource::prepareToPlay(int samplesPerBlockExpected, double sampleRate)
{
	m_outsr = sampleRate;
	m_vol_smoother.reset(sampleRate, 0.5);
	m_lastplayrate = -1.0;
	m_stop_play_requested = false;
	m_output_counter = 0;
	m_output_silence_counter = 0;
	m_stream_end_reached = false;
	m_firstbuffer = true;
	m_output_has_begun = false;
	m_drypreviewbuf.setSize(m_num_outchans, 65536);
	initObjects();
	
}

void StretchAudioSource::releaseResources()
{
	
}

AudioBuffer<float>* StretchAudioSource::getSourceAudioBuffer()
{
    if (m_inputfile==nullptr)
        return nullptr;
    return m_inputfile->getAudioBuffer();
}

bool StretchAudioSource::isResampling()
{
    if (m_inputfile==nullptr || m_inputfile->info.samplerate==0)
        return false;
    return (int)m_outsr!=m_inputfile->info.samplerate;
}

int64_t StretchAudioSource::getDiskReadSampleCount() const
{
	if (m_inputfile == nullptr)
		return 0;
	return m_inputfile->getDiskReadSampleCount();
}

std::vector<SpectrumProcess> StretchAudioSource::getSpectrumProcessOrder()
{
	return m_specproc_order;
}

void StretchAudioSource::setSpectrumProcessOrder(std::vector<SpectrumProcess> order)
{
	ScopedLock locker(m_cs);
	m_specproc_order = order;
	/*
	Logger::writeToLog("<**");
	for (auto& e : m_specproc_order)
		Logger::writeToLog(e.m_enabled->name + " " + String(e.m_index));
	Logger::writeToLog("**>");
	*/
	for (int i = 0; i < m_stretchers.size(); ++i)
	{
		m_stretchers[i]->m_spectrum_processes = order;
	}
}

std::pair<Range<double>, Range<double>> StretchAudioSource::getFileCachedRangesNormalized()
{
	if (m_inputfile == nullptr)
		return {};
	return m_inputfile->getCachedRangesNormalized();
}

void StretchAudioSource::setFreeFilterEnvelope(shared_envelope env)
{
	ScopedLock locker(m_cs);
	m_free_filter_envelope = env;
	for (int i = 0; i < m_stretchers.size(); ++i)
	{
		m_stretchers[i]->setFreeFilterEnvelope(env);
	}
}

bool StretchAudioSource::isLoopingEnabled()
{
	if (m_inputfile == nullptr || m_inputfile->info.nsamples == 0)
		return false;
	return m_inputfile->isLooping();
}

void StretchAudioSource::setLoopingEnabled(bool b)
{
	ScopedLock locker(m_cs);
	if (m_inputfile != nullptr)
	{
		if (m_inputfile->isLooping() == false && b == true)
			seekPercent(m_inputfile->getActiveRange().getStart());
		m_inputfile->setLoopEnabled(b);
	}
}

void StretchAudioSource::setAudioBufferAsInputSource(AudioBuffer<float>* buf, int sr, int len)
{
	ScopedLock locker(m_cs);
	m_inputfile->setAudioBuffer(buf, sr, len);
	m_seekpos = 0.0;
    m_audiobuffer_is_source = true;
	m_curfile = URL();
	if (m_playrange.isEmpty())
		setPlayRange({ 0.0,1.0 });
	++m_param_change_count;
}

void StretchAudioSource::setMainVolume(double decibels)
{
	if (decibels == m_main_volume)
		return;
	if (m_cs.tryEnter())
	{
		m_main_volume = jlimit(-144.0, 12.0, decibels);
		++m_param_change_count;
		m_cs.exit();
	}
}

#ifdef OLDMODULE_ENAB
void StretchAudioSource::setSpectralModulesEnabled(const std::array<AudioParameterBool*, 9>& params)
{
	jassert(m_specprocmap.size() > 0);
	/*
	bool changed = false;
	for (int i = 0; i < m_specproc_order.size(); ++i)
	{
		int index = m_specprocmap[i];
		if (*params[index] != *m_specproc_order[i].m_enabled)
		{
			changed = true;
			break;
		}
	}
	if (changed == false)
		return;
	*/
	if (m_cs.tryEnter())
	{
		for (int i = 0; i < m_specproc_order.size(); ++i)
		{
			int index = m_specprocmap[i];
			*m_specproc_order[i].m_enabled = (bool)(*params[index]);
		}
		for (int i = 0; i < m_stretchers.size(); ++i)
		{
			m_stretchers[i]->m_spectrum_processes = m_specproc_order;
		}
		++m_param_change_count;
		m_cs.exit();
	}
}
#endif

void StretchAudioSource::setSpectralModuleEnabled(int index, bool b)
{
	++m_param_change_count;
}

void StretchAudioSource::setLoopXFadeLength(double lenseconds)
{
	if (lenseconds == m_loopxfadelen)
		return;
	if (m_cs.tryEnter())
	{
		m_loopxfadelen = jlimit(0.0, 1.0, lenseconds);
		++m_param_change_count;
		m_cs.exit();
	}
}

void StretchAudioSource::setPreviewDry(bool b)
{
	if (b == m_preview_dry)
		return;
	if (m_cs.tryEnter())
	{
		m_resampler->Reset();
		if (m_preview_dry == true && b == false && m_inputfile->info.nsamples>0)
			m_resampler->SetRates(m_inputfile->info.samplerate, m_outsr);
		m_preview_dry = b;
		++m_param_change_count;
		m_cs.exit();
	}
}

bool StretchAudioSource::isPreviewingDry() const
{
	return m_preview_dry;
}

void StretchAudioSource::setDryPlayrate(double rate)
{
	if (rate == m_dryplayrate)
		return;
	if (m_cs.tryEnter())
	{
		m_dryplayrate = rate;
		++m_param_change_count;
		m_cs.exit();
	}
}

double StretchAudioSource::getDryPlayrate() const
{
	return m_dryplayrate;
}


void StretchAudioSource::setSpectralOrderPreset(int id)
{
	if (id == m_current_spec_order_preset)
		return;
	if (m_cs.tryEnter())
	{
		m_current_spec_order_preset = id;
		++m_param_change_count;
		m_cs.exit();
	}
}


void StretchAudioSource::getNextAudioBlock(const AudioSourceChannelInfo & bufferToFill)
{
	ScopedLock locker(m_cs);
	if ( m_preview_dry == true && m_inputfile!=nullptr && m_inputfile->info.nsamples>0)
	{
        if (m_pause_state != 2)
            playDrySound(bufferToFill);

        if (m_pause_state == 1)
        {
            bufferToFill.buffer->applyGainRamp(bufferToFill.startSample, bufferToFill.numSamples, 1.0f, 0.0f);
            m_pause_state = 2;
        }
        else if (m_pause_state == 2)
        {
            bufferToFill.buffer->clear(bufferToFill.startSample,bufferToFill.numSamples);
        }
        else if (m_pause_state == 3)
        {
            bufferToFill.buffer->applyGainRamp(bufferToFill.startSample, bufferToFill.numSamples, 0.0f, 1.0f);
            m_pause_state = 0;
        }

		return;
	}
	double maingain = Decibels::decibelsToGain(m_main_volume);
	if (m_pause_state == 2)
	{
		bufferToFill.buffer->clear(bufferToFill.startSample,bufferToFill.numSamples);
		return;
	}
	if (m_stretchoutringbuf.available() > 0)
		m_output_has_begun = true;
	bool freezing = m_freezing;
	
	if (m_stretchers[0]->isFreezing() != freezing)
	{
		if (freezing == true && m_inputfile!=nullptr)
			m_freeze_pos = 1.0/m_inputfile->info.nsamples*m_inputfile->getCurrentPosition();
		for (auto& e : m_stretchers)
		{
			e->set_freezing(freezing);
		}
	}
    
	
	if (m_vol_smoother.getTargetValue() != maingain)
		m_vol_smoother.setTargetValue(maingain);
	FloatVectorOperations::disableDenormalisedNumberSupport();
	float* const* outarrays = bufferToFill.buffer->getArrayOfWritePointers();
	int outbufchans = jmin(m_num_outchans, bufferToFill.buffer->getNumChannels());
	int offset = bufferToFill.startSample;
	if (m_stretchers.size() == 0)
		return;
	if (m_inputfile == nullptr)
		return;
	if (m_inputfile->info.nsamples == 0)
		return;
	m_inputfile->setXFadeLenSeconds(m_loopxfadelen);
    
	double silencethreshold = Decibels::decibelsToGain(-70.0);
	auto ringbuffilltask = [this](int framestoproduce)
	{
		while (m_stretchoutringbuf.available() < framestoproduce*m_num_outchans)
		{
			int readsize = 0;
			double in_pos = (double)m_inputfile->getCurrentPosition() / (double)m_inputfile->info.nsamples;
            float in_pos_100 = in_pos*100.0;

            if (m_firstbuffer)
			{
				readsize = m_stretchers[0]->get_nsamples_for_fill();
				m_firstbuffer = false;
			}
			else
			{
				readsize = m_stretchers[0]->get_nsamples(in_pos*100.0);
			};
			int readed = 0;
			if (readsize != 0)
			{
				m_last_filepos = m_inputfile->getCurrentPosition();
				readed = m_inputfile->readNextBlock(m_file_inbuf, readsize, m_num_outchans);
			}
			if (m_rand_count % (int)m_free_filter_envelope->m_transform_y_random_rate == 0)
			{
				m_free_filter_envelope->updateRandomState();
			}
			++m_rand_count;
			
			auto inbufptrs = m_file_inbuf.getArrayOfWritePointers();
			REALTYPE onset_max = std::numeric_limits<REALTYPE>::min();
#ifdef USE_PPL_TO_PROCESS_STRETCHERS
			std::array<REALTYPE, 16> onset_values_arr;
			Concurrency::parallel_for(0, (int)m_stretchers.size(), [this, readed, &onset_values_arr](int i)
			{
				REALTYPE onset_val = m_stretchers[i]->process(m_inbufs[i].data(), readed);
				onset_values_arr[i] = onset_val;
			});
			for (int i = 0; i < m_stretchers.size(); ++i)
				onset_max = std::max(onset_max, onset_values_arr[i]);
#else
			for (int i = 0; i < m_stretchers.size(); ++i)
			{
				REALTYPE onset_l = m_stretchers[i]->process(inbufptrs[i], readed);
				onset_max = std::max(onset_max, onset_l);
			}
#endif
			for (int i = 0; i < m_stretchers.size(); ++i)
				m_stretchers[i]->here_is_onset(onset_max);
			int outbufsize = m_stretchers[0]->get_bufsize();

            if (m_stretchers.size() > 1) {
                m_binaural_beats->process(m_stretchers[0]->out_buf.data(),m_stretchers[1]->out_buf.data(),
                                          outbufsize, in_pos_100);
            }

			int nskip = m_stretchers[0]->get_skip_nsamples();
			if (nskip > 0)
				m_inputfile->skip(nskip);

			for (int i = 0; i < outbufsize; i++)
			{
				for (int ch = 0; ch < m_num_outchans; ++ch)
				{
					REALTYPE outsa = m_stretchers[ch]->out_buf[i];
					m_stretchoutringbuf.push(outsa);

				}
			}
			
		}
		
	};
	int previousxfadestate = m_xfadetask.state;
	auto resamplertask = [this, &ringbuffilltask, &bufferToFill]()
	{
		double* rsinbuf = nullptr;
		int outsamplestoproduce = bufferToFill.numSamples;
		if (m_xfadetask.state == 1)
			outsamplestoproduce = m_xfadetask.xfade_len;
		int wanted = m_resampler->ResamplePrepare(outsamplestoproduce, m_num_outchans, &rsinbuf);
		ringbuffilltask(wanted);
		for (int i = 0; i < wanted*m_num_outchans; ++i)
		{
			double sample = m_stretchoutringbuf.get();
			rsinbuf[i] = sample;
		}
		if (outsamplestoproduce*m_num_outchans > m_resampler_outbuf.size())
		{
			m_resampler_outbuf.resize(outsamplestoproduce*m_num_outchans);
		}
		/*int produced =*/ m_resampler->ResampleOut(m_resampler_outbuf.data(), wanted, outsamplestoproduce, m_num_outchans);
		if (m_xfadetask.state == 1)
		{
			//Logger::writeToLog("Filling xfade buffer");
			for (int i = 0; i < outsamplestoproduce; ++i)
			{
				for (int j = 0; j < m_num_outchans; ++j)
				{
					m_xfadetask.buffer.setSample(j, i, m_resampler_outbuf[i*m_num_outchans + j]);
				}
			}
			if (m_process_fftsize != m_xfadetask.requested_fft_size)
			{
				m_process_fftsize = m_xfadetask.requested_fft_size;
				//Logger::writeToLog("Initing stretcher objects");
				initObjects();
			}
			m_xfadetask.state = 2;
		}
	};

	resamplertask();
	if (previousxfadestate == 1 && m_xfadetask.state == 2)
	{
		//Logger::writeToLog("Rerunning resampler task");
		resamplertask();
	}
	
	bool source_ended = m_inputfile->hasEnded();
	double samplelimit = 16384.0;
	if (m_clip_output == true)
		samplelimit = 1.0;
	for (int i = 0; i < bufferToFill.numSamples; ++i)
	{
		double smoothed_gain = m_vol_smoother.getNextValue();
		double mixed = 0.0;
		for (int j = 0; j < outbufchans; ++j)
		{
			double outsample = m_resampler_outbuf[i*m_num_outchans + j];
			if (m_xfadetask.state == 2)
			{
				double xfadegain = 1.0 / m_xfadetask.xfade_len*m_xfadetask.counter;
				jassert(xfadegain >= 0.0 && xfadegain <= 1.0);
				double outsample2 = m_xfadetask.buffer.getSample(j, m_xfadetask.counter);
				outsample = xfadegain * outsample + (1.0 - xfadegain)*outsample2;
			}
			outarrays[j][i + offset] = jlimit(-samplelimit,samplelimit , outsample * smoothed_gain);
			mixed += outsample;
		}
		if (m_xfadetask.state == 2)
		{
			++m_xfadetask.counter;
			if (m_xfadetask.counter >= m_xfadetask.xfade_len)
				m_xfadetask.state = 0;
		}
		if (source_ended && m_output_counter>=2*m_process_fftsize)
		{
			if (fabs(mixed) < silencethreshold)
				++m_output_silence_counter;
			else
				m_output_silence_counter = 0;
		}
		
	}
	if (m_pause_state == 1)
	{
		bufferToFill.buffer->applyGainRamp(bufferToFill.startSample, bufferToFill.numSamples, 1.0f, 0.0f);
		m_pause_state = 2;
	}
	if (m_pause_state == 3)
	{
		bufferToFill.buffer->applyGainRamp(bufferToFill.startSample, bufferToFill.numSamples, 0.0f, 1.0f);
		m_pause_state = 0;
	}
	m_output_counter += bufferToFill.numSamples;
}

void StretchAudioSource::setNextReadPosition(int64 /*newPosition*/)
{
	
}

int64 StretchAudioSource::getNextReadPosition() const
{
	return int64();
}

int64 StretchAudioSource::getTotalLength() const
{
	if (m_inputfile == nullptr)
		return 0;
	return m_inputfile->info.nsamples;
}

bool StretchAudioSource::isLooping() const
{
	return false;
}

String StretchAudioSource::setAudioFile(const URL & url)
{
	ScopedLock locker(m_cs);
	if (m_inputfile->openAudioFile(url))
	{
		m_curfile = url;
		m_firstbuffer = true;
        m_audiobuffer_is_source = false;
        initObjects();
		return String();
	}
	return "Could not open file";
}

URL StretchAudioSource::getAudioFile()
{
	return m_curfile;
}

void StretchAudioSource::setNumOutChannels(int chans)
{
	jassert(chans > 0 && chans < g_maxnumoutchans);
	m_num_outchans = chans;
}

void StretchAudioSource::initObjects()
{
	ScopedLock locker(m_cs);
	m_inputfile->setActiveRange(m_playrange);
	if (m_inputfile->getActiveRange().contains(m_inputfile->getCurrentPositionPercent())==false)
		m_inputfile->seek(m_playrange.getStart(), true);
	
	m_firstbuffer = true;
	if (m_stretchoutringbuf.getSize() < m_num_outchans*m_process_fftsize)
	{
		int newsize = m_num_outchans*m_process_fftsize*2;
		//Logger::writeToLog("Resizing circular buffer to " + String(newsize));
		m_stretchoutringbuf.resize(newsize);
	}
    if (m_xfadetask.buffer.getNumChannels() < m_num_outchans)
    {
        m_xfadetask.buffer.setSize(m_num_outchans, m_xfadetask.buffer.getNumSamples());
    }
	m_stretchoutringbuf.clear();
	m_resampler->Reset();
	m_resampler->SetRates(m_inputfile->info.samplerate, m_outsr);
	REALTYPE stretchratio = m_playrate;
    FFTWindow windowtype = W_HAMMING;
    if (m_fft_window_type>=0)
        windowtype = (FFTWindow)m_fft_window_type;
	int inbufsize = m_process_fftsize;
	double onsetsens = m_onsetdetection;
	m_stretchers.resize(m_num_outchans);
	for (int i = 0; i < m_stretchers.size(); ++i)
	{
		if (m_stretchers[i] == nullptr)
		{
			//Logger::writeToLog("Creating stretch instance " + String(i));
			m_stretchers[i] = std::make_shared<ProcessedStretch>(stretchratio,
				m_process_fftsize, windowtype, false, (float)m_inputfile->info.samplerate, i + 1);
		}
		m_stretchers[i]->setBufferSize(m_process_fftsize);
		m_stretchers[i]->setSampleRate(m_inputfile->info.samplerate);
		m_stretchers[i]->set_onset_detection_sensitivity(onsetsens);
		m_stretchers[i]->set_parameters(&m_ppar);
		m_stretchers[i]->set_freezing(m_freezing);
		m_stretchers[i]->setFreeFilterEnvelope(m_free_filter_envelope);
		fill_container(m_stretchers[i]->out_buf, 0.0f);
		m_stretchers[i]->m_spectrum_processes = m_specproc_order;
	}
    m_binaural_beats = std::make_unique<BinauralBeats>(m_inputfile->info.samplerate);
    m_binaural_beats->pars = m_bbpar;

	m_file_inbuf.setSize(m_num_outchans, 3 * inbufsize);
}

void StretchAudioSource::playDrySound(const AudioSourceChannelInfo & bufferToFill)
{
	auto bufs = bufferToFill.buffer->getArrayOfWritePointers();
	double maingain = Decibels::decibelsToGain(m_main_volume);
	m_inputfile->setXFadeLenSeconds(m_loopxfadelen);
	double* rsinbuf = nullptr;
	m_resampler->SetRates(m_inputfile->info.samplerate*m_dryplayrate, m_outsr);
	int wanted = m_resampler->ResamplePrepare(bufferToFill.numSamples, m_num_outchans, &rsinbuf);
	m_inputfile->readNextBlock(m_drypreviewbuf, wanted, m_num_outchans);
	for (int i = 0; i < wanted; ++i)
		for (int j = 0; j < m_num_outchans; ++j)
			rsinbuf[i*m_num_outchans + j] = m_drypreviewbuf.getSample(j, i);
	m_resampler->ResampleOut(m_resampler_outbuf.data(), wanted, bufferToFill.numSamples, m_num_outchans);
	for (int i = 0; i < m_num_outchans; ++i)
		for (int j = 0; j < bufferToFill.numSamples; ++j)
			bufs[i][j + bufferToFill.startSample] = maingain * m_resampler_outbuf[j*m_num_outchans + i];
}

double StretchAudioSource::getLastSourcePositionPercent()
{
    if (m_inputfile == nullptr || m_inputfile->info.nsamples == 0)
        return 0.0;
    return (1.0/m_inputfile->info.nsamples)*m_last_filepos;
}


double StretchAudioSource::getInfilePositionPercent()
{
	if (m_inputfile == nullptr || m_inputfile->info.nsamples == 0)
		return 0.0;
	return 1.0/m_inputfile->info.nsamples*m_inputfile->getCurrentPosition();
}

double StretchAudioSource::getInfilePositionSeconds()
{
	if (m_inputfile == nullptr || m_inputfile->info.nsamples == 0)
		return 0.0;
	//return m_lastinpos*m_inputfile->getLengthSeconds();
	return (double)m_inputfile->getCurrentPosition() / m_inputfile->info.samplerate;
}

double StretchAudioSource::getInfileLengthSeconds()
{
	if (m_inputfile == nullptr || m_inputfile->info.nsamples == 0)
		return 0.0;
	return (double)m_inputfile->info.nsamples / m_inputfile->info.samplerate;
}

double StretchAudioSource::getInfileSamplerate()
{
	if (m_inputfile == nullptr)
		return 0.0;
	return m_inputfile->info.samplerate;
}

void StretchAudioSource::setRate(double rate)
{
	if (rate == m_playrate)
		return;
	if (m_cs.tryEnter())
	{
		m_playrate = rate;
		for (int i = 0; i < m_stretchers.size(); ++i)
		{
			m_stretchers[i]->set_rap((float)rate);
		}
		++m_param_change_count;
		m_cs.exit();
	}
}

void StretchAudioSource::setProcessParameters(ProcessParameters * pars, BinauralBeatsParameters * bbpars)
{
	if (*pars == m_ppar && (!bbpars || m_bbpar == *bbpars))
		return;
	if (m_cs.tryEnter())
	{
		m_ppar = *pars;
        if (bbpars) {
            m_bbpar = *bbpars;
            m_binaural_beats->pars = m_bbpar;
        }

		for (int i = 0; i < m_stretchers.size(); ++i)
		{
			m_stretchers[i]->set_parameters(pars);
		}
		++m_param_change_count;
		m_cs.exit();
	}
}

const ProcessParameters& StretchAudioSource::getProcessParameters()
{
	return m_ppar;
}

void StretchAudioSource::setFFTWindowingType(int windowtype)
{
    if (windowtype==m_fft_window_type)
        return;
	if (m_cs.tryEnter())
	{
		m_fft_window_type = windowtype;
		for (int i = 0; i < m_stretchers.size(); ++i)
		{
			m_stretchers[i]->window_type = (FFTWindow)windowtype;
		}
		++m_param_change_count;
		m_cs.exit();
	}
}

void StretchAudioSource::setFFTSize(int size, bool force)
{
    jassert(size>0);
    if (force || (m_xfadetask.state == 0 && (m_process_fftsize == 0 || size != m_process_fftsize)))
	{
        DBG("Using FFT size: " << size);

		ScopedLock locker(m_cs);
		if (m_xfadetask.buffer.getNumChannels() < m_num_outchans)
		{
			m_xfadetask.buffer.setSize(m_num_outchans, m_xfadetask.buffer.getNumSamples());
			
		}
		if (!force && m_process_fftsize > 0)
		{
			m_xfadetask.state = 1;
			m_xfadetask.counter = 0;
			m_xfadetask.xfade_len = 16384;
			m_xfadetask.requested_fft_size = size;
		}
		else
		{
			m_process_fftsize = size;
			initObjects();
		}


		++m_param_change_count;
	}
}

void StretchAudioSource::setPaused(bool b)
{
	if (b == true && m_pause_state>0)
		return;
	if (b == false && m_pause_state == 0)
		return;
	ScopedLock locker(m_cs);
	if (b == true && m_pause_state == 0)
	{
		m_pause_state = 1;
		return;
	}
	if (b == false && m_pause_state == 2)
	{
		m_pause_state = 3;
		return;
	}
}

bool StretchAudioSource::isPaused() const
{
	return m_pause_state > 0;
}

void StretchAudioSource::seekPercent(double pos)
{
	ScopedLock locker(m_cs);
	m_seekpos = pos;
	//m_firstbuffer = true;
	//m_resampler->Reset();
	m_inputfile->seek(pos, true);
	++m_param_change_count;
}

double StretchAudioSource::getOutputDurationSecondsForRange(Range<double> range, int fftsize)
{
	if (m_inputfile == nullptr || m_inputfile->info.nsamples == 0)
		return 0.0;
	if (m_preview_dry==true)
        return (double)range.getLength()*m_inputfile->info.nsamples/m_inputfile->info.samplerate;
    int64_t play_end_pos = (fftsize * 2)+range.getLength()*m_playrate*m_inputfile->info.nsamples;
	return (double)play_end_pos / m_inputfile->info.samplerate;
}

void StretchAudioSource::setOnsetDetection(double x)
{
	if (x == m_onsetdetection)
		return;
	if (m_cs.tryEnter())
	{
		m_onsetdetection = x;
		for (int i = 0; i < m_stretchers.size(); ++i)
		{
			m_stretchers[i]->set_onset_detection_sensitivity((float)x);
		}
		++m_param_change_count;
		m_cs.exit();
	}
}

void StretchAudioSource::setPlayRange(Range<double> playrange, bool force)
{
	if (!force && (playrange == m_playrange || playrange == m_inputfile->getActiveRange()))
		return;
	if (m_cs.tryEnter() || force)
	{
		if (playrange.isEmpty())
			m_playrange = { 0.0,1.0 };
		else
			m_playrange = playrange;
		m_stream_end_reached = false;
		m_inputfile->setActiveRange(m_playrange);
		
		//if (m_playrange.contains(m_seekpos) == false)
		//	m_inputfile->seek(m_playrange.getStart());
		m_seekpos = m_playrange.getStart();
		++m_param_change_count;
		m_cs.exit();
	}
}

bool StretchAudioSource::isLoopEnabled()
{
	if (m_inputfile == nullptr)
		return false;
	return m_inputfile->isLooping();
}

bool StretchAudioSource::hasReachedEnd()
{
	if (m_inputfile == nullptr)
		return false;
	if (m_inputfile->isLooping() && m_maxloops == 0)
		return false;
	if (m_inputfile->isLooping() && m_inputfile->getLoopCount() > m_maxloops)
		return true;
	//return m_output_counter>=m_process_fftsize*2;
	return m_output_silence_counter>=65536;
}
