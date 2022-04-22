// SPDX-License-Identifier: GPLv3-or-later WITH Appstore-exception
// Copyright (C) 2017 Xenakios
// Copyright (C) 2020 Jesse Chappell


#pragma once

#include <vector>
#include <algorithm>
#include <random>
#include "../JuceLibraryCode/JuceHeader.h"
#include "PS_Source/globals.h"

struct envelope_point
{
	envelope_point()
        : pt_x(0.0), pt_y(0.0), ShapeParam1(0.5), ShapeParam2(0.5) {}
    envelope_point(double x, double y, double p1=0.5, double p2=0.5)
        : pt_x(x), pt_y(y),ShapeParam1(p1),ShapeParam2(p2) {}
    double pt_x;
    double pt_y;
    int Shape = 0;
    double ShapeParam1;
    double ShapeParam2;
    int Status = 0;
	size_t get_hash() const
	{
		size_t seed = 0;
		seed ^= std::hash<double>()(pt_x) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
		seed ^= std::hash<double>()(pt_y) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
		seed ^= std::hash<int>()(Shape) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
		seed ^= std::hash<double>()(ShapeParam1) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
		seed ^= std::hash<double>()(ShapeParam2) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
		return seed;
	}
	

};

inline bool operator<(const envelope_point& a, const envelope_point& b)
{
    return a.pt_x<b.pt_x;
}

template<typename T>
inline void appendToMemoryBlock(MemoryBlock& mb, T x)
{
	T temp(x);
	mb.append((void*)&temp, sizeof(temp));
}


struct grid_entry
{
    grid_entry(double v) : m_value(v) {}
    double m_value=0.0;
    bool m_foo=false;
};

inline double grid_value(const grid_entry& ge)
{
    return ge.m_value;
}

inline bool operator<(const grid_entry& a, const grid_entry& b)
{
    return a.m_value<b.m_value;
}

using grid_t=std::vector<grid_entry>;

//#define BEZIER_EXPERIMENT

inline double get_shaped_value(double x, int, double p1, double)
{
#ifndef BEZIER_EXPERIMENT
    if (p1<0.5)
    {
        double foo=1.0-(p1*2.0);
        return 1.0-pow(1.0-x,1.0+foo*4.0);
    }
    double foo=(p1-0.5)*2.0;
    return pow(x,1.0+foo*4.0);
#else
    /*
    double pt0=-2.0*p1;
    double pt1=2.0*p2;
    double pt2=1.0;
    return pow(1-x,2.0)*pt0+2*(1-x)*x*pt1+pow(x,2)*pt2;
    */
    if (p2<=0.5)
    {
        if (p1<0.5)
        {
            double foo=1.0-(p1*2.0);
            return 1.0-pow(1.0-x,1.0+foo*4.0);
        }
        double foo=(p1-0.5)*2.0;
        return pow(x,1.0+foo*4.0);
    } else
    {
        if (p1<0.5)
        {
            if (x<0.5)
            {
                x*=2.0;
                p1*=2.0;
                return 0.5*pow(x,p1*4.0);
            } else
            {
                x-=0.5;
                x*=2.0;
                p1*=2.0;
                return 1.0-0.5*pow(1.0-x,p1*4.0);
            }
        } else
        {
            if (x<0.5)
            {
                x*=2.0;
                p1-=0.5;
                p1*=2.0;
                return 0.5-0.5*pow(1.0-x,p1*4.0);
            } else
            {
                x-=0.5;
                x*=2.0;
                p1-=0.5;
                p1*=2.0;
                return 0.5+0.5*pow(x,p1*4.0);
            }
        }
    }
    return x;
#endif
}

using nodes_t=std::vector<envelope_point>;

inline double GetInterpolatedEnvelopeValue(const nodes_t& m_nodes, double atime, double m_defvalue=0.5)
{
    int maxnodeind=(int)m_nodes.size()-1;
    if (m_nodes.size()==0) return m_defvalue;
    if (m_nodes.size()==1) return m_nodes[0].pt_y;
    if (atime<=m_nodes[0].pt_x)
        return m_nodes[0].pt_y;
    if (atime>m_nodes[maxnodeind].pt_x)
        return m_nodes[maxnodeind].pt_y;
    const envelope_point to_search(atime,0.0);
    //to_search.Time=atime;
    auto it=std::lower_bound(m_nodes.begin(),m_nodes.end(),to_search,
                             [](const envelope_point& a, const envelope_point& b)
    { return a.pt_x<b.pt_x; } );
    if (it==m_nodes.end())
    {
        return m_defvalue;
    }
    --it; // lower_bound has returned iterator to point one too far
    double t1=it->pt_x;
    double v1=it->pt_y;
    double p1=it->ShapeParam1;
    double p2=it->ShapeParam2;
    ++it; // next envelope point
    double tdelta=it->pt_x-t1;
    if (tdelta<0.00001)
        tdelta=0.00001;
    double vdelta=it->pt_y-v1;
    return v1+vdelta*get_shaped_value(((1.0/tdelta*(atime-t1))),0,p1,p2);

}

inline double interpolate_foo(double atime,double t0, double v0, double t1, double v1, double p1, double p2)
{
    double tdelta=t1-t0;
    if (tdelta<0.00001)
        tdelta=0.00001;
    double vdelta=v1-v0;
    return v0+vdelta*get_shaped_value(((1.0/tdelta*(atime-t0))),0,p1,p2);
}

class breakpoint_envelope
{
public:
    breakpoint_envelope() : m_name("Untitled") 
	{
		m_randbuf.resize(1024);
	}
    breakpoint_envelope(String name, double minv=0.0, double maxv=1.0)
        : m_minvalue(minv), m_maxvalue(maxv), m_name(name)
    {
        m_defshape=0;
        //m_color=RGB(0,255,255);
        m_defvalue=0.5;
        m_updateopinprogress=false;
        m_value_grid={0.0,0.25,0.5,0.75,1.0};
		m_randbuf.resize(1024);
    }
	std::unique_ptr<breakpoint_envelope> duplicate()
	{
		auto result = std::make_unique<breakpoint_envelope>();
		result->m_nodes = m_nodes;
		result->m_randbuf = m_randbuf;
		result->m_transform_wrap_x = m_transform_wrap_x;
		result->m_transform_x_shift = m_transform_x_shift;
		return result;
	}


    void SetName(String Name) { m_name=Name; }
    const String& GetName() const { return m_name; }
    double GetDefValue() const { return m_defvalue; }
    void SetDefValue(double value) { m_defvalue=value; }
    int GetDefShape() const { return m_defshape; }
	ValueTree saveState(Identifier id) const
	{
		ValueTree result(id);
		for (int i = 0; i < m_nodes.size(); ++i)
		{
			ValueTree pt_tree("pt");
			storeToTreeProperties(pt_tree, nullptr,
				"x", m_nodes[i].pt_x, "y", m_nodes[i].pt_y, "p1", m_nodes[i].ShapeParam1, "p2", m_nodes[i].ShapeParam2);
			result.addChild(pt_tree, -1, nullptr);
		}
		result.setProperty("wrapxtransform", m_transform_wrap_x, nullptr);
		result.setProperty("yrandlerp", m_transform_y_random_linear_interpolation, nullptr);
		return result;
	}
	void restoreState(ValueTree state)
	{
		if (state.isValid()==false)
            return;
		m_transform_wrap_x = state.getProperty("wrapxtransform", false);
		m_transform_y_random_linear_interpolation = state.getProperty("yrandlerp", false);
		int numnodes = state.getNumChildren();
		if (numnodes > 0)
		{
			m_nodes.clear();
			for (int i = 0; i < numnodes; ++i)
			{
				ValueTree pt_tree = state.getChild(i);
				double x, y = 0.0;
				double p1, p2 = 0.5;
				getFromTreeProperties(pt_tree, "x", x, "y", y, "p1", p1, "p2", p2);
				m_nodes.emplace_back(x, y, p1,p2);
			}
			SortNodes();
		}
	}
	MD5 getHash() const
	{
		MemoryBlock mb;
		for (int i = 0; i < m_nodes.size(); ++i)
		{
			appendToMemoryBlock(mb, m_nodes[i].pt_x);
			appendToMemoryBlock(mb, m_nodes[i].pt_y);
			appendToMemoryBlock(mb, m_nodes[i].ShapeParam1);
			appendToMemoryBlock(mb, m_nodes[i].ShapeParam2);
		}
		return MD5(mb);
	}
	
    int GetNumPoints() const { return (int)m_nodes.size(); }
    void SetDefShape(int value) { m_defshape=value; }
	double getNodeLeftBound(int index, double margin=0.01) const noexcept
	{
		if (m_nodes.size() == 0)
			return 0.0;
		if (index == 0)
			return 0.0;
		return m_nodes[index - 1].pt_x + margin;
	}
	double getNodeRightBound(int index, double margin = 0.01) const noexcept
	{
		if (m_nodes.size() == 0)
			return 1.0;
		if (index == m_nodes.size()-1)
			return 1.0;
		return m_nodes[index + 1].pt_x - margin;
	}
	const std::vector<envelope_point>& get_all_nodes() const { return m_nodes; }
    void set_all_nodes(nodes_t nds) { m_nodes=std::move(nds); }
    void set_reset_nodes(const std::vector<envelope_point>& nodes, bool convertvalues=false)
    {
        if (convertvalues==false)
            m_reset_nodes=nodes;
        else
        {
            if (scaled_to_normalized_func)
            {
                m_nodes.clear();
                for (int i=0;i<nodes.size();++i)
                {
                    envelope_point node=nodes[i];
                    node.pt_y=scaled_to_normalized_func(node.pt_y);
                    m_nodes.push_back(node);
                }
            }
        }
    }
    void ResetEnvelope()
    {
        m_nodes=m_reset_nodes;
        m_playoffset=0.0;
    }
    Colour GetColor() const
    {
        return m_colour;
    }
    void SetColor(Colour colour)
    {
        m_colour=colour;
    }
    void BeginUpdate() // used for doing larger update operations, so can avoid needlessly sorting etc
    {
        m_updateopinprogress=true;
    }
    void EndUpdate()
    {
        m_updateopinprogress=false;
        SortNodes();
    }
    void AddNode(envelope_point newnode)
    {
        m_nodes.push_back(newnode);
        if (!m_updateopinprogress)
            SortNodes();
    }
    void ClearAllNodes()
    {
        m_nodes.clear();
    }
    void DeleteNode(int indx)
    {
        if (indx<0 || indx>m_nodes.size()-1)
            return;
        m_nodes.erase(m_nodes.begin()+indx);
    }
    void delete_nodes_in_time_range(double t0, double t1)
    {
        m_nodes.erase(std::remove_if(std::begin(m_nodes),
                                       std::end(m_nodes),
                                       [t0,t1](const envelope_point& a) { return a.pt_x>=t0 && a.pt_x<=t1; } ),
                                       std::end(m_nodes) );
    }
	template<typename F>
	void removePointsConditionally(F predicate)
	{
		m_nodes.erase(std::remove_if(m_nodes.begin(), m_nodes.end(), predicate), m_nodes.end());
	}
    envelope_point& GetNodeAtIndex(int indx)
    {
        if (m_nodes.size()==0)
        {
            throw(std::length_error("Empty envelope accessed"));
        }
        if (indx<0)
            indx=0;
        if (indx>=(int)m_nodes.size())
            indx=(int)m_nodes.size()-1;
        return m_nodes[indx];
    }
    const envelope_point& GetNodeAtIndex(int indx) const
    {
        if (m_nodes.size()==0)
        {
            throw(std::length_error("Empty envelope accessed"));
        }
        if (indx<0)
            indx=0;
        if (indx>=(int)m_nodes.size())
            indx=(int)m_nodes.size()-1;
        return m_nodes[indx];
    }
    void SetNodeStatus(int indx, int nstatus)
    {
        int i=indx;
        if (indx<0) i=0;
        if (indx>(int)m_nodes.size()-1) i=(int)m_nodes.size()-1;
        m_nodes[i].Status=nstatus;
    }
    void SetNodeStatusForAll(int nstatus)
    {
        for (int i=0; i < m_nodes.size(); ++i) {
            m_nodes[i].Status = nstatus;
        }
    }

    void SetNode(int indx, envelope_point anode)
    {
        int i=indx;
        if (indx<0) i=0;
        if (indx>(int)m_nodes.size()-1) i=(int)m_nodes.size()-1;
        m_nodes[i]=anode;
    }
    void SetNodeTimeValue(int indx,bool setTime,bool setValue,double atime,double avalue)
    {
        int i=indx;
        if (indx<0) i=0;
        if (indx>(int)m_nodes.size()-1) i=(int)m_nodes.size()-1;
        if (setTime) m_nodes[i].pt_x=atime;
        if (setValue) m_nodes[i].pt_y=avalue;
    }


    double GetInterpolatedEnvelopeValue(double atime) const
    {
        double t0=0.0;
        double t1=0.0;
        double v0=0.0;
        double v1=0.0;
        double p1=0.0;
        double p2=0.0;
        const int maxnodeind=(int)m_nodes.size()-1;
        if (m_nodes.size()==0)
            return m_defvalue;
        if (m_nodes.size()==1)
            return m_nodes[0].pt_y;
        if (atime<=m_nodes[0].pt_x)
        {
#ifdef INTERPOLATING_ENVELOPE_BORDERS
            t1=m_nodes[0].Time;
            t0=0.0-(1.0-m_nodes[maxnodeind].Time);
            v0=m_nodes[maxnodeind].Value;
            p1=m_nodes[maxnodeind].ShapeParam1;
            p2=m_nodes[maxnodeind].ShapeParam2;
            v1=m_nodes[0].Value;
            return interpolate_foo(atime,t0,v0,t1,v1,p1,p2);
#else
            return m_nodes[0].pt_y;
#endif
        }
        if (atime>m_nodes[maxnodeind].pt_x)
        {
#ifdef INTERPOLATING_ENVELOPE_BORDERS
            t0=m_nodes[maxnodeind].Time;
            t1=1.0+(m_nodes[0].Time);
            v0=m_nodes[maxnodeind].Value;
            v1=m_nodes[0].Value;
            p1=m_nodes[maxnodeind].ShapeParam1;
            p2=m_nodes[maxnodeind].ShapeParam2;
            return interpolate_foo(atime,t0,v0,t1,v1,p1,p2);
#else
            return m_nodes.back().pt_y;
#endif
        }
        const envelope_point to_search(atime,0.0);
        //to_search.Time=atime;
        auto it=std::lower_bound(m_nodes.begin(),m_nodes.end(),to_search,
                                 [](const envelope_point& a, const envelope_point& b)
        { return a.pt_x<b.pt_x; } );
        if (it==m_nodes.end())
        {
            return m_defvalue;
        }
        --it; // lower_bound has returned iterator to point one too far
        t0=it->pt_x;
        v0=it->pt_y;
        p1=it->ShapeParam1;
        p2=it->ShapeParam2;
        ++it; // next envelope point
        t1=it->pt_x;
        v1=it->pt_y;
        return interpolate_foo(atime,t0,v0,t1,v1,p1,p2);
    }
    bool IsSorted() const
    {
        return std::is_sorted(m_nodes.begin(), m_nodes.end(), []
                              (const envelope_point& lhs, const envelope_point& rhs)
                              {
                                  return lhs.pt_x<rhs.pt_x;
                              });
    }
    void SortNodes()
    {
        stable_sort(m_nodes.begin(),m_nodes.end(),
             [](const envelope_point& a, const envelope_point& b){ return a.pt_x<b.pt_x; } );
    }
    double minimum_value() const { return m_minvalue; }
    double maximum_value() const { return m_maxvalue; }
    void set_minimum_value(double v) { m_minvalue=v; }
    void set_maximum_value(double v) { m_maxvalue=v; }
    std::function<double(double)> normalized_to_scaled_func;
    std::function<double(double)> scaled_to_normalized_func;
    void beginRelativeTransformation()
    {
        m_old_nodes=m_nodes;
    }
    void endRelativeTransformation()
    {
        m_old_nodes.clear();
    }
	nodes_t& getRelativeTransformBaseNodes()
	{
		return m_old_nodes;
	}
    template<typename F>
	inline void performRelativeTransformation(F&& f)
	{
		for (int i = 0; i < m_old_nodes.size(); ++i)
		{
			envelope_point node = m_old_nodes[i];
			f(i, node);
			node.ShapeParam1 = jlimit(0.0, 1.0, node.ShapeParam1);
			m_nodes[i] = node;
		}
	}
	void adjustEnvelopeSegmentValues(int index, double amount)
	{
		if (index >= m_old_nodes.size())
		{
			m_nodes.back().pt_y = jlimit(0.0,1.0,m_old_nodes.back().pt_y+amount);
			return;
		}
		m_nodes[index].pt_y = jlimit(0.0, 1.0, m_old_nodes[index].pt_y + amount);
		m_nodes[index+1].pt_y = jlimit(0.0, 1.0, m_old_nodes[index+1].pt_y + amount);
	}
    const nodes_t& repeater_nodes() const
    {
        return m_repeater_nodes;
    }
    void store_repeater_nodes()
    {
        m_repeater_nodes.clear();
        for (int i=0;i<m_nodes.size();++i)
        {
            if (m_nodes[i].pt_x>=m_playoffset && m_nodes[i].pt_x<=m_playoffset+1.0)
            {
                envelope_point temp=m_nodes[i];
                temp.pt_x-=m_playoffset;
                m_repeater_nodes.push_back(temp);
            }
        }
    }
    double get_play_offset() const { return m_playoffset; }
    //void set_play_offset(double x) { m_playoffset=bound_value(m_mintime,x,m_maxtime); }
    //time_range get_play_offset_range() const { return std::make_pair(m_mintime,m_maxtime); }
    const grid_t& get_value_grid() const { return m_value_grid; }
    void set_value_grid(grid_t g) { m_value_grid=std::move(g); }
    template<typename F>
    void manipulate(F&& f)
    {
        nodes_t backup=m_nodes;
        if (f(backup)==true)
        {
            std::swap(backup,m_nodes);
            SortNodes();
        }
    }
	template<typename F0, typename F1>
	inline void resamplePointToLinearSegments(int point_index,double /*xmin*/, double /*xmax*/, double /*ymin*/, double /*ymax*/, 
		F0&& handlesegmentfunc, F1&& numsegmentsfunc)
	{
		if (m_nodes.size() == 0)
			return;
		
		envelope_point pt0 = GetNodeAtIndex(point_index);
		envelope_point pt1 = GetNodeAtIndex(point_index+1);
		double xdiff = pt1.pt_x - pt0.pt_x;
		if (xdiff > 0.0)
		{
			int numsegments = numsegmentsfunc(xdiff);
			for (int j=0;j<numsegments;++j)
			{
				double cb_x0 = pt0.pt_x + xdiff / (numsegments)*j;
				double cb_y0 = GetInterpolatedEnvelopeValue(cb_x0);
				double cb_x1 = pt0.pt_x + xdiff / (numsegments)*(j+1);
				double cb_y1 = GetInterpolatedEnvelopeValue(cb_x1);
				handlesegmentfunc(cb_x0, cb_y0,cb_x1,cb_y1);
			}
		}
		
	}
	double m_transform_x_shift = 0.0;
	double m_transform_y_shift = 0.0;
	double m_transform_y_scale = 1.0;
    double m_transform_y_sinus = 0.0;
    double m_transform_y_sinus_freq = 8.0;
    double m_transform_y_tilt = 0.0;
	double m_transform_y_random_amount = 0.0;
	double m_transform_y_random_rate = 2.0;
	bool m_transform_y_random_linear_interpolation = false;
	int m_transform_y_random_bands = 32;
	bool m_transform_wrap_x = false;
	double m_min_pt_value = 0.0;
	double m_max_pt_value = 0.0;
	inline double getTransformedValue(double x)
	{
		if (isTransformed() == false)
			return GetInterpolatedEnvelopeValue(x);
		double temp = x-m_transform_x_shift;
		if (m_transform_wrap_x == true)
		{
			temp = fmod(x - m_transform_x_shift, 1.0);
			if (temp < 0.0)
				temp += 1.0;
		}
		double v = GetInterpolatedEnvelopeValue(temp);
		double center_v = m_minvalue + (m_maxvalue - m_minvalue) / 2.0;
		double diff = center_v - v;
		double scaled = center_v - m_transform_y_scale * diff;
		double shifted = scaled + m_transform_y_shift;
		if (m_transform_y_sinus>0.0)
			shifted+=m_transform_y_sinus * sin(2*c_PI*(x-m_transform_x_shift)*m_transform_y_sinus_freq);
        double tiltline = m_transform_y_tilt-(2.0*m_transform_y_tilt*x);
        double tilted = shifted+tiltline;
		if (m_transform_y_random_amount > 0.0)
		{
			if (m_transform_y_random_linear_interpolation == false)
			{
				int tableindex = jlimit<int>(0, m_randbuf.size() - 1, floor(x * (m_transform_y_random_bands)));
				double randamt = jmap(m_randbuf[tableindex], 0.0, 1.0, -m_transform_y_random_amount, m_transform_y_random_amount);
				tilted += randamt;
			}
			else
			{
				double fracindex = x * m_transform_y_random_bands;
				int tableindex0 = jlimit<int>(0, m_randbuf.size() - 1, floor(fracindex));
				int tableindex1 = tableindex0 + 1;
				double y0 = m_randbuf[tableindex0];
				double y1 = m_randbuf[tableindex1];
				double interpolated = y0 + (y1 - y0)*fractpart(fracindex);
				double randamt = jmap(interpolated, 0.0, 1.0, -m_transform_y_random_amount, m_transform_y_random_amount);
				tilted += randamt;
			}
		}
		return jlimit(0.0,1.0,tilted);
	}
	bool isTransformed() const
	{
		return m_transform_x_shift != 0.0 || m_transform_y_shift != 0.0
        || m_transform_y_scale!=1.0 || m_transform_y_sinus!=0.0 || m_transform_y_tilt!=0.0 
		|| m_transform_y_random_amount>0.0;
	}
	void updateMinMaxValues()
	{
		double minv = 1.0;
		double maxv = 0.0;
		for (auto& e : m_nodes)
		{
			minv = std::min(minv, e.pt_y);
			maxv = std::max(maxv, e.pt_y);
		}
		m_minvalue = minv;
		m_maxvalue = maxv;
	}
	void updateRandomState()
	{
		//Logger::writeToLog("updating envelope random state");
		std::uniform_real_distribution<double> dist(0.0,1.0);
		for (int i = 0; i < m_transform_y_random_bands+1; ++i)
			m_randbuf[i] = dist(m_randgen);
	}
private:
    nodes_t m_nodes;
    double m_playoffset=0.0;
    double m_minvalue=0.0;
    double m_maxvalue=1.0;
    double m_mintime=-2.0;
    double m_maxtime=2.0;
    int m_defshape;
    Colour m_colour;
    String m_name;
    bool m_updateopinprogress;
    double m_defvalue; // "neutral" value to be used for resets and stuff

    nodes_t m_reset_nodes;
    nodes_t m_old_nodes;
    nodes_t m_repeater_nodes;
    grid_t m_value_grid;
	std::mt19937 m_randgen;
	std::vector<double> m_randbuf;
	JUCE_LEAK_DETECTOR(breakpoint_envelope)
};

template<typename F, typename... Args>
inline double derivative(const F& f, double x, const Args&... func_args)
{
	const double epsilon = std::numeric_limits<double>::epsilon() * 100;
	//const double epsilon=0.000001;
	return (f(x + epsilon, func_args...) - f(x, func_args...)) / epsilon;
}

using shared_envelope = std::shared_ptr<breakpoint_envelope>;
