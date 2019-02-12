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

#include <vector>
#include <memory>
#include <set>
#include "../JuceLibraryCode/JuceHeader.h"

const String g_plugintitle{ "PaulXStretch 1.2.4" };

using REALTYPE = float;

using floatvector = std::vector<REALTYPE>;
using float2dvector = std::vector<std::vector<float>>;
using float3dvector = std::vector<std::vector<std::vector<float>>>;

template<typename T>
using uptrvec = std::vector<std::unique_ptr<T>>;

template<typename T>
using sptrvec = std::vector<std::shared_ptr<T>>;

template<typename T>
inline std::unique_ptr<T> unique_from_raw(T* ptr)
{
	return std::unique_ptr<T>(ptr);
}

template<typename T>
inline String toString(const T& x)
{
    return String(x);
}

inline String toString(double x)
{
    return String(x,3);
}


template<typename... Args>
inline String formatted(Args... args)
{
    String result;
    (result << ... << toString(args));
    return result;
}

#ifndef USEOLDPLAYCURSOR
#define USEOLDPLAYCURSOR
#endif

#ifndef NULL
#define NULL 0
#endif

const double c_PI = 3.14159265359;

const int g_maxnumoutchans = 32;

#ifndef USE_LUA_SCRIPTING
//#define USE_LUA_SCRIPTING
#endif

template<typename... Args>
inline bool hasProperties(ValueTree src, Args&&... args)
{
	if (sizeof...(args) == 0)
		return false;
	return (src.hasProperty(args) && ...);
}

template<typename... Ts>
inline void storeToTreeProperties(ValueTree dest, UndoManager* uman, juce::Identifier varname, var val, Ts&&... args)
{
	dest.setProperty(varname, val, uman);
	if constexpr(sizeof...(Ts)>1)
		storeToTreeProperties(dest, uman, args...);
}

template<typename T>
inline void storeToTreeProperties(ValueTree dest, UndoManager* uman, juce::Identifier varname, Range<T> x)
{
	dest.setProperty(varname+"_start", x.getStart(), uman);
	dest.setProperty(varname+"_end", x.getEnd(), uman);
}

inline void storeToTreeProperties(ValueTree dest, UndoManager* uman, AudioParameterFloat* par)
{
	if (par) dest.setProperty(par->paramID, (float)*par, uman);
}

inline void storeToTreeProperties(ValueTree dest, UndoManager* uman, AudioParameterBool* par)
{
    if (par) dest.setProperty(par->paramID,(bool)*par,uman);
}

inline void storeToTreeProperties(ValueTree dest, UndoManager* uman, AudioParameterInt* par)
{
    if (par) dest.setProperty(par->paramID,(int)*par,uman);
}

inline void storeToTreeProperties(ValueTree dest, UndoManager* uman, const OwnedArray<AudioProcessorParameter>& pars,
	const std::set<AudioProcessorParameter*>& ignorepars = {})
{
	for (auto& e : pars)
	{
		if (ignorepars.count(e))
			continue;
		auto parf = dynamic_cast<AudioParameterFloat*>(e);
		if (parf != nullptr)
			storeToTreeProperties(dest, nullptr, parf);
		auto pari = dynamic_cast<AudioParameterInt*>(e);
		if (pari != nullptr)
			storeToTreeProperties(dest, nullptr, pari);
		auto parb = dynamic_cast<AudioParameterBool*>(e);
		if (parb != nullptr)
			storeToTreeProperties(dest, nullptr, parb);
	}
}

template<typename... Ts, typename T>
inline void getFromTreeProperties(ValueTree src, juce::Identifier varname, T& val, Ts&... args)
{
	if (src.hasProperty(varname))
		val = src.getProperty(varname);
	if constexpr(sizeof...(Ts)>1)
		getFromTreeProperties(src, args...);
}

template<typename T>
inline void getFromTreeProperties(ValueTree src, juce::Identifier varname, Range<T>& rng)
{
	if (hasProperties(src, varname + "_start", varname + "_end"))
	{
		rng = { src.getProperty(varname + "_start") , src.getProperty(varname + "_end") };
	}
	else
	{
		jassert(false);
	}
}

template<typename T>
inline void getFromTreeProperties(ValueTree src, T par)
{
	static_assert(std::is_base_of<AudioProcessorParameterWithID,typename std::remove_pointer<T>::type>::value,
		"T must inherit from AudioProcessorParameterWithID");
	if (par!=nullptr && src.hasProperty(par->paramID))
    {
        *par = src.getProperty(par->paramID);
    }
}

inline void getFromTreeProperties(ValueTree src, const OwnedArray<AudioProcessorParameter>& pars)
{
	for (auto& e : pars)
	{
		auto parf = dynamic_cast<AudioParameterFloat*>(e);
		if (parf != nullptr && src.hasProperty(parf->paramID))
			*parf = src.getProperty(parf->paramID);
		auto pari = dynamic_cast<AudioParameterInt*>(e);
		if (pari != nullptr && src.hasProperty(pari->paramID))
			*pari = src.getProperty(pari->paramID);
		auto parb = dynamic_cast<AudioParameterBool*>(e);
		if (parb != nullptr && src.hasProperty(parb->paramID))
			*parb = src.getProperty(parb->paramID);
	}
}

template<typename F>
inline void timeCall(String msgprefix,F&& f)
{
	double t0 = Time::getMillisecondCounterHiRes();
	f();
	double t1 = Time::getMillisecondCounterHiRes();
	Logger::writeToLog(formatted(msgprefix, " took " , t1 - t0 , " ms"));
}

template<typename Cont, typename T>
inline void fill_container(Cont& c, const T& x)
{
	std::fill(std::begin(c), std::end(c), x);
}

template<typename T>
class CircularBuffer final
{
public:
	CircularBuffer(int size)
	{
		m_buf.resize(size);
	}
	void clear()
	{
		m_avail = 0;
		m_readpos = 0;
		m_writepos = 0;
		fill_container(m_buf, T());
	}
	void push(T x)
	{
		m_buf[m_writepos] = x;
		++m_writepos;
		++m_avail;
		if (m_writepos >= m_buf.size())
			m_writepos = 0;
	}
	T get()
	{
		jassert(m_avail > 0);
		T x = m_buf[m_readpos];
		++m_readpos;
		--m_avail;
		if (m_readpos >= m_buf.size())
			m_readpos = 0;
		return x;
	}
	int available() { return m_avail; }
	int getToBuf(T* buf, int len)
	{
		jassert(m_avail > 0);
		if (len > m_avail)
			len = m_avail;
		for (int i = 0; i < len; ++i)
			buf[i] = get();
		return len;
	}
	int getFromBuf(T* buf, int len)
	{
		for (int i = 0; i < len; ++i)
			push(buf[i]);
		return len;
	}
	int getSize() { return (int)m_buf.size(); }
	void resize(int size)
	{
		m_avail = 0;
		m_readpos = 0;
		m_writepos = 0;
		m_buf.resize(size);
	}
private:
	int m_writepos = 0;
	int m_readpos = 0;
	int m_avail = 0;
	std::vector<T> m_buf;
};

template<typename T, typename F>
inline void callGUI(T* ap, F&& f, bool async)
{
	auto ed = dynamic_cast<typename T::EditorType*>(ap->getActiveEditor());
	if (ed)
	{
		if (async == false)
			f(ed);
		else
			MessageManager::callAsync([ed, f]() { f(ed); });
	}
}

inline String secondsToString2(double secs)
{
	RelativeTime rt(secs);
	String result;
	result.preallocateBytes(32);
	bool empty = true;
	if ((int)rt.inHours()>0)
		result << String((int)rt.inHours() % 24).paddedLeft('0', empty ? 1 : 2) << ':';
	result << String((int)rt.inMinutes() % 60).paddedLeft('0', 2) << ':';
	result << String((int)rt.inSeconds() % 60).paddedLeft('0', 2);
	auto millis = (int)rt.inMilliseconds() % 1000;
	if (millis > 0)
		result << '.' << String(millis).paddedLeft('0', 3);
	return result.trimEnd();
}

inline String secondsToString(double seconds)
{
	int64_t durintseconds = seconds;
	int64_t durintminutes = seconds / 60.0;
	int64_t durinthours = seconds / 3600.0;
	int64_t durintdays = seconds / (3600 * 24.0);
	String timestring;
	if (durintminutes < 1)
		timestring = String(seconds, 3) + " seconds";
	if (durintminutes >= 1 && durinthours < 1)
		timestring = String(durintminutes) + " mins " + String(durintseconds % 60) + " secs";
	if (durinthours >= 1 && durintdays < 1)
		timestring = String(durinthours) + " hours " + String(durintminutes % 60) + " mins " + String(durintseconds % 60) + " secs";
	if (durintdays >= 1)
		timestring = String(durintdays) + " days " + String(durinthours % 24) + " hours " +
		String(durintminutes % 60) + " mins ";
	return timestring;
}

inline void toggleBool(bool& b)
{
	b = !b;
}

inline void toggleBool(AudioParameterBool* b)
{
	*b = !(*b);
}

template<typename T>
inline bool is_in_range(T val, T start, T end)
{
	return val >= start && val <= end;
}

inline void sanitizeTimeRange(double& t0, double& t1)
{
	if (t0 > t1)
		std::swap(t0, t1);
	if (t1 - t0 < 0.001)
		t1 = t0 + 0.001;
}

inline double fractpart(double x) { return x - (int)x; };

class SignalSmoother
{
public:
	SignalSmoother()
	{
		m_a = 0.5;
		m_slope = m_a;
		m_b = 1.0 - m_a;
		m_z = 0;
	}
	inline double process(double in)
	{
		double result = in + m_a * (m_z - in);
		m_z = result;
		return result; 
	}
	void setSlope(double x, double sr)
	{
		if (x != m_slope || sr != m_sr)
		{
			m_slope = x;
			m_sr = sr;
			double srCompensate = srCompensate = sr / 100.0;
			double compensated_a = powf(x, (1.0 / srCompensate));
			m_a = compensated_a;
			m_b = 1.0 - m_a;
		}
	}
	double getSlope() const { return m_slope; }
	double getSamplerate() const { return m_sr; }
private:
	double m_a, m_b, m_z;
	double m_slope;
	double m_sr = 0.0;
};
