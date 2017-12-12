#include "StretchSource.h"

#ifdef WIN32
#include <ppl.h>
//#define USE_PPL_TO_PROCESS_STRETCHERS
#undef min
#undef max
#endif

StretchAudioSource::StretchAudioSource(int initialnumoutchans, AudioFormatManager* afm) : m_afm(afm)
{
	m_resampler = std::make_unique<WDL_Resampler>();
	m_resampler_outbuf.resize(1024*1024);
	m_inputfile = std::make_unique<AInputS>(m_afm);
	m_specproc_order = { 0,1,2,3,4,5,6,7 };
	setNumOutChannels(initialnumoutchans);
	m_xfadetask.buffer.setSize(initialnumoutchans, 65536);
	m_xfadetask.buffer.clear();
}

StretchAudioSource::~StretchAudioSource()
{
	
}

void StretchAudioSource::prepareToPlay(int /*samplesPerBlockExpected*/, double sampleRate)
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
	
	initObjects();
	
}

void StretchAudioSource::releaseResources()
{
	
}

bool StretchAudioSource::isResampling()
{
    if (m_inputfile==nullptr || m_inputfile->info.samplerate==0)
        return false;
    return (int)m_outsr!=m_inputfile->info.samplerate;
}

std::vector<int> StretchAudioSource::getSpectrumProcessOrder()
{
	return m_specproc_order;
}

void StretchAudioSource::setSpectrumProcessOrder(std::vector<int> order)
{
	ScopedLock locker(m_cs);
	m_specproc_order = order;
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

ValueTree StretchAudioSource::getStateTree()
{
	ValueTree tree("stretchsourcestate");
	storeToTreeProperties(tree, nullptr, "pitch_shift", m_ppar.pitch_shift.cents, 
		"octaves_minus2", m_ppar.octave.om2,
		"octaves_minus1",m_ppar.octave.om1,
		"octave0",m_ppar.octave.o0,
		"octave_plus1",m_ppar.octave.o1,
		"octaves_plus15",m_ppar.octave.o15,
		"octaves_plus2",m_ppar.octave.o2);
	return tree;
}

void StretchAudioSource::setStateTree(ValueTree state)
{
	ScopedLock locker(m_cs);
	getFromTreeProperties(state, "pitch_shift", m_ppar.pitch_shift.cents,
		"octaves_minus2", m_ppar.octave.om2,
		"octaves_minus1", m_ppar.octave.om1,
		"octave0", m_ppar.octave.o0,
		"octave_plus1", m_ppar.octave.o1,
		"octaves_plus15", m_ppar.octave.o15,
		"octaves_plus2", m_ppar.octave.o2);
	for (int i = 0; i < m_stretchers.size(); ++i)
	{
		m_stretchers[i]->set_parameters(&m_ppar);
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
		m_inputfile->setLoopEnabled(b);
	}
}

void StretchAudioSource::setAudioBufferAsInputSource(AudioBuffer<float>* buf, int sr, int len)
{
	ScopedLock locker(m_cs);
	m_inputfile->setAudioBuffer(buf, sr, len);
	m_seekpos = 0.0;

	m_curfile = File();
	if (m_playrange.isEmpty())
		setPlayRange({ 0.0,1.0 }, true);
	++m_param_change_count;
}

void StretchAudioSource::setMainVolume(double decibels)
{
	if (decibels == m_main_volume)
		return;
	ScopedLock locker(m_cs);
	m_main_volume = jlimit(-144.0, 12.0, decibels);
	++m_param_change_count;
}

void StretchAudioSource::setLoopXFadeLength(double lenseconds)
{
	if (lenseconds == m_loopxfadelen)
		return;
	ScopedLock locker(m_cs);
	m_loopxfadelen = jlimit(0.0, 1.0, lenseconds);
	++m_param_change_count;
}

void StretchAudioSource::getNextAudioBlock(const AudioSourceChannelInfo & bufferToFill)
{
	ScopedLock locker(m_cs);
	if (m_stretchoutringbuf.available() > 0)
		m_output_has_begun = true;
	bool freezing = m_freezing;
	
    if (m_stretchers[0]->isFreezing() != freezing)
	{
		if (freezing == true && m_inputfile!=nullptr)
			m_freeze_pos = 1.0/m_inputfile->info.nsamples*m_inputfile->getCurrentPosition();
		for (auto& e : m_stretchers)
			e->set_freezing(m_freezing);
	}
    
	double maingain = Decibels::decibelsToGain(m_main_volume);
	if (m_vol_smoother.getTargetValue() != maingain)
		m_vol_smoother.setValue(maingain);
	FloatVectorOperations::disableDenormalisedNumberSupport();
	float** outarrays = bufferToFill.buffer->getArrayOfWritePointers();
	int outbufchans = m_num_outchans; // bufferToFill.buffer->getNumChannels();
	int offset = bufferToFill.startSample;
	if (m_stretchers.size() == 0)
		return;
	if (m_inputfile == nullptr)
		return;
	if (m_inputfile->info.nsamples == 0)
		return;
	m_inputfile->setXFadeLenSeconds(m_loopxfadelen);
    
	double silencethreshold = Decibels::decibelsToGain(-70.0);
	bool tempfirst = true;
    auto foofilepos0 = m_inputfile->getCurrentPosition();
	auto ringbuffilltask = [this](int framestoproduce)
	{
		while (m_stretchoutringbuf.available() < framestoproduce*m_num_outchans)
		{
			int readsize = 0;
			double in_pos = (double)m_inputfile->getCurrentPosition() / (double)m_inputfile->info.nsamples;
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
				readed = m_inputfile->readNextBlock(m_file_inbuf, readsize, m_num_outchans);
			}
			auto inbufptrs = m_file_inbuf.getArrayOfReadPointers();
			for (int ch = 0; ch < m_num_outchans; ++ch)
			{
				int inchantouse = ch;
				for (int i = 0; i < readed; i++)
				{
					m_inbufs[ch][i] = inbufptrs[inchantouse][i];
				}
			}
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
				REALTYPE onset_l = m_stretchers[i]->process(m_inbufs[i].data(), readed);
				onset_max = std::max(onset_max, onset_l);
			}
#endif
			for (int i = 0; i < m_stretchers.size(); ++i)
				m_stretchers[i]->here_is_onset(onset_max);
			int outbufsize = m_stretchers[0]->get_bufsize();
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
			Logger::writeToLog("Filling xfade buffer");
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
				Logger::writeToLog("Initing stretcher objects");
				initObjects();
			}
			m_xfadetask.state = 2;
		}
	};
	
	resamplertask();
	if (previousxfadestate == 1 && m_xfadetask.state == 2)
	{
		Logger::writeToLog("Rerunning resampler task");
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
	//if (m_inputfile->hasEnded())
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

String StretchAudioSource::setAudioFile(File file)
{
	ScopedLock locker(m_cs);
	if (m_inputfile->openAudioFile(file))
	{
		m_curfile = file;
		return String();
	}
	return "Could not open file";
}

File StretchAudioSource::getAudioFile()
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
	m_inputfile->seek(m_seekpos);
	
	m_firstbuffer = true;
	if (m_stretchoutringbuf.getSize() < m_num_outchans*m_process_fftsize)
	{
		int newsize = m_num_outchans*m_process_fftsize*2;
		//Logger::writeToLog("Resizing circular buffer to " + String(newsize));
		m_stretchoutringbuf.resize(newsize);
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
		m_stretchers[i]->m_spectrum_processes = m_specproc_order;
	}
	m_inbufs.resize(m_num_outchans);
	m_file_inbuf.setSize(m_num_outchans, 3 * inbufsize);
	int poolsize = m_stretchers[0]->get_max_bufsize();
	for (int i = 0; i<m_num_outchans; ++i)
		m_inbufs[i].resize(poolsize);
	
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
	}
}

void StretchAudioSource::setProcessParameters(ProcessParameters * pars)
{
	if (*pars == m_ppar)
		return;
	if (m_cs.tryEnter())
	{
		m_ppar = *pars;
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
	ScopedLock locker(m_cs);
    m_fft_window_type = windowtype;
    for (int i = 0; i < m_stretchers.size(); ++i)
    {
        m_stretchers[i]->window_type = (FFTWindow)windowtype;
    }
}

void StretchAudioSource::setFFTSize(int size)
{
    jassert(size>0);
    if (m_xfadetask.state == 0 && (m_process_fftsize == 0 || size != m_process_fftsize))
	{
		ScopedLock locker(m_cs);
		if (m_xfadetask.buffer.getNumChannels() < m_num_outchans)
		{
			m_xfadetask.buffer.setSize(m_num_outchans, m_xfadetask.buffer.getNumSamples());
			
		}
		if (m_process_fftsize > 0)
		{
			m_xfadetask.state = 1;
			m_xfadetask.counter = 0;
			m_xfadetask.xfade_len = 44100;
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

void StretchAudioSource::seekPercent(double pos)
{
	ScopedLock locker(m_cs);
	m_seekpos = pos;
	m_inputfile->seek(pos);
	++m_param_change_count;
}

double StretchAudioSource::getOutputDurationSecondsForRange(Range<double> range, int fftsize)
{
	if (m_inputfile == nullptr || m_inputfile->info.nsamples == 0)
		return 0.0;
	int64_t play_end_pos = (fftsize * 2)+range.getLength()*m_playrate*m_inputfile->info.nsamples;
	return (double)play_end_pos / m_inputfile->info.samplerate;
}

void StretchAudioSource::setOnsetDetection(double x)
{
	ScopedLock locker(m_cs);
	m_onsetdetection = x;
	for (int i = 0; i < m_stretchers.size(); ++i)
	{
		m_stretchers[i]->set_onset_detection_sensitivity((float)x);
	}
	++m_param_change_count;
}

void StretchAudioSource::setPlayRange(Range<double> playrange, bool isloop)
{
	if (m_playrange.isEmpty() == false && playrange == m_playrange)
		return;
	if (m_cs.tryEnter())
	{
		if (playrange.isEmpty())
			m_playrange = { 0.0,1.0 };
		else
			m_playrange = playrange;
		m_stream_end_reached = false;
		m_inputfile->setActiveRange(m_playrange);
		m_inputfile->setLoopEnabled(isloop);
		if (m_playrange.contains(m_seekpos) == false)
			m_inputfile->seek(m_playrange.getStart());
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

std::pair<Range<double>, Range<double>> MultiStretchAudioSource::getFileCachedRangesNormalized()
{
    return getActiveStretchSource()->getFileCachedRangesNormalized();
}

void MultiStretchAudioSource::setAudioBufferAsInputSource(AudioBuffer<float>* buf, int sr, int len)
{
	m_stretchsources[0]->setAudioBufferAsInputSource(buf, sr, len);
	m_stretchsources[1]->setAudioBufferAsInputSource(buf, sr, len);
}

StretchAudioSource * MultiStretchAudioSource::getActiveStretchSource() const
{
	return m_stretchsources[0].get();
}

void MultiStretchAudioSource::switchActiveSource()
{
	std::swap(m_stretchsources[0], m_stretchsources[1]);
	m_is_in_switch = true;
	m_xfadegain.reset(m_samplerate, 2.0);
	m_xfadegain.setValue(1.0);
}

MultiStretchAudioSource::MultiStretchAudioSource(int initialnumoutchans, AudioFormatManager* afm)
	: m_afm(afm)
{
	m_stretchsources.resize(2);
	m_stretchsources[0] = std::make_shared<StretchAudioSource>(initialnumoutchans,m_afm);
	m_stretchsources[1] = std::make_shared<StretchAudioSource>(initialnumoutchans,m_afm);
	m_numoutchans = initialnumoutchans;
	m_processbuffers[0].setSize(m_numoutchans, 4096);
	m_processbuffers[1].setSize(m_numoutchans, 4096);
}

MultiStretchAudioSource::~MultiStretchAudioSource()
{
	
}

void MultiStretchAudioSource::prepareToPlay(int samplesPerBlockExpected, double sampleRate)
{
    m_is_in_switch = false;
    m_is_playing = true;
	m_blocksize = samplesPerBlockExpected;
	m_samplerate = sampleRate;
	if (m_processbuffers[0].getNumSamples() < samplesPerBlockExpected)
	{
		m_processbuffers[0].setSize(m_numoutchans, samplesPerBlockExpected);
		m_processbuffers[1].setSize(m_numoutchans, samplesPerBlockExpected);
	}
	getActiveStretchSource()->prepareToPlay(samplesPerBlockExpected, sampleRate);
    
}

void MultiStretchAudioSource::releaseResources()
{
	m_is_playing = false;
	getActiveStretchSource()->releaseResources();
}

void MultiStretchAudioSource::getNextAudioBlock(const AudioSourceChannelInfo & bufferToFill)
{
	std::lock_guard<std::mutex> locker(m_mutex);
	m_blocksize = bufferToFill.numSamples;
	if (m_is_in_switch == false)
	{
		getActiveStretchSource()->setMainVolume(val_MainVolume.getValue());
        getActiveStretchSource()->setLoopXFadeLength(val_XFadeLen.getValue());
		getActiveStretchSource()->setFreezing(m_freezing);
        getActiveStretchSource()->getNextAudioBlock(bufferToFill);
        
	}
	else
	{
		//if (bufferToFill.numSamples > m_processbuffers[0].getNumSamples())
		{
			m_processbuffers[0].setSize(m_numoutchans, bufferToFill.numSamples);
			m_processbuffers[1].setSize(m_numoutchans, bufferToFill.numSamples);
		}
		AudioSourceChannelInfo ascinfo1(m_processbuffers[0]);
		AudioSourceChannelInfo ascinfo2(m_processbuffers[1]);
		m_stretchsources[0]->setMainVolume(val_MainVolume.getValue());
		m_stretchsources[1]->setMainVolume(val_MainVolume.getValue());
        m_stretchsources[0]->setLoopXFadeLength(val_XFadeLen.getValue());
        m_stretchsources[1]->setLoopXFadeLength(val_XFadeLen.getValue());
		m_stretchsources[0]->setFreezing(m_freezing);
		m_stretchsources[1]->setFreezing(m_freezing);
		m_stretchsources[1]->setFFTWindowingType(m_stretchsources[0]->getFFTWindowingType());
        m_stretchsources[0]->getNextAudioBlock(ascinfo1);
		m_stretchsources[1]->getNextAudioBlock(ascinfo2);
		int offset = bufferToFill.startSample;
		float** outbufpts = bufferToFill.buffer->getArrayOfWritePointers();
		for (int i = 0; i < bufferToFill.numSamples; ++i)
		{
			double fadegain = m_xfadegain.getNextValue();
			for (int j = 0; j < m_numoutchans; ++j)
			{
				double procsample0 = (1.0-fadegain)*m_processbuffers[0].getSample(j, i);
				double procsample1 = (fadegain)*m_processbuffers[1].getSample(j, i);
				outbufpts[j][i + offset] = procsample0 + procsample1;
			}
		}
		if (m_xfadegain.isSmoothing() == false)
		{
			std::swap(m_stretchsources[0], m_stretchsources[1]);
			m_xfadegain.setValue(0.0);
			m_xfadegain.reset(m_samplerate, m_switchxfadelen);
            m_is_in_switch = false;
		}
	}
}

void MultiStretchAudioSource::setNextReadPosition(int64 newPosition)
{
	getActiveStretchSource()->setNextReadPosition(newPosition);
}

int64 MultiStretchAudioSource::getNextReadPosition() const
{
	return getActiveStretchSource()->getNextReadPosition();
}

int64 MultiStretchAudioSource::getTotalLength() const
{
	return getActiveStretchSource()->getTotalLength();
}

bool MultiStretchAudioSource::isLooping() const
{
	return getActiveStretchSource()->isLooping();
}

String MultiStretchAudioSource::setAudioFile(File file)
{
	if (m_is_playing == false)
	{
		return m_stretchsources[0]->setAudioFile(file);
	}
	else
	{
		String result = m_stretchsources[1]->setAudioFile(file);
		m_stretchsources[1]->setFFTSize(m_stretchsources[0]->getFFTSize());
		m_stretchsources[1]->setNumOutChannels(m_stretchsources[0]->getNumOutChannels());
		m_stretchsources[1]->setRate(m_stretchsources[0]->getRate());
		m_stretchsources[1]->setPlayRange({ 0.0,1.0 }, m_stretchsources[0]->isLoopEnabled());
		auto pars = m_stretchsources[0]->getProcessParameters();
		m_stretchsources[1]->setProcessParameters(&pars);
		m_stretchsources[1]->setSpectrumProcessOrder(m_stretchsources[0]->getSpectrumProcessOrder());
		m_stretchsources[1]->prepareToPlay(m_blocksize, m_samplerate);
		
		m_mutex.lock();
		m_xfadegain.reset(m_samplerate, m_switchxfadelen);
		m_xfadegain.setValue(1.0);
		m_is_in_switch = true;
		m_mutex.unlock();
		return result;
	}
	
}

File MultiStretchAudioSource::getAudioFile()
{
	return getActiveStretchSource()->getAudioFile();
}

void MultiStretchAudioSource::setNumOutChannels(int chans)
{
	m_numoutchans = chans;
	getActiveStretchSource()->setNumOutChannels(chans);
}

double MultiStretchAudioSource::getInfilePositionPercent()
{
	return getActiveStretchSource()->getInfilePositionPercent();
}

void MultiStretchAudioSource::setRate(double rate)
{
	getActiveStretchSource()->setRate(rate);
}

double MultiStretchAudioSource::getRate() 
{ 
	return getActiveStretchSource()->getRate();
}

void MultiStretchAudioSource::setProcessParameters(ProcessParameters * pars)
{
	getActiveStretchSource()->setProcessParameters(pars);
}

void MultiStretchAudioSource::setFFTWindowingType(int windowtype)
{
    getActiveStretchSource()->setFFTWindowingType(windowtype);
}

void MultiStretchAudioSource::setFFTSize(int size)
{
	if (size == getActiveStretchSource()->getFFTSize())
		return;
	if (m_is_playing == false)
	{
		getActiveStretchSource()->setFFTSize(size);
	}
	else
	{
		double curpos = m_stretchsources[0]->getInfilePositionPercent();
		m_stretchsources[1]->setFFTSize(size);
		m_stretchsources[1]->setNumOutChannels(m_stretchsources[0]->getNumOutChannels());
		if (m_stretchsources[0]->getAudioFile()!=File())
			m_stretchsources[1]->setAudioFile(m_stretchsources[0]->getAudioFile());
		m_stretchsources[1]->setRate(m_stretchsources[0]->getRate());
		m_stretchsources[1]->setPlayRange(m_stretchsources[0]->getPlayRange(), m_stretchsources[0]->isLoopEnabled());
		m_stretchsources[1]->seekPercent(curpos);
		auto pars = m_stretchsources[0]->getProcessParameters();
		m_stretchsources[1]->setProcessParameters(&pars);
		m_stretchsources[1]->setSpectrumProcessOrder(m_stretchsources[0]->getSpectrumProcessOrder());
		m_stretchsources[1]->prepareToPlay(m_blocksize, m_samplerate);
		m_mutex.lock();
		m_xfadegain.reset(m_samplerate, m_switchxfadelen);
		m_xfadegain.setValue(1.0);
		m_is_in_switch = true;
		m_mutex.unlock();
	}
}

int MultiStretchAudioSource::getFFTSize() 
{ 
	return getActiveStretchSource()->getFFTSize();
}


void MultiStretchAudioSource::seekPercent(double pos)
{
	getActiveStretchSource()->seekPercent(pos);
}

double MultiStretchAudioSource::getInfilePositionSeconds()
{
	return getActiveStretchSource()->getInfilePositionSeconds();
}

double MultiStretchAudioSource::getInfileLengthSeconds()
{
	return getActiveStretchSource()->getInfileLengthSeconds();
}

double MultiStretchAudioSource::getOutputDurationSecondsForRange(Range<double> range, int fftsize)
{
	return getActiveStretchSource()->getOutputDurationSecondsForRange(range, fftsize);
}

void MultiStretchAudioSource::setOnsetDetection(double x)
{
	getActiveStretchSource()->setOnsetDetection(x);
}

void MultiStretchAudioSource::setPlayRange(Range<double> playrange, bool isloop)
{
	getActiveStretchSource()->setPlayRange(playrange, isloop);
}

bool MultiStretchAudioSource::isLoopingEnabled()
{
	return getActiveStretchSource()->isLoopingEnabled();
}

void MultiStretchAudioSource::setLoopingEnabled(bool b)
{
	getActiveStretchSource()->setLoopingEnabled(b);
}

bool MultiStretchAudioSource::hasReachedEnd()
{
	return getActiveStretchSource()->hasReachedEnd();
}

bool MultiStretchAudioSource::isResampling()
{
	return getActiveStretchSource()->isResampling();
}

std::vector<int> MultiStretchAudioSource::getSpectrumProcessOrder()
{
	return getActiveStretchSource()->getSpectrumProcessOrder();
}

void MultiStretchAudioSource::setSpectrumProcessOrder(std::vector<int> order)
{
	getActiveStretchSource()->setSpectrumProcessOrder(order);
}
