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

#include "envelope_component.h"

EnvelopeComponent::EnvelopeComponent(CriticalSection* cs) : m_cs(cs)
{
	OnEnvelopeEdited = [](breakpoint_envelope*) {};
	setWantsKeyboardFocus(true);
	ValueFromNormalized = [](double x) { return x; };
	TimeFromNormalized = [](double x) { return x; };
	addChildComponent(&m_bubble);
    setOpaque(true);
}

EnvelopeComponent::~EnvelopeComponent()
{
	
}

void EnvelopeComponent::show_bubble(int x, int y, const envelope_node& node)
{
	double scaledtime = TimeFromNormalized(node.Time);
	double scaledvalue = ValueFromNormalized(node.Value);
	x -= 50;
	if (x < 0)
		x = 0;
	if (x + 100 > getWidth())
		x = getWidth() - 100;
	if (y < 0)
		y = 0;
	if (y + 20 > getHeight())
		y = getHeight() - 20;
	AttributedString temp(String::formatted("%.2f %.2f", scaledtime, scaledvalue));
	temp.setColour(Colours::white);
	m_bubble.showAt({ x,y,100,20 }, temp , 5000);
}

void EnvelopeComponent::paint(Graphics& g)
{
	if (!EnvelopeUnderlayDraw)
	{
		g.fillAll(Colours::black);
		g.setColour(Colours::white.darker());
		juce::Rectangle<int> rect(0, 0, getWidth(), getHeight());
        
		g.setFont(15.0);
		
	}
	else
	{
		g.saveState();
		EnvelopeUnderlayDraw(this, g);
		g.restoreState();
	}
	
	if (m_envelope == nullptr)
	{
		g.drawText("No envelope set", 10, 10, getWidth(), getHeight(), Justification::centred);
		return;
	}
	if (m_envelope.unique() == true)
	{
		g.drawText("Envelope is orphaned (may be a bug)", 10, 10, getWidth(), getHeight(), Justification::centred);
		return;
	}
	for (int i = 0; i < 10; ++i)
	{
		double norm = 1.0 / 10 * i;
		double hz = TimeFromNormalized(norm);
		int xcor = getWidth() / 10 * i;
		g.drawText(String(hz, 1), xcor, getHeight() - 20, 100, 20, Justification::topLeft);
	}
	String name = m_envelope->GetName();
	if (name.isEmpty() == true)
		name = "Untitled envelope";
	g.drawText(name, 10, 10, getWidth(), getHeight(), Justification::topLeft);
	auto draw_env = [this, &g](Colour envcolor, bool drawTransformed, float linethickness)
	{
		g.setColour(envcolor);
		double y0 = 0.0;
		if (drawTransformed==false)
			y0 = m_envelope->GetInterpolatedNodeValue(0.0);
		else y0 = m_envelope->getTransformedValue(0.0);
		const int drawstep = 1;
		for (int i = 1; i < getWidth(); ++i)
		{
			double env_x = 1.0 / getWidth()*i;
			double y1 = 0.0;
			if (drawTransformed==false)
				y1 = m_envelope->GetInterpolatedNodeValue(env_x);
			else y1 = m_envelope->getTransformedValue(env_x);
			double foo_y0 = (double)getHeight() - jmap<double>(y0, m_view_start_value, m_view_end_value, 0.0, getHeight());
			double foo_y1 = (double)getHeight() - jmap<double>(y1, m_view_start_value, m_view_end_value, 0.0, getHeight());
			g.drawLine((float)i, foo_y0, (float)i + 1, foo_y1, linethickness);
			y0 = y1;
		}
	};
	draw_env(m_env_color, false, 1.0f);
	draw_env(Colours::aquamarine.darker(), true, 1.0f);
	for (int i = 0; i < m_envelope->GetNumNodes(); ++i)
	{
		const envelope_node& pt = m_envelope->GetNodeAtIndex(i);
		double xcor = jmap(pt.Time, m_view_start_time, m_view_end_time, 0.0, (double)getWidth());
		double ycor = (double)getHeight() - jmap(pt.Value, m_view_start_value, m_view_end_value, 0.0, (double)getHeight());
		g.setColour(Colours::white);
		if (pt.Status == 0)
			g.drawRect((float)xcor - 4.0f, (float)ycor - 4.0f, 8.0f, 8.0f, 1.0f);
		else g.fillRect((float)xcor - 4.0f, (float)ycor - 4.0f, 8.0f, 8.0f);
	}
}

void EnvelopeComponent::changeListenerCallback(ChangeBroadcaster*)
{
	repaint();
}

void EnvelopeComponent::timerCallback(int)
{
	
}

void EnvelopeComponent::set_envelope(std::shared_ptr<breakpoint_envelope> env, String name)
{
	m_envelope = env;
	m_name = name;
	repaint();
}

void EnvelopeComponent::mouseDrag(const MouseEvent& ev)
{
	if (m_envelope == nullptr)
		return;
	if (m_segment_drag_info.first >= 0 && ev.mods.isAltDown())
	{
		double dist = jmap<double>(ev.getDistanceFromDragStartX(), -300.0, 300.0, -1.0, 1.0);
		m_envelope->performRelativeTransformation([dist, this](int index, envelope_node& point) 
		{ 
			if (index == m_segment_drag_info.first)
			{
				point.ShapeParam1 += dist;
				m_segment_drag_info.second = true;
			}
		});
		repaint();
		return;
	}
	if (m_segment_drag_info.first >= 0)
	{
		double dist = jmap<double>(ev.getDistanceFromDragStartY(), -getHeight(), getHeight(), -1.0, 1.0);
		m_envelope->adjustEnvelopeSegmentValues(m_segment_drag_info.first, -dist);
		m_envelope->updateMinMaxValues();
		repaint();
		return;
	}
	if (m_node_to_drag >= 0)
	{
		//Logger::writeToLog("trying to move pt " + String(m_node_to_drag));
		envelope_node& pt = m_envelope->GetNodeAtIndex(m_node_to_drag);
		double left_bound = m_view_start_time;
		double right_bound = m_view_end_time;
		if (m_node_to_drag > 0 )
		{
			left_bound = m_envelope->GetNodeAtIndex(m_node_to_drag - 1).Time;
		}
		if (m_node_to_drag < m_envelope->GetNumNodes() - 1)
		{
			right_bound = m_envelope->GetNodeAtIndex(m_node_to_drag + 1).Time;
		}
		double normx = jmap((double)ev.x, 0.0, (double)getWidth(), m_view_start_time, m_view_end_time);
		double normy = jmap((double)getHeight() - ev.y, 0.0, (double)getHeight(), m_view_start_value, m_view_end_value);
		pt.Time=jlimit(left_bound+0.001, right_bound - 0.001, normx);
		pt.Value=jlimit(0.0,1.0,normy);
		m_envelope->updateMinMaxValues();
		m_last_tip = String(pt.Time, 2) + " " + String(pt.Value, 2);
		show_bubble(ev.x, ev.y, pt);
		m_node_that_was_dragged = m_node_to_drag;
		repaint();
		return;
	}
}

void EnvelopeComponent::mouseMove(const MouseEvent & ev)
{
	if (m_envelope == nullptr)
		return;
	m_node_to_drag = find_hot_envelope_point(ev.x, ev.y);
	if (m_node_to_drag >= 0)
	{
		if (m_mouse_down == false)
		{
			show_bubble(ev.x, ev.y, m_envelope->GetNodeAtIndex(m_node_to_drag));
			setMouseCursor(MouseCursor::PointingHandCursor);
		}
	}
	else
	{
		int temp = findHotEnvelopeSegment(ev.x, ev.y, true);
		if (temp>=0)
			setMouseCursor(MouseCursor::UpDownResizeCursor);
		else
			setMouseCursor(MouseCursor::NormalCursor);
		m_bubble.setVisible(false);
	}
}

void EnvelopeComponent::mouseDown(const MouseEvent & ev)
{
	if (m_envelope == nullptr)
		return;
	if (ev.mods.isRightButtonDown() == true)
	{
		PopupMenu menu;
		menu.addItem(1, "Reset");
		menu.addItem(2, "Invert");
		int r = menu.show();
		if (r == 1)
		{
			m_cs->enter();
			m_envelope->ResetEnvelope();
			m_cs->exit();
		}
		if (r == 2)
		{
			for (int i = 0; i < m_envelope->GetNumNodes(); ++i)
			{
				double val = 1.0 - m_envelope->GetNodeAtIndex(i).Value;
				m_envelope->GetNodeAtIndex(i).Value = val;
			}
		}
		repaint();
		return;
	}
	m_node_to_drag = find_hot_envelope_point(ev.x, ev.y);
	m_mouse_down = true;
	m_segment_drag_info = { findHotEnvelopeSegment(ev.x, ev.y, true),false };
	if (m_segment_drag_info.first >= 0)
	{
		m_envelope->beginRelativeTransformation();
		return;
	}
	if (m_node_to_drag >= 0 && ev.mods.isAltDown() == true)
	{
		if (m_envelope->GetNumNodes() < 2)
		{
			m_bubble.showAt({ ev.x,ev.y, 0,0 }, AttributedString("Can't remove last node"), 3000, false, false);
			return;
		}
		m_cs->enter();
		m_envelope->DeleteNode(m_node_to_drag);
		m_cs->exit();
		m_envelope->updateMinMaxValues();
		m_node_to_drag = -1;
		OnEnvelopeEdited(m_envelope.get());
		repaint();
		return;
	}
	if (m_node_to_drag >= 0 && ev.mods.isShiftDown()==true)
	{
		int oldstatus = m_envelope->GetNodeAtIndex(m_node_to_drag).Status;
		if (oldstatus==0)
			m_envelope->GetNodeAtIndex(m_node_to_drag).Status=1;
		else m_envelope->GetNodeAtIndex(m_node_to_drag).Status=0;
		repaint();
		return;
	}
	if (m_node_to_drag == -1)
	{
		double normx = jmap((double)ev.x, 0.0, (double)getWidth(), m_view_start_time, m_view_end_time);
		double normy = jmap((double)getHeight() - ev.y, 0.0, (double)getHeight(), m_view_start_value, m_view_end_value);
		m_cs->enter();
		m_envelope->AddNode ({ normx,normy, 0.5});
		m_envelope->SortNodes();
		m_cs->exit();
		m_envelope->updateMinMaxValues();
		m_mouse_down = false;
		OnEnvelopeEdited(m_envelope.get());
		repaint();
	}
}

void EnvelopeComponent::mouseUp(const MouseEvent &ev)
{
	if (ev.mods == ModifierKeys::noModifiers)
		m_bubble.setVisible(false);
	if (m_node_that_was_dragged >= 0 || m_segment_drag_info.second==true)
	{
		OnEnvelopeEdited(m_envelope.get());
	}
	m_mouse_down = false;
	m_node_that_was_dragged = -1;
	m_node_to_drag = -1;
	if (m_segment_drag_info.second == true)
	{
		m_segment_drag_info = { -1,false };
		m_envelope->endRelativeTransformation();
	}
}

bool EnvelopeComponent::keyPressed(const KeyPress & ev)
{
	if (m_envelope == nullptr)
		return false;
	auto f = [this](auto& env_var, double amt)
    {
        env_var+=amt;
        return true;
    };
    bool r = false;
    if (ev == 'Q')
        r = f(m_envelope->m_transform_x_shift,-0.01);
    if (ev == 'W')
		r = f(m_envelope->m_transform_x_shift,0.01);
	if (ev == 'E')
        r = f(m_envelope->m_transform_y_shift,0.01);
	if (ev == 'D')
		r = f(m_envelope->m_transform_y_shift,-0.01);
	if (ev == 'R')
		r = f(m_envelope->m_transform_y_scale,0.05);
	if (ev == 'F')
		r = f(m_envelope->m_transform_y_scale,-0.05);
    if (ev == 'T')
        r = f(m_envelope->m_transform_y_sinus,0.01);
    if (ev == 'G')
        r = f(m_envelope->m_transform_y_sinus,-0.01);
    if (ev == 'Y')
        r = f(m_envelope->m_transform_y_tilt,0.02);
    if (ev == 'H')
        r = f(m_envelope->m_transform_y_tilt,-0.02);
    if (ev == 'V')
        r = f(m_envelope->m_transform_y_sinus_freq,1.0);
    if (ev == 'B')
        r = f(m_envelope->m_transform_y_sinus_freq,-1.0);
    m_envelope->m_transform_y_sinus_freq = jlimit(1.0,64.0, m_envelope->m_transform_y_sinus_freq);
    if (r==true)
    {
        repaint();
        return true;
    }
	if (ev == 'A')
	{
		m_envelope->m_transform_x_shift = 0.0;
		m_envelope->m_transform_y_scale = 1.0;
		m_envelope->m_transform_y_shift = 0.0;
		m_envelope->m_transform_y_sinus = 0.0;
		repaint();
		return true;
	}

	if (ev == KeyPress::deleteKey)
	{
		m_node_to_drag = -1;
		//m_envelope->ClearAllNodes();
		m_cs->enter();
		m_envelope->removePointsConditionally([](const envelope_node& pt) { return pt.Status == 1; });
		if (m_envelope->GetNumNodes()==0)
			m_envelope->AddNode({ 0.0,0.5 });
		m_cs->exit();
		repaint();
		OnEnvelopeEdited(m_envelope.get());
		
		return true;
	}
	return false;
}

int EnvelopeComponent::find_hot_envelope_point(double xcor, double ycor)
{
	if (m_envelope == nullptr)
		return -1;
	for (int i = 0; i < m_envelope->GetNumNodes(); ++i)
	{
		const envelope_node& pt = m_envelope->GetNodeAtIndex(i);
		double ptxcor = jmap(pt.Time, m_view_start_time, m_view_end_time, 0.0, (double)getWidth());
		double ptycor = (double)getHeight() - jmap(pt.Value, m_view_start_value, m_view_end_value, 0.0, (double)getHeight());
		juce::Rectangle<double> target(ptxcor - 4.0, ptycor - 4.0, 8.0, 8.0);
		if (target.contains(xcor, ycor) == true)
		{
			return i;
		}
	}
	return -1;
}

int EnvelopeComponent::findHotEnvelopeSegment(double xcor, double ycor, bool detectsegment)
{
	if (m_envelope == nullptr)
		return -1;
	for (int i = 0; i < m_envelope->GetNumNodes()-1; ++i)
	{
		const envelope_node& pt0 = m_envelope->GetNodeAtIndex(i);
		const envelope_node& pt1 = m_envelope->GetNodeAtIndex(i+1);
		float xcor0 = (float)jmap<double>(pt0.Time, m_view_start_time, m_view_end_time, 0.0, getWidth());
		float xcor1 = (float)jmap<double>(pt1.Time, m_view_start_time, m_view_end_time, 0.0, getWidth());
		float segwidth = xcor1 - xcor0;
		juce::Rectangle<float> segrect(xcor0+8.0f, 0.0f, segwidth-16.0f, (float)getHeight());
		if (segrect.contains((float)xcor, (float)ycor))
		{
			if (detectsegment == false)
				return i;
			else
			{
				double normx = jmap<double>(xcor, 0.0, getWidth(), m_view_start_time, m_view_end_time);
				double yval = m_envelope->GetInterpolatedNodeValue(normx);
				float ycor0 = (float)(getHeight()-jmap<double>(yval, 0.0, 1.0, 0.0, getHeight()));
				juce::Rectangle<float> segrect2((float)(xcor - 20), (float)(ycor - 10), 40, 20);
				if (segrect2.contains((float)xcor, ycor0))
					return i;
			}
		}
		
		
	}
	return -1;
}
