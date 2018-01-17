/*
Copyright (C) 2006-2011 Nasca Octavian Paul
Author: Nasca Octavian Paul

Copyright (C) 2017 Xenakios

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

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <set>

#ifdef WIN32
#undef min
#undef max
#endif

String g_plugintitle{ "PaulXStretch 1.0.0 preview 5" };

std::set<PaulstretchpluginAudioProcessor*> g_activeprocessors;

struct PresetEntry
{
	PresetEntry(String name, String data) : m_name(name), m_data(data) {}
	String m_name;
	String m_data;
};

const int g_num_presets = 4;

static const PresetEntry g_presets[g_num_presets] = 
{ 
	{"Factory reset","cGF1bHN0cmV0Y2gzcGx1Z2luc3RhdGUAASZtYWludm9sdW1lMAABCQQAAACAi1cewHN0cmV0Y2hhbW91bnQwAAEJBAAAACAAAPA/ZmZ0c2l6ZTAAAQkEAAAAYGZm5j9waXRjaHNoaWZ0MAABCQQAAAAAAAAAAGZyZXFzaGlmdDAAAQkEAAAAAAAAAABwbGF5cmFuZ2Vfc3RhcnQwAAEJBAAAAAAAAAAAcGxheXJhbmdlX2VuZDAAAQkEAAAAAAAA8D9zcHJlYWQwAAEJBAAAAAAAAAAAY29tcHJlc3MwAAEJBAAAAAAAAAAAbG9vcHhmYWRlbGVuMAABCQQAAABA4XqEP251bWhhcm1vbmljczAAAQkEAAAAAABAWUBoYXJtb25pY3NmcmVxMAABCQQAAAAAAABgQGhhcm1vbmljc2J3MAABCQQAAAAAAAA5QG9jdGF2ZW1peG0yXzAAAQkEAAAAAAAAAABvY3RhdmVtaXhtMV8wAAEJBAAAAAAAAAAAb2N0YXZlbWl4MF8wAAEJBAAAAAAAAPA/b2N0YXZlbWl4MV8wAAEJBAAAAAAAAAAAb2N0YXZlbWl4MTVfMAABCQQAAAAAAAAAAG9jdGF2ZW1peDJfMAABCQQAAAAAAAAAAHRvbmFsdnNub2lzZWJ3XzAAAQkEAAAAgBSu5z90b25hbHZzbm9pc2VwcmVzZXJ2ZV8wAAEJBAAAAAAAAOA/ZmlsdGVyX2xvd18wAAEJBAAAAAAAADRAZmlsdGVyX2hpZ2hfMAABCQQAAAAAAIjTQG9uc2V0ZGV0ZWN0XzAAAQkEAAAAAAAAAABtYXhjYXB0dXJlbGVuXzAAAQkEAAAAAAAAJEBudW1vdXRjaGFuczAAAQUBAgAAAGltcG9ydGVkZmlsZQABKAVDOlxNdXNpY0F1ZGlvXHNvdXJjZXNhbXBsZXNcc2hlaWxhLndhdgBudW1zcGVjdHJhbHN0YWdlcwABBQEIAAAAc3BlY29yZGVyMAABBQEDAAAAc3BlY29yZGVyMQABBQEAAAAAc3BlY29yZGVyMgABBQEBAAAAc3BlY29yZGVyMwABBQECAAAAc3BlY29yZGVyNAABBQEEAAAAc3BlY29yZGVyNQABBQEFAAAAc3BlY29yZGVyNgABBQEGAAAAc3BlY29yZGVyNwABBQEHAAAAcHJlYnVmYW1vdW50AAEFAQIAAABsb2FkZmlsZXdpdGhzdGF0ZQABAQIAzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3NzQ=="},
	{"Chipmunk","cGF1bHN0cmV0Y2gzcGx1Z2luc3RhdGUAASZtYWludm9sdW1lMAABCQQAAACAi1cewHN0cmV0Y2hhbW91bnQwAAEJBAAAAAAAAOA/ZmZ0c2l6ZTAAAQkEAAAAAAAA4D9waXRjaHNoaWZ0MAABCQQAAAAAAAAoQGZyZXFzaGlmdDAAAQkEAAAAAAAAAABwbGF5cmFuZ2Vfc3RhcnQwAAEJBAAAAAAAAAAAcGxheXJhbmdlX2VuZDAAAQkEAAAAAAAA8D9zcHJlYWQwAAEJBAAAAAAAAAAAY29tcHJlc3MwAAEJBAAAAAAAAAAAbG9vcHhmYWRlbGVuMAABCQQAAABA4XqUP251bWhhcm1vbmljczAAAQkEAAAAAABAWUBoYXJtb25pY3NmcmVxMAABCQQAAAAAAABgQGhhcm1vbmljc2J3MAABCQQAAAAAAAA5QG9jdGF2ZW1peG0yXzAAAQkEAAAAAAAAAABvY3RhdmVtaXhtMV8wAAEJBAAAAAAAAAAAb2N0YXZlbWl4MF8wAAEJBAAAAAAAAPA/b2N0YXZlbWl4MV8wAAEJBAAAAAAAAAAAb2N0YXZlbWl4MTVfMAABCQQAAAAAAAAAAG9jdGF2ZW1peDJfMAABCQQAAAAAAAAAAHRvbmFsdnNub2lzZWJ3XzAAAQkEAAAAgBSu5z90b25hbHZzbm9pc2VwcmVzZXJ2ZV8wAAEJBAAAAAAAAOA/ZmlsdGVyX2xvd18wAAEJBAAAAAAAADRAZmlsdGVyX2hpZ2hfMAABCQQAAAAAAIjTQG9uc2V0ZGV0ZWN0XzAAAQkEAAAAAAAAAABtYXhjYXB0dXJlbGVuXzAAAQkEAAAAAAAAJEBudW1vdXRjaGFuczAAAQUBAgAAAGltcG9ydGVkZmlsZQABKAVDOlxNdXNpY0F1ZGlvXHNvdXJjZXNhbXBsZXNcc2hlaWxhLndhdgBudW1zcGVjdHJhbHN0YWdlcwABBQEIAAAAc3BlY29yZGVyMAABBQEAAAAAc3BlY29yZGVyMQABBQEBAAAAc3BlY29yZGVyMgABBQECAAAAc3BlY29yZGVyMwABBQEDAAAAc3BlY29yZGVyNAABBQEEAAAAc3BlY29yZGVyNQABBQEFAAAAc3BlY29yZGVyNgABBQEGAAAAc3BlY29yZGVyNwABBQEHAAAAcHJlYnVmYW1vdW50AAEFAQIAAABsb2FkZmlsZXdpdGhzdGF0ZQABAQIAzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3NzQ=="},
	{"Chipmunk harmonic series","cGF1bHN0cmV0Y2gzcGx1Z2luc3RhdGUAASZtYWludm9sdW1lMAABCQQAAACAi1cewHN0cmV0Y2hhbW91bnQwAAEJBAAAAAAAAOA/ZmZ0c2l6ZTAAAQkEAAAAoJmZ2T9waXRjaHNoaWZ0MAABCQQAAAAAAAAoQGZyZXFzaGlmdDAAAQkEAAAAAAAAAABwbGF5cmFuZ2Vfc3RhcnQwAAEJBAAAAAAAAAAAcGxheXJhbmdlX2VuZDAAAQkEAAAAAAAA8D9zcHJlYWQwAAEJBAAAAAAAAAAAY29tcHJlc3MwAAEJBAAAAAAAAAAAbG9vcHhmYWRlbGVuMAABCQQAAABA4XqUP251bWhhcm1vbmljczAAAQkEAAAAQMTsSkBoYXJtb25pY3NmcmVxMAABCQQAAAAAAABQQGhhcm1vbmljc2J3MAABCQQAAAAAAAA5QG9jdGF2ZW1peG0yXzAAAQkEAAAAAAAAAABvY3RhdmVtaXhtMV8wAAEJBAAAAAAAAAAAb2N0YXZlbWl4MF8wAAEJBAAAAAAAAPA/b2N0YXZlbWl4MV8wAAEJBAAAAAAAAAAAb2N0YXZlbWl4MTVfMAABCQQAAAAAAAAAAG9jdGF2ZW1peDJfMAABCQQAAAAAAAAAAHRvbmFsdnNub2lzZWJ3XzAAAQkEAAAAgBSu5z90b25hbHZzbm9pc2VwcmVzZXJ2ZV8wAAEJBAAAAAAAAOA/ZmlsdGVyX2xvd18wAAEJBAAAAAAAADRAZmlsdGVyX2hpZ2hfMAABCQQAAAAAAIjTQG9uc2V0ZGV0ZWN0XzAAAQkEAAAAAAAAAABtYXhjYXB0dXJlbGVuXzAAAQkEAAAAAAAAJEBudW1vdXRjaGFuczAAAQUBAgAAAGltcG9ydGVkZmlsZQABKAVDOlxNdXNpY0F1ZGlvXHNvdXJjZXNhbXBsZXNcc2hlaWxhLndhdgBudW1zcGVjdHJhbHN0YWdlcwABBQEIAAAAc3BlY29yZGVyMAABBQEDAAAAc3BlY29yZGVyMQABBQEAAAAAc3BlY29yZGVyMgABBQEBAAAAc3BlY29yZGVyMwABBQECAAAAc3BlY29yZGVyNAABBQEEAAAAc3BlY29yZGVyNQABBQEFAAAAc3BlY29yZGVyNgABBQEGAAAAc3BlY29yZGVyNwABBQEHAAAAcHJlYnVmYW1vdW50AAEFAQIAAABsb2FkZmlsZXdpdGhzdGF0ZQABAQIAzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3NzQ=="},
	{"Dark noise","cGF1bHN0cmV0Y2gzcGx1Z2luc3RhdGUAASNtYWludm9sdW1lMAABCQQAAACAi1cewHN0cmV0Y2hhbW91bnQwAAEJBAAAAGAAACBAZmZ0c2l6ZTAAAQkEAAAAwMzM7D9waXRjaHNoaWZ0MAABCQQAAAAAAAAAAGZyZXFzaGlmdDAAAQkEAAAAAAAAAABwbGF5cmFuZ2Vfc3RhcnQwAAEJBAAAAAAAAAAAcGxheXJhbmdlX2VuZDAAAQkEAAAAAAAA8D9zcHJlYWQwAAEJBAAAAAAAAPA/Y29tcHJlc3MwAAEJBAAAAKAONdg/bG9vcHhmYWRlbGVuMAABCQQAAABA4XqEP251bWhhcm1vbmljczAAAQkEAAAAAABAWUBoYXJtb25pY3NmcmVxMAABCQQAAAAAAABgQGhhcm1vbmljc2J3MAABCQQAAAAAAAA5QG9jdGF2ZW1peG0yXzAAAQkEAAAAAAAAAABvY3RhdmVtaXhtMV8wAAEJBAAAAAAAAAAAb2N0YXZlbWl4MF8wAAEJBAAAAAAAAPA/b2N0YXZlbWl4MV8wAAEJBAAAAAAAAAAAb2N0YXZlbWl4MTVfMAABCQQAAAAAAAAAAG9jdGF2ZW1peDJfMAABCQQAAAAAAAAAAHRvbmFsdnNub2lzZWJ3XzAAAQkEAAAAgBSu5z90b25hbHZzbm9pc2VwcmVzZXJ2ZV8wAAEJBAAAAAAAAOA/ZmlsdGVyX2xvd18wAAEJBAAAAAAAADRAZmlsdGVyX2hpZ2hfMAABCQQAAABgAHCXQG9uc2V0ZGV0ZWN0XzAAAQkEAAAAAAAAAABtYXhjYXB0dXJlbGVuXzAAAQkEAAAAAAAAJEBudW1vdXRjaGFuczAAAQUBAgAAAG51bXNwZWN0cmFsc3RhZ2VzAAEFAQgAAABzcGVjb3JkZXIwAAEFAQMAAABzcGVjb3JkZXIxAAEFAQAAAABzcGVjb3JkZXIyAAEFAQEAAABzcGVjb3JkZXIzAAEFAQIAAABzcGVjb3JkZXI0AAEFAQQAAABzcGVjb3JkZXI1AAEFAQUAAABzcGVjb3JkZXI2AAEFAQYAAABzcGVjb3JkZXI3AAEFAQcAAAAAzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3Nzc3N"}
};

int get_optimized_updown(int n, bool up) {
	int orig_n = n;
	while (true) {
		n = orig_n;

		while (!(n % 11)) n /= 11;
		while (!(n % 7)) n /= 7;

		while (!(n % 5)) n /= 5;
		while (!(n % 3)) n /= 3;
		while (!(n % 2)) n /= 2;
		if (n<2) break;
		if (up) orig_n++;
		else orig_n--;
		if (orig_n<4) return 4;
	};
	return orig_n;
};

int optimizebufsize(int n) {
	int n1 = get_optimized_updown(n, false);
	int n2 = get_optimized_updown(n, true);
	if ((n - n1)<(n2 - n)) return n1;
	else return n2;
};

//==============================================================================
PaulstretchpluginAudioProcessor::PaulstretchpluginAudioProcessor()
	: m_bufferingthread("pspluginprebufferthread")
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
{
	
    g_activeprocessors.insert(this);
	
	m_recbuffer.setSize(2, 44100);
	m_recbuffer.clear();
	if (m_afm->getNumKnownFormats()==0)
		m_afm->registerBasicFormats();
	m_thumb = std::make_unique<AudioThumbnail>(512, *m_afm, *m_thumbcache);
	//m_thumb->addChangeListener(this);
	m_stretch_source = std::make_unique<StretchAudioSource>(2, m_afm);
	
	
	m_ppar.pitch_shift.enabled = true;
	m_ppar.freq_shift.enabled = true;
	m_ppar.filter.enabled = true;
    m_ppar.compressor.enabled = true;
	m_stretch_source->setOnsetDetection(0.0);
	m_stretch_source->setLoopingEnabled(true);
	m_stretch_source->setFFTWindowingType(1);
	addParameter(new AudioParameterFloat("mainvolume0", "Main volume", -24.0f, 12.0f, -3.0f)); // 0
	addParameter(new AudioParameterFloat("stretchamount0", "Stretch amount", 
		NormalisableRange<float>(0.1f, 1024.0f, 0.01f, 0.25),1.0f)); // 1
	addParameter(new AudioParameterFloat("fftsize0", "FFT size", 0.0f, 1.0f, 0.7f)); // 2
	addParameter(new AudioParameterFloat("pitchshift0", "Pitch shift", -24.0f, 24.0f, 0.0f)); // 3
	addParameter(new AudioParameterFloat("freqshift0", "Frequency shift", -1000.0f, 1000.0f, 0.0f)); // 4
	addParameter(new AudioParameterFloat("playrange_start0", "Sound start", 0.0f, 1.0f, 0.0f)); // 5
	addParameter(new AudioParameterFloat("playrange_end0", "Sound end", 0.0f, 1.0f, 1.0f)); // 6
	addParameter(new AudioParameterBool("freeze0", "Freeze", false)); // 7
	addParameter(new AudioParameterFloat("spread0", "Frequency spread", 0.0f, 1.0f, 0.0f)); // 8
	addParameter(new AudioParameterFloat("compress0", "Compress", 0.0f, 1.0f, 0.0f)); // 9
	addParameter(new AudioParameterFloat("loopxfadelen0", "Loop xfade length", 0.0f, 1.0f, 0.01f)); // 10
    auto numhar_convertFrom0To1Func = [](float rangemin, float rangemax, float value)
    {
        return jmap<float>(value, 0.0f, 1.0f, 101.0f, 1.0f);
    };
    auto numhar_convertTo0To1Func = [](float rangemin, float rangemax, float value)
    {
        return jmap<float>(value, 101.0f, 1.0f, 0.0f, 1.0f);
    };
    addParameter(new AudioParameterFloat("numharmonics0", "Num harmonics",
                                         NormalisableRange<float>(1.0f, 101.0f,
                                        numhar_convertFrom0To1Func, numhar_convertTo0To1Func), 101.0f)); // 11
	addParameter(new AudioParameterFloat("harmonicsfreq0", "Harmonics base freq", 
		NormalisableRange<float>(1.0f, 5000.0f, 1.00f, 0.5), 128.0f)); // 12
	addParameter(new AudioParameterFloat("harmonicsbw0", "Harmonics bandwidth", 0.1f, 200.0f, 25.0f)); // 13
	addParameter(new AudioParameterBool("harmonicsgauss0", "Gaussian harmonics", false)); // 14
	addParameter(new AudioParameterFloat("octavemixm2_0", "2 octaves down level", 0.0f, 1.0f, 0.0f)); // 15
	addParameter(new AudioParameterFloat("octavemixm1_0", "Octave down level", 0.0f, 1.0f, 0.0f)); // 16
	addParameter(new AudioParameterFloat("octavemix0_0", "Normal pitch level", 0.0f, 1.0f, 1.0f)); // 17
	addParameter(new AudioParameterFloat("octavemix1_0", "1 octave up level", 0.0f, 1.0f, 0.0f)); // 18
	addParameter(new AudioParameterFloat("octavemix15_0", "1 octave and fifth up level", 0.0f, 1.0f, 0.0f)); // 19
	addParameter(new AudioParameterFloat("octavemix2_0", "2 octaves up level", 0.0f, 1.0f, 0.0f)); // 20
	addParameter(new AudioParameterFloat("tonalvsnoisebw_0", "Tonal vs Noise BW", 0.74f, 1.0f, 0.74f)); // 21
	addParameter(new AudioParameterFloat("tonalvsnoisepreserve_0", "Tonal vs Noise preserve", -1.0f, 1.0f, 0.5f)); // 22
	auto filt_convertFrom0To1Func = [](float rangemin, float rangemax, float value) 
	{
		if (value < 0.5f)
			return jmap<float>(value, 0.0f, 0.5f, 20.0f, 1000.0f);
		return jmap<float>(value, 0.5f, 1.0f, 1000.0f, 20000.0f);
	};
	auto filt_convertTo0To1Func = [](float rangemin, float rangemax, float value)
	{
		if (value < 1000.0f)
			return jmap<float>(value, 20.0f, 1000.0f, 0.0f, 0.5f);
		return jmap<float>(value, 1000.0f, 20000.0f, 0.5f, 1.0f);
	};
	addParameter(new AudioParameterFloat("filter_low_0", "Filter low",
                                         NormalisableRange<float>(20.0f, 20000.0f, 
											 filt_convertFrom0To1Func, filt_convertTo0To1Func), 20.0f)); // 23
	addParameter(new AudioParameterFloat("filter_high_0", "Filter high",
                                         NormalisableRange<float>(20.0f, 20000.0f, 
											 filt_convertFrom0To1Func,filt_convertTo0To1Func), 20000.0f));; // 24
	addParameter(new AudioParameterFloat("onsetdetect_0", "Onset detection", 0.0f, 1.0f, 0.0f)); // 25
	addParameter(new AudioParameterBool("capture_enabled0", "Capture", false)); // 26
	m_outchansparam = new AudioParameterInt("numoutchans0", "Num output channels", 2, 8, 2); // 27
	addParameter(m_outchansparam); // 27
	addParameter(new AudioParameterBool("pause_enabled0", "Pause", false)); // 28
	addParameter(new AudioParameterFloat("maxcapturelen_0", "Max capture length", 1.0f, 120.0f, 10.0f)); // 29
	addParameter(new AudioParameterBool("passthrough0", "Pass input through", false)); // 30
	auto& pars = getParameters();
	for (const auto& p : pars)
		m_reset_pars.push_back(p->getValue());
	setPreBufferAmount(2);
    startTimer(1, 50);
}

PaulstretchpluginAudioProcessor::~PaulstretchpluginAudioProcessor()
{
	g_activeprocessors.erase(this);
	m_bufferingthread.stopThread(1000);
}

void PaulstretchpluginAudioProcessor::resetParameters()
{
	ScopedLock locker(m_cs);
	for (int i = 0; i < m_reset_pars.size(); ++i)
	{
		if (i!=cpi_main_volume && i!=cpi_passthrough)
			setParameter(i, m_reset_pars[i]);
	}
}

void PaulstretchpluginAudioProcessor::setPreBufferAmount(int x)
{
	int temp = jlimit(0, 5, x);
	if (temp != m_prebuffer_amount || m_use_backgroundbuffering == false)
	{
        m_use_backgroundbuffering = true;
        m_prebuffer_amount = temp;
		m_recreate_buffering_source = true;
        ScopedLock locker(m_cs);
        m_prebuffering_inited = false;
        m_cur_num_out_chans = *m_outchansparam;
        //Logger::writeToLog("Switching to use " + String(m_cur_num_out_chans) + " out channels");
        String err;
        startplay({ *getFloatParameter(cpi_soundstart),*getFloatParameter(cpi_soundend) },
                  m_cur_num_out_chans, m_curmaxblocksize, err);
        m_prebuffering_inited = true;
	}
}

int PaulstretchpluginAudioProcessor::getPreBufferAmount()
{
	if (m_use_backgroundbuffering == false)
		return -1;
	return m_prebuffer_amount;
}

ValueTree PaulstretchpluginAudioProcessor::getStateTree(bool ignoreoptions, bool ignorefile)
{
	ValueTree paramtree("paulstretch3pluginstate");
	for (int i = 0; i<getNumParameters(); ++i)
	{
		auto par = getFloatParameter(i);
		if (par != nullptr)
		{
			paramtree.setProperty(par->paramID, (double)*par, nullptr);
		}
	}
	paramtree.setProperty(m_outchansparam->paramID, (int)*m_outchansparam, nullptr);
	if (m_current_file != File() && ignorefile == false)
	{
		paramtree.setProperty("importedfile", m_current_file.getFullPathName(), nullptr);
	}
	auto specorder = m_stretch_source->getSpectrumProcessOrder();
	paramtree.setProperty("numspectralstages", (int)specorder.size(), nullptr);
	for (int i = 0; i < specorder.size(); ++i)
	{
		paramtree.setProperty("specorder" + String(i), specorder[i], nullptr);
	}
	if (ignoreoptions == false)
	{
		if (m_use_backgroundbuffering)
			paramtree.setProperty("prebufamount", m_prebuffer_amount, nullptr);
		else
			paramtree.setProperty("prebufamount", -1, nullptr);
		paramtree.setProperty("loadfilewithstate", m_load_file_with_state, nullptr);
	}
	return paramtree;
}

void PaulstretchpluginAudioProcessor::setStateFromTree(ValueTree tree)
{
	if (tree.isValid())
	{
		{
			ScopedLock locker(m_cs);
			m_load_file_with_state = tree.getProperty("loadfilewithstate", true);
			if (tree.hasProperty("numspectralstages"))
			{
				std::vector<int> order;
				int ordersize = tree.getProperty("numspectralstages");
				for (int i = 0; i < ordersize; ++i)
				{
					order.push_back((int)tree.getProperty("specorder" + String(i)));
				}
				m_stretch_source->setSpectrumProcessOrder(order);
			}
			for (int i = 0; i < getNumParameters(); ++i)
			{
				auto par = getFloatParameter(i);
				if (par != nullptr)
				{
					double parval = tree.getProperty(par->paramID, (double)*par);
					*par = parval;
				}
			}
			if (tree.hasProperty(m_outchansparam->paramID))
				*m_outchansparam = tree.getProperty(m_outchansparam->paramID, 2);

		}
		int prebufamt = tree.getProperty("prebufamount", 2);
		if (prebufamt == -1)
			m_use_backgroundbuffering = false;
		else
			setPreBufferAmount(prebufamt);
		if (m_load_file_with_state == true)
		{
			String fn = tree.getProperty("importedfile");
			if (fn.isEmpty() == false)
			{
				File f(fn);
				setAudioFile(f);
			}
		}
		m_state_dirty = true;
	}
}

//==============================================================================
const String PaulstretchpluginAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool PaulstretchpluginAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool PaulstretchpluginAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool PaulstretchpluginAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double PaulstretchpluginAudioProcessor::getTailLengthSeconds() const
{
	return 0.0;
	//return (double)m_bufamounts[m_prebuffer_amount]/getSampleRate();
}

int PaulstretchpluginAudioProcessor::getNumPrograms()
{
	return g_num_presets;
}

int PaulstretchpluginAudioProcessor::getCurrentProgram()
{
    return m_cur_program;
}

void PaulstretchpluginAudioProcessor::setCurrentProgram (int index)
{
	index = jlimit(0, g_num_presets-1, index);
	m_cur_program = index;
	bool temp = m_load_file_with_state;
	m_load_file_with_state = false;
	MemoryBlock mb;
	MemoryOutputStream stream(mb, true);
	if (Base64::convertFromBase64(stream, g_presets[index].m_data)==true)
	{
		ValueTree tree = ValueTree::readFromData(mb.getData(), mb.getSize());
		tree.setProperty("loadfilewithstate", false, nullptr);
		setStateFromTree(tree);
	}
	m_load_file_with_state = temp;
}

const String PaulstretchpluginAudioProcessor::getProgramName (int index)
{
	index = jlimit(0, g_num_presets-1, index);
	return g_presets[index].m_name;
}

void PaulstretchpluginAudioProcessor::changeProgramName (int index, const String& newName)
{
}

void PaulstretchpluginAudioProcessor::setFFTSize(double size)
{
	if (m_prebuffer_amount == 5)
		m_fft_size_to_use = pow(2, 7.0 + size * 14.5);
	else m_fft_size_to_use = pow(2, 7.0 + size * 10.0); // chicken out from allowing huge FFT sizes if not enough prebuffering
	int optim = optimizebufsize(m_fft_size_to_use);
	m_fft_size_to_use = optim;
	m_stretch_source->setFFTSize(optim);
	//Logger::writeToLog(String(m_fft_size_to_use));
}

void PaulstretchpluginAudioProcessor::startplay(Range<double> playrange, int numoutchans, int maxBlockSize, String& err)
{
	m_stretch_source->setPlayRange(playrange, true);

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
	m_stretch_source->setProcessParameters(&m_ppar);
	m_last_outpos_pos = 0.0;
	m_last_in_pos = playrange.getStart()*m_stretch_source->getInfileLengthSeconds();
	m_buffering_source->prepareToPlay(maxBlockSize, getSampleRateChecked());
}

void PaulstretchpluginAudioProcessor::setParameters(const std::vector<double>& pars)
{
	ScopedLock locker(m_cs);
	for (int i = 0; i < getNumParameters(); ++i)
	{
		if (i<pars.size())
			setParameter(i, pars[i]);
	}
}

double PaulstretchpluginAudioProcessor::getSampleRateChecked()
{
	if (m_cur_sr < 1.0)
		return 44100.0;
	return m_cur_sr;
}

void PaulstretchpluginAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
	ScopedLock locker(m_cs);
	m_cur_sr = sampleRate;
	m_curmaxblocksize = samplesPerBlock;
	m_input_buffer.setSize(2, samplesPerBlock);
	int numoutchans = *m_outchansparam;
	if (numoutchans != m_cur_num_out_chans)
		m_prebuffering_inited = false;
	if (m_using_memory_buffer == true)
	{
		int len = jlimit(100,m_recbuffer.getNumSamples(), 
			int(getSampleRateChecked()*(*getFloatParameter(cpi_max_capture_len))));
		m_stretch_source->setAudioBufferAsInputSource(&m_recbuffer, 
			getSampleRateChecked(), 
			len);
		callGUI(this,[this,len](auto ed) { ed->setAudioBuffer(&m_recbuffer, getSampleRateChecked(), len); },false);
	}
	if (m_prebuffering_inited == false)
	{
		setFFTSize(*getFloatParameter(cpi_fftsize));
		m_stretch_source->setProcessParameters(&m_ppar);
		m_stretch_source->setFFTWindowingType(1);
		String err;
		startplay({ *getFloatParameter(cpi_soundstart),*getFloatParameter(cpi_soundend) },
		numoutchans, samplesPerBlock, err);
		m_cur_num_out_chans = numoutchans;
		m_prebuffering_inited = true;
	}
	else
	{
		m_buffering_source->prepareToPlay(samplesPerBlock, getSampleRateChecked());
	}
}

void PaulstretchpluginAudioProcessor::releaseResources()
{
	//m_control->stopplay();
	//m_ready_to_play = false;
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool PaulstretchpluginAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    if (layouts.getMainOutputChannelSet() != AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void copyAudioBufferWrappingPosition(const AudioBuffer<float>& src, AudioBuffer<float>& dest, int destbufpos, int maxdestpos)
{
	for (int i = 0; i < dest.getNumChannels(); ++i)
	{
		int channel_to_copy = i % src.getNumChannels();
		if (destbufpos + src.getNumSamples() > maxdestpos)
		{
			int wrappos = (destbufpos + src.getNumSamples()) % maxdestpos;
			int partial_len = src.getNumSamples() - wrappos;
			dest.copyFrom(channel_to_copy, destbufpos, src, channel_to_copy, 0, partial_len);
			dest.copyFrom(channel_to_copy, partial_len, src, channel_to_copy, 0, wrappos);
		}
		else
		{
			dest.copyFrom(channel_to_copy, destbufpos, src, channel_to_copy, 0, src.getNumSamples());
		}
	}
}

void PaulstretchpluginAudioProcessor::processBlock (AudioSampleBuffer& buffer, MidiBuffer& midiMessages)
{
	ScopedLock locker(m_cs);
	AudioPlayHead* phead = getPlayHead();
	if (phead != nullptr)
	{
		phead->getCurrentPosition(m_playposinfo);
	}
	ScopedNoDenormals noDenormals;
	double srtemp = getSampleRate();
	if (srtemp != m_cur_sr)
		m_cur_sr = srtemp;
    const int totalNumInputChannels  = getTotalNumInputChannels();
    const int totalNumOutputChannels = getTotalNumOutputChannels();
	for (int i = 0; i < totalNumInputChannels; ++i)
		m_input_buffer.copyFrom(i, 0, buffer, i, 0, buffer.getNumSamples());
    for (int i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());
	if (m_prebuffering_inited == false)
		return;
	if (m_is_recording == true)
	{
		if (m_playposinfo.isPlaying == false && m_capture_when_host_plays == true)
			return;
		int recbuflenframes = m_max_reclen * getSampleRate();
		copyAudioBufferWrappingPosition(buffer, m_recbuffer, m_rec_pos, recbuflenframes);
		callGUI(this,[this, &buffer](PaulstretchpluginAudioProcessorEditor*ed) 
		{
			ed->addAudioBlock(buffer, getSampleRate(), m_rec_pos);
		}, false);
		m_rec_pos = (m_rec_pos + buffer.getNumSamples()) % recbuflenframes;
		return;
	}
	jassert(m_buffering_source != nullptr);
	jassert(m_bufferingthread.isThreadRunning());
	if (m_last_host_playing == false && m_playposinfo.isPlaying)
	{
		m_stretch_source->seekPercent(*getFloatParameter(cpi_soundstart));
		m_last_host_playing = true;
	}
	else if (m_last_host_playing == true && m_playposinfo.isPlaying == false)
	{
		m_last_host_playing = false;
	}
	if (m_play_when_host_plays == true && m_playposinfo.isPlaying == false)
		return;
	m_stretch_source->setMainVolume(*getFloatParameter(cpi_main_volume));
	m_stretch_source->setRate(*getFloatParameter(cpi_stretchamount));

	setFFTSize(*getFloatParameter(cpi_fftsize));
	m_ppar.pitch_shift.cents = *getFloatParameter(cpi_pitchshift) * 100.0;
	m_ppar.freq_shift.Hz = *getFloatParameter(cpi_frequencyshift);
	m_ppar.spread.enabled = *getFloatParameter(cpi_spreadamount) > 0.0f;
	m_ppar.spread.bandwidth = *getFloatParameter(cpi_spreadamount);
    m_ppar.compressor.enabled = *getFloatParameter(cpi_compress)>0.0f;
    m_ppar.compressor.power = *getFloatParameter(cpi_compress);
	m_ppar.harmonics.enabled = *getFloatParameter(cpi_numharmonics)<101.0;
	m_ppar.harmonics.nharmonics = *getFloatParameter(cpi_numharmonics);
	m_ppar.harmonics.freq = *getFloatParameter(cpi_harmonicsfreq);
    m_ppar.harmonics.bandwidth = *getFloatParameter(cpi_harmonicsbw);
    m_ppar.harmonics.gauss = getParameter(cpi_harmonicsgauss);
	m_ppar.octave.om2 = *getFloatParameter(cpi_octavesm2);
	m_ppar.octave.om1 = *getFloatParameter(cpi_octavesm1);
	m_ppar.octave.o0 = *getFloatParameter(cpi_octaves0);
	m_ppar.octave.o1 = *getFloatParameter(cpi_octaves1);
	m_ppar.octave.o15 = *getFloatParameter(cpi_octaves15);
	m_ppar.octave.o2 = *getFloatParameter(cpi_octaves2);
	m_ppar.octave.enabled = true;
	m_ppar.filter.low = *getFloatParameter(cpi_filter_low);
	m_ppar.filter.high = *getFloatParameter(cpi_filter_high);
	m_ppar.tonal_vs_noise.enabled = (*getFloatParameter(cpi_tonalvsnoisebw)) > 0.75;
	m_ppar.tonal_vs_noise.bandwidth = *getFloatParameter(cpi_tonalvsnoisebw);
	m_ppar.tonal_vs_noise.preserve = *getFloatParameter(cpi_tonalvsnoisepreserve);
	m_stretch_source->setOnsetDetection(*getFloatParameter(cpi_onsetdetection));
	m_stretch_source->setLoopXFadeLength(*getFloatParameter(cpi_loopxfadelen));
	double t0 = *getFloatParameter(cpi_soundstart);
	double t1 = *getFloatParameter(cpi_soundend);
	if (t0 > t1)
		std::swap(t0, t1);
	if (t1 - t0 < 0.001)
		t1 = t0 + 0.001;
	m_stretch_source->setPlayRange({ t0,t1 }, true);
	m_stretch_source->setFreezing(getParameter(cpi_freeze));
	m_stretch_source->setPaused(getParameter(cpi_pause_enabled));
	m_stretch_source->setProcessParameters(&m_ppar);
	AudioSourceChannelInfo aif(buffer);
	if (isNonRealtime() || m_use_backgroundbuffering == false)
	{
		m_stretch_source->getNextAudioBlock(aif);
	}
	else
	{
		m_buffering_source->getNextAudioBlock(aif);
	}
	if (getParameter(cpi_passthrough) > 0.5f)
	{
		for (int i = 0; i < totalNumInputChannels; ++i)
		{
			buffer.addFrom(i, 0, m_input_buffer, i, 0, buffer.getNumSamples());
		}
	}
	for (int i = 0; i < buffer.getNumChannels(); ++i)
	{
		for (int j = 0; j < buffer.getNumSamples(); ++j)
		{
			float sample = buffer.getSample(i,j);
			if (std::isnan(sample) || std::isinf(sample))
				++m_abnormal_output_samples;
		}
	}
}

//==============================================================================
bool PaulstretchpluginAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

AudioProcessorEditor* PaulstretchpluginAudioProcessor::createEditor()
{
	return new PaulstretchpluginAudioProcessorEditor (*this);
}

//==============================================================================
void PaulstretchpluginAudioProcessor::getStateInformation (MemoryBlock& destData)
{
	ValueTree paramtree = getStateTree(false,false);
	MemoryOutputStream stream(destData,true);
	paramtree.writeToStream(stream);
}

void PaulstretchpluginAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
	ValueTree tree = ValueTree::readFromData(data, sizeInBytes);
	setStateFromTree(tree);
}

void PaulstretchpluginAudioProcessor::setRecordingEnabled(bool b)
{
	ScopedLock locker(m_cs);
	int lenbufframes = getSampleRateChecked()*m_max_reclen;
	if (b == true)
	{
		m_using_memory_buffer = true;
		m_current_file = File();
		m_recbuffer.setSize(2, m_max_reclen*getSampleRateChecked()+4096,false,false,true);
		m_recbuffer.clear();
		m_rec_pos = 0;
		callGUI(this,[this,lenbufframes](PaulstretchpluginAudioProcessorEditor* ed)
		{
			ed->beginAddingAudioBlocks(2, getSampleRateChecked(), lenbufframes);
		},false);
		m_is_recording = true;
	}
	else
	{
		if (m_is_recording == true)
		{
			finishRecording(lenbufframes);
		}
	}
}

double PaulstretchpluginAudioProcessor::getRecordingPositionPercent()
{
	if (m_is_recording==false)
		return 0.0;
	return 1.0 / m_recbuffer.getNumSamples()*m_rec_pos;
}

String PaulstretchpluginAudioProcessor::setAudioFile(File f)
{
    //if (f==File())
    //    return String();
    //if (f==m_current_file && f.getLastModificationTime()==m_current_file_date)
    //    return String();
    auto ai = unique_from_raw(m_afm->createReaderFor(f));
	if (ai != nullptr)
	{
		if (ai->numChannels > 32)
		{
			//MessageManager::callAsync([cb, file]() { cb("Too many channels in file " + file.getFullPathName()); });
			return "Too many channels in file "+f.getFullPathName();
		}
		if (ai->bitsPerSample>32)
		{
			//MessageManager::callAsync([cb, file]() { cb("Too high bit depth in file " + file.getFullPathName()); });
			return "Too high bit depth in file " + f.getFullPathName();
		}
		m_thumb->setSource(new FileInputSource(f));
		ScopedLock locker(m_cs);
		m_stretch_source->setAudioFile(f);
		m_stretch_source->seekPercent(*getFloatParameter(cpi_soundstart));
		m_current_file = f;
        m_current_file_date = m_current_file.getLastModificationTime();
		m_using_memory_buffer = false;
		return String();
		//MessageManager::callAsync([cb, file]() { cb(String()); });

	}
	
	return "Could not open file " + f.getFullPathName();
}

Range<double> PaulstretchpluginAudioProcessor::getTimeSelection()
{
	return { *getFloatParameter(cpi_soundstart),*getFloatParameter(cpi_soundend) };
}

double PaulstretchpluginAudioProcessor::getPreBufferingPercent()
{
	if (m_buffering_source==nullptr)
		return 0.0;
	return m_buffering_source->getPercentReady();
}

void PaulstretchpluginAudioProcessor::timerCallback(int id)
{
	if (id == 1)
	{
		bool capture = getParameter(cpi_capture_enabled);
		if (capture == false && m_max_reclen != *getFloatParameter(cpi_max_capture_len))
		{
			m_max_reclen = *getFloatParameter(cpi_max_capture_len);
			//Logger::writeToLog("Changing max capture len to " + String(m_max_reclen));
		}
		if (capture == true && m_is_recording == false)
		{
			setRecordingEnabled(true);
			return;
		}
		if (capture == false && m_is_recording == true)
		{
			setRecordingEnabled(false);
			return;
		}
		if (m_cur_num_out_chans != *m_outchansparam)
		{
			jassert(m_curmaxblocksize > 0);
			ScopedLock locker(m_cs);
			m_prebuffering_inited = false;
			m_cur_num_out_chans = *m_outchansparam;
			//Logger::writeToLog("Switching to use " + String(m_cur_num_out_chans) + " out channels");
			String err;
			startplay({ *getFloatParameter(cpi_soundstart),*getFloatParameter(cpi_soundend) },
				m_cur_num_out_chans, m_curmaxblocksize, err);
			m_prebuffering_inited = true;
		}
	}
}

void PaulstretchpluginAudioProcessor::finishRecording(int lenrecording)
{
	m_is_recording = false;
	m_stretch_source->setAudioBufferAsInputSource(&m_recbuffer, getSampleRateChecked(), lenrecording);
	m_stretch_source->setPlayRange({ *getFloatParameter(cpi_soundstart),*getFloatParameter(cpi_soundend) }, true);
	auto ed = dynamic_cast<PaulstretchpluginAudioProcessorEditor*>(getActiveEditor());
	if (ed)
	{
		//ed->setAudioBuffer(&m_recbuffer, getSampleRate(), lenrecording);
	}
}

//==============================================================================
// This creates new instances of the plugin..
AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PaulstretchpluginAudioProcessor();
}
