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
#include "../JuceLibraryCode/JuceHeader.h"

using REALTYPE = float;

using floatvector = std::vector<REALTYPE>;
using float2dvector = std::vector<std::vector<float>>;
using float3dvector = std::vector<std::vector<std::vector<float>>>;

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

#ifndef M_PI
#define M_PI 3.14159265359
#endif

const int g_maxnumoutchans = 32;

#ifndef USE_LUA_SCRIPTING
//#define USE_LUA_SCRIPTING
#endif

inline void storeToTreeProperties(ValueTree dest, UndoManager* uman, juce::Identifier varname, var val)
{
	dest.setProperty(varname, val, uman);
}

template<typename... Ts>
inline void storeToTreeProperties(ValueTree dest, UndoManager* uman, juce::Identifier varname, var val, Ts&&... args)
{
	dest.setProperty(varname, val, uman);
	storeToTreeProperties(dest, uman, args...);
}

template<typename T>
inline void storeToTreeProperties(ValueTree dest, UndoManager* uman, juce::Identifier varname, Range<T> x)
{
	dest.setProperty(varname+"_start", x.getStart(), uman);
	dest.setProperty(varname+"_end", x.getEnd(), uman);
}

template<typename T>
inline void getFromTreeProperties(ValueTree src, juce::Identifier varname, T& val)
{
	if (src.hasProperty(varname))
		val = src.getProperty(varname);
}

template<typename... Ts, typename T>
inline void getFromTreeProperties(ValueTree src, juce::Identifier varname, T& val, Ts&... args)
{
	if (src.hasProperty(varname))
		val = src.getProperty(varname);
	getFromTreeProperties(src, args...);
}

template<typename T>
inline void getFromTreeProperties(ValueTree src, juce::Identifier varname, Range<T>& rng)
{
	if (src.hasProperty(varname + "_start") && src.hasProperty(varname + "_end"))
	{
		rng.setStart(src.getProperty(varname + "_start"));
		rng.setEnd(src.getProperty(varname + "_end"));
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
		std::fill(m_buf.begin(), m_buf.end(), T());
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

template<typename Cont,typename T>
inline void fill_container(Cont& c, const T& x)
{
	std::fill(std::begin(c), std::end(c), x);
}

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
