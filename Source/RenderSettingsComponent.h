// SPDX-License-Identifier: GPLv3-or-later WITH Appstore-exception
// Copyright (C) 2017 Xenakios
// Copyright (C) 2020 Jesse Chappell

#pragma once

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
	int getPreferredHeight() const;
    int getPreferredWidth() const;
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
    ToggleButton m_shareAfterRenderToggle;
	String ID_lastrenderpath{ "lastrenderpath" };
    String ID_lastrendershare{ "lastrendershare" };
    int prefHeight = 400;
    int prefWidth = 480;
    std::unique_ptr<FileChooser> m_filechooser;
    bool pendingRender = false;
	//==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RenderSettingsComponent)
};
