#pragma once

/*
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

#include "../JuceLibraryCode/JuceHeader.h"
class PaulstretchpluginAudioProcessor;

class RenderSettingsComponent  : public Component,
                                 public ComboBox::Listener,
                                 public Button::Listener,
								 public	TextEditor::Listener
{
public:
    //==============================================================================
    RenderSettingsComponent (PaulstretchpluginAudioProcessor* mc);
    ~RenderSettingsComponent();

    void paint (Graphics& g) override;
    void resized() override;
    void comboBoxChanged (ComboBox* comboBoxThatHasChanged) override;
    void buttonClicked (Button* buttonThatWasClicked) override;
	int getPreferredHeight();
	void textEditorTextChanged(TextEditor& ed) override;
private:
	PaulstretchpluginAudioProcessor * m_proc = nullptr;
    Label labelSamplerate;
    ComboBox comboBoxSamplerate;
    Label labelBitDepth;
    ComboBox comboBoxBitDepth;
    TextButton buttonRender;
    Label label3;
    TextEditor numLoopsEditor;
    Label label4;
    TextEditor outfileNameEditor;
    TextButton buttonSelectFile;
	Label m_labelMaxOutDuration;
	TextEditor m_editorMaxOutDuration;
	ToggleButton m_toggleFloatClip;
	//==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RenderSettingsComponent)
};
