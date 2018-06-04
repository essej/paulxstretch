/*

Copyright (C) 2017 Xenakios

This program is free software; you can redistribute it and/or modify
it under the terms of version 3 of the GNU General Public License
as published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License (version 3) for more details.

www.gnu.org/licenses

*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include <memory>
#include <functional>
#include <future>
#include "jcdp_envelope.h"

class EnvelopeComponent : public Component, 
	public ChangeListener, 
	public MultiTimer
	//public TooltipClient
{
public:
	EnvelopeComponent(CriticalSection* cs);
	~EnvelopeComponent();
	void paint(Graphics& g) override;
	void set_envelope(std::shared_ptr<breakpoint_envelope> env, String name = String());
	std::shared_ptr<breakpoint_envelope> get_envelope() { return m_envelope; }
	String get_name() { return m_name; }
	void mouseMove(const MouseEvent& ev) override;
	void mouseDown(const MouseEvent& ev) override;
	void mouseDrag(const MouseEvent& ev) override;
	void mouseUp(const MouseEvent& ev) override;
	bool keyPressed(const KeyPress& ev) override;
	double get_view_start_time() const { return m_view_start_time; }
	double get_view_end_time() const { return m_view_end_time; }
	void set_view_start_time(double t) { m_view_start_time = t; repaint(); }
	void set_view_end_time(double t) { m_view_end_time = t; repaint(); }
	std::function<void(EnvelopeComponent*, Graphics&)> EnvelopeUnderlayDraw;
	std::function<void(EnvelopeComponent*, Graphics&)> EnvelopeOverlayDraw;
	std::function<void(breakpoint_envelope*)> OnEnvelopeEdited;
	std::function<double(double)> ValueFromNormalized;
	std::function<double(double)> TimeFromNormalized;
	void changeListenerCallback(ChangeBroadcaster*) override;
	void timerCallback(int id) override;
	//String getTooltip() override;
private:
	std::shared_ptr<breakpoint_envelope> m_envelope;
	String m_name;
	Colour m_env_color{ Colours::yellow };
	double m_view_start_time = 0.0;
	double m_view_end_time = 1.0;
	double m_view_start_value = 0.0;
	double m_view_end_value = 1.0;
	int find_hot_envelope_point(double xcor, double ycor);
	int findHotEnvelopeSegment(double xcor, double ycor, bool detectsegment);
	bool m_mouse_down = false;
	int m_node_to_drag = -1;
	std::pair<int, bool> m_segment_drag_info{ -1,false };
	int m_node_that_was_dragged = -1;
	String m_last_tip;
	BubbleMessageComponent m_bubble;
	void show_bubble(int x, int y, const envelope_node &node);
	CriticalSection* m_cs = nullptr;
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EnvelopeComponent)
};
