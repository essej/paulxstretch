// SPDX-License-Identifier: GPLv3-or-later WITH Appstore-exception
// Copyright (C) 2021 Jesse Chappell


#pragma once

#include "JuceHeader.h"

#include "PluginProcessor.h"
#include "CustomLookAndFeel.h"
#include "SonoChoiceButton.h"
#include "GenericItemChooser.h"


class OptionsView :
public Component,
public Button::Listener,
public SonoChoiceButton::Listener,
public GenericItemChooser::Listener,
public TextEditor::Listener,
public MultiTimer
{
public:
    OptionsView(PaulstretchpluginAudioProcessor& proc, std::function<AudioDeviceManager*()> getaudiodevicemanager);
    virtual ~OptionsView();


    class Listener {
    public:
        virtual ~Listener() {}
        virtual void optionsChanged(OptionsView *comp) {}
    };

    void addListener(Listener * listener) { listeners.add(listener); }
    void removeListener(Listener * listener) { listeners.remove(listener); }

    void timerCallback(int timerid) override;

    void buttonClicked (Button* buttonThatWasClicked) override;

    void choiceButtonSelected(SonoChoiceButton *comp, int index, int ident) override;


    void textEditorReturnKeyPressed (TextEditor&) override;
    void textEditorEscapeKeyPressed (TextEditor&) override;
    void textEditorTextChanged (TextEditor&) override;
    void textEditorFocusLost (TextEditor&) override;

    juce::Rectangle<int> getMinimumContentBounds() const;
    juce::Rectangle<int> getPreferredContentBounds() const;

    void grabInitialFocus();

    void updateState(bool ignorecheck=false);

    void paint(Graphics & g) override;
    void resized() override;

    void showPopTip(const String & message, int timeoutMs, Component * target, int maxwidth=100);

    void optionsTabChanged (int newCurrentTabIndex);

    void showAudioTab();
    void showOptionsTab();
    void showAboutTab();

    void showWarnings();


    std::function<AudioDeviceManager*()> getAudioDeviceManager; // = []() { return 0; };
    std::function<void()> updateSliderSnap; // = []() { return 0; };
    std::function<void()> updateKeybindings; // = []() { return 0; };
    std::function<void()> saveSettingsIfNeeded; // = []() { return 0; };


protected:


    void configEditor(TextEditor *editor, bool passwd = false);
    void configLabel(Label *label, bool val=false);
    void configLevelSlider(Slider *);

    void chooseRecDirBrowser();
    void createAbout();


    PaulstretchpluginAudioProcessor& processor;

    CustomBigTextLookAndFeel smallLNF;
    CustomBigTextLookAndFeel sonoSliderLNF;


    ListenerList<Listener> listeners;

    std::unique_ptr<AudioDeviceSelectorComponent> mAudioDeviceSelector;
    std::unique_ptr<Viewport> mAudioOptionsViewport;
    std::unique_ptr<Viewport> mOtherOptionsViewport;
    std::unique_ptr<Viewport> mRecordOptionsViewport;


    std::unique_ptr<Component> mOptionsComponent;
    std::unique_ptr<Viewport> mAboutViewport;
    std::unique_ptr<Label> mAboutLabel;

    int minOptionsHeight = 0;
    int minRecOptionsHeight = 0;

    uint32 settingsClosedTimestamp = 0;

    std::unique_ptr<FileChooser> mFileChooser;

    std::unique_ptr<SonoChoiceButton> mCaptureBufferChoice;
    std::unique_ptr<Label>  mOptionsCaptureBufferStaticLabel;

    std::unique_ptr<ToggleButton> mOptionsLoadFileWithPluginButton;
    std::unique_ptr<ToggleButton> mOptionsPlayWithTransportButton;
    std::unique_ptr<ToggleButton> mOptionsCaptureWithTransportButton;
    std::unique_ptr<ToggleButton> mOptionsRestorePlayStateButton;
    
    std::unique_ptr<ToggleButton> mOptionsMutePassthroughWhenCaptureButton;
    std::unique_ptr<ToggleButton> mOptionsMuteProcessedWhenCaptureButton;
    std::unique_ptr<ToggleButton> mOptionsSaveCaptureToDiskButton;
    std::unique_ptr<ToggleButton> mOptionsEndRecordingAfterMaxButton;

    std::unique_ptr<ToggleButton> mOptionsSliderSnapToMouseButton;
    std::unique_ptr<TextButton> mOptionsDumpPresetToClipboardButton;
    std::unique_ptr<ToggleButton> mOptionsShowTechnicalInfoButton;
    std::unique_ptr<TextButton> mOptionsResetParamsButton;

    std::unique_ptr<SonoChoiceButton> mRecFormatChoice;
    std::unique_ptr<SonoChoiceButton> mRecBitsChoice;
    std::unique_ptr<Label> mRecFormatStaticLabel;
    std::unique_ptr<Label> mRecLocationStaticLabel;
    std::unique_ptr<TextButton> mRecLocationButton;

    std::unique_ptr<TabbedComponent> mSettingsTab;

    int minHeight = 0;
    int prefHeight = 0;
    bool firsttime = true;

    // keep this down here, so it gets destroyed early
    std::unique_ptr<BubbleMessageComponent> popTip;


    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OptionsView)

};
