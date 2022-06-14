// SPDX-License-Identifier: GPLv3-or-later WITH Appstore-exception
// Copyright (C) 2021 Jesse Chappell

#include "OptionsView.h"



class PaulxstretchOptionsTabbedComponent : public TabbedComponent
{
public:
    PaulxstretchOptionsTabbedComponent(TabbedButtonBar::Orientation orientation, OptionsView & editor_) : TabbedComponent(orientation), editor(editor_) {

    }

    void currentTabChanged (int newCurrentTabIndex, const String& newCurrentTabName) override {

        editor.optionsTabChanged(newCurrentTabIndex);
    }

protected:
    OptionsView & editor;

};


enum {
    nameTextColourId = 0x1002830,
    selectedColourId = 0x1002840,
    separatorColourId = 0x1002850,
};




OptionsView::OptionsView(PaulstretchpluginAudioProcessor& proc, std::function<AudioDeviceManager*()> getaudiodevicemanager)
: Component(), getAudioDeviceManager(getaudiodevicemanager), processor(proc), smallLNF(14), sonoSliderLNF(13)
{
    setColour (nameTextColourId, Colour::fromFloatRGBA(1.0f, 1.0f, 1.0f, 0.9f));
    setColour (selectedColourId, Colour::fromFloatRGBA(0.0f, 0.4f, 0.8f, 0.5f));
    setColour (separatorColourId, Colour::fromFloatRGBA(0.3f, 0.3f, 0.3f, 0.3f));

    sonoSliderLNF.textJustification = Justification::centredRight;
    sonoSliderLNF.sliderTextJustification = Justification::centredRight;

    mOptionsComponent = std::make_unique<Component>();

    mSettingsTab = std::make_unique<TabbedComponent>(TabbedButtonBar::Orientation::TabsAtTop);
    mSettingsTab->setTabBarDepth(36);
    mSettingsTab->setOutline(0);
    mSettingsTab->getTabbedButtonBar().setMinimumTabScaleFactor(0.1f);
    //mSettingsTab->addComponentListener(this);

    
    mOptionsLoadFileWithPluginButton = std::make_unique<ToggleButton>(TRANS("Load file with plugin state"));
    mOptionsLoadFileWithPluginButton->addListener(this);
    mOptionsLoadFileWithPluginButton->onClick = [this] () {
        toggleBool(processor.m_load_file_with_state);
    };
    
    mOptionsPlayWithTransportButton = std::make_unique<ToggleButton>(TRANS("Play when host transport running"));
    mOptionsPlayWithTransportButton->onClick = [this] () {
        toggleBool(processor.m_play_when_host_plays);
    };

    mOptionsCaptureWithTransportButton = std::make_unique<ToggleButton>(TRANS("Capture when host transport running"));
    mOptionsCaptureWithTransportButton->onClick = [this] () {
        toggleBool(processor.m_capture_when_host_plays);
    };

    mOptionsRestorePlayStateButton = std::make_unique<ToggleButton>(TRANS("Restore playing state"));
    mOptionsRestorePlayStateButton->onClick = [this] () {
        toggleBool(processor.m_restore_playstate);
    };

    mOptionsMutePassthroughWhenCaptureButton = std::make_unique<ToggleButton>(TRANS("Mute passthrough while capturing"));
    mOptionsMutePassthroughWhenCaptureButton->onClick = [this] () {
        toggleBool(processor.m_mute_while_capturing);
    };

    mOptionsMuteProcessedWhenCaptureButton = std::make_unique<ToggleButton>(TRANS("Mute processed audio output while capturing"));
    mOptionsMuteProcessedWhenCaptureButton->onClick = [this] () {
        toggleBool(processor.m_mute_processed_while_capturing);
    };

    mOptionsSaveCaptureToDiskButton = std::make_unique<ToggleButton>(TRANS("Save captured audio to disk"));
    mOptionsSaveCaptureToDiskButton->onClick = [this] () {
        toggleBool(processor.m_save_captured_audio);
    };

    mOptionsEndRecordingAfterMaxButton = std::make_unique<ToggleButton>(TRANS("End recording after capturing max length"));
    mOptionsEndRecordingAfterMaxButton->onClick = [this] () {
        toggleBool(processor.m_auto_finish_record);
    };

    mOptionsSliderSnapToMouseButton = std::make_unique<ToggleButton>(TRANS("Sliders jump to position"));
    mOptionsSliderSnapToMouseButton->onClick = [this] () {
        toggleBool(processor.m_use_jumpsliders);
        if (updateSliderSnap)
            updateSliderSnap();
    };

    
    mOptionsShowTechnicalInfoButton = std::make_unique<ToggleButton>(TRANS("Show technical info in waveform"));
    mOptionsShowTechnicalInfoButton->onClick = [this] () {
        toggleBool(processor.m_show_technical_info);
    };

    

    mRecFormatChoice = std::make_unique<SonoChoiceButton>();
    mRecFormatChoice->addChoiceListener(this);
    mRecFormatChoice->addItem(TRANS("FLAC"), PaulstretchpluginAudioProcessor::FileFormatFLAC);
    mRecFormatChoice->addItem(TRANS("WAV"), PaulstretchpluginAudioProcessor::FileFormatWAV);
    mRecFormatChoice->addItem(TRANS("OGG"), PaulstretchpluginAudioProcessor::FileFormatOGG);

    mRecBitsChoice = std::make_unique<SonoChoiceButton>();
    mRecBitsChoice->addChoiceListener(this);
    mRecBitsChoice->addItem(TRANS("16 bit"), 16);
    mRecBitsChoice->addItem(TRANS("24 bit"), 24);
    mRecBitsChoice->addItem(TRANS("32 bit"), 32); 


    mRecFormatStaticLabel = std::make_unique<Label>("", TRANS("Recorded File Format:"));
    configLabel(mRecFormatStaticLabel.get(), false);
    mRecFormatStaticLabel->setJustificationType(Justification::centredRight);


    mRecLocationStaticLabel = std::make_unique<Label>("", TRANS("Record Location:"));
    configLabel(mRecLocationStaticLabel.get(), false);
    mRecLocationStaticLabel->setJustificationType(Justification::centredRight);

    mRecLocationButton = std::make_unique<TextButton>("fileloc");
    mRecLocationButton->setButtonText("");
    mRecLocationButton->setLookAndFeel(&smallLNF);
    mRecLocationButton->addListener(this);

    mOptionsCaptureBufferStaticLabel = std::make_unique<Label>("", TRANS("Capture Buffer Length:"));
    configLabel(mOptionsCaptureBufferStaticLabel.get(), false);
    mOptionsCaptureBufferStaticLabel->setJustificationType(Justification::centredRight);

    mCaptureBufferChoice = std::make_unique<SonoChoiceButton>();
    mCaptureBufferChoice->addChoiceListener(this);
    mCaptureBufferChoice->addItem(TRANS("2 seconds"), 2);
    mCaptureBufferChoice->addItem(TRANS("5 seconds"), 5);
    mCaptureBufferChoice->addItem(TRANS("10 seconds"), 10);
    mCaptureBufferChoice->addItem(TRANS("30 seconds"), 30);
    mCaptureBufferChoice->addItem(TRANS("60 seconds"), 60);
    mCaptureBufferChoice->addItem(TRANS("120 seconds"), 120);


    mOptionsDumpPresetToClipboardButton = std::make_unique<TextButton>("dump");
    mOptionsDumpPresetToClipboardButton->setButtonText(TRANS("Dump Preset to clipboard"));
    mOptionsDumpPresetToClipboardButton->setLookAndFeel(&smallLNF);
    mOptionsDumpPresetToClipboardButton->addListener(this);

    mOptionsResetParamsButton = std::make_unique<TextButton>("reset");
    mOptionsResetParamsButton->setButtonText(TRANS("Reset Parameters"));
    mOptionsResetParamsButton->setLookAndFeel(&smallLNF);
    mOptionsResetParamsButton->onClick = [this] () {
        processor.resetParameters();
    };

    
    
    addAndMakeVisible(mSettingsTab.get());

    mOptionsComponent->addAndMakeVisible(mCaptureBufferChoice.get());
    mOptionsComponent->addAndMakeVisible(mOptionsCaptureBufferStaticLabel.get());
    mOptionsComponent->addAndMakeVisible(mOptionsLoadFileWithPluginButton.get());
    mOptionsComponent->addAndMakeVisible(mOptionsPlayWithTransportButton.get());
    mOptionsComponent->addAndMakeVisible(mOptionsCaptureWithTransportButton.get());
    mOptionsComponent->addAndMakeVisible(mOptionsRestorePlayStateButton.get());

    mOptionsComponent->addAndMakeVisible(mOptionsMutePassthroughWhenCaptureButton.get());
    mOptionsComponent->addAndMakeVisible(mOptionsMuteProcessedWhenCaptureButton.get());
    mOptionsComponent->addAndMakeVisible(mOptionsSaveCaptureToDiskButton.get());
    mOptionsComponent->addAndMakeVisible(mOptionsEndRecordingAfterMaxButton.get());
    mOptionsComponent->addAndMakeVisible(mOptionsSliderSnapToMouseButton.get());
#if JUCE_DEBUG
    mOptionsComponent->addAndMakeVisible(mOptionsDumpPresetToClipboardButton.get());
#endif
    mOptionsComponent->addAndMakeVisible(mOptionsShowTechnicalInfoButton.get());
    mOptionsComponent->addAndMakeVisible(mOptionsResetParamsButton.get());

    mOptionsComponent->addAndMakeVisible(mOptionsSliderSnapToMouseButton.get());

    mOptionsComponent->addAndMakeVisible(mRecFormatChoice.get());
    mOptionsComponent->addAndMakeVisible(mRecFormatChoice.get());
    mOptionsComponent->addAndMakeVisible(mRecBitsChoice.get());
    mOptionsComponent->addAndMakeVisible(mRecFormatStaticLabel.get());
    mOptionsComponent->addAndMakeVisible(mRecLocationButton.get());
    mOptionsComponent->addAndMakeVisible(mRecLocationStaticLabel.get());


    if (JUCEApplicationBase::isStandaloneApp() && getAudioDeviceManager && getAudioDeviceManager())
    {
        if (!mAudioDeviceSelector) {
            int minNumInputs  = std::numeric_limits<int>::max(), maxNumInputs  = 0,
            minNumOutputs = std::numeric_limits<int>::max(), maxNumOutputs = 0;

            auto updateMinAndMax = [] (int newValue, int& minValue, int& maxValue)
            {
                minValue = jmin (minValue, newValue);
                maxValue = jmax (maxValue, newValue);
            };

            /*
            if (channelConfiguration.size() > 0)
            {
                auto defaultConfig =  channelConfiguration.getReference (0);
                updateMinAndMax ((int) defaultConfig.numIns,  minNumInputs,  maxNumInputs);
                updateMinAndMax ((int) defaultConfig.numOuts, minNumOutputs, maxNumOutputs);
            }
             */

            if (auto* bus = processor.getBus (true, 0)) {
                auto maxsup = bus->getMaxSupportedChannels(128);
                updateMinAndMax (maxsup, minNumInputs, maxNumInputs);
                updateMinAndMax (bus->getDefaultLayout().size(), minNumInputs, maxNumInputs);
                if (bus->isNumberOfChannelsSupported(1)) {
                    updateMinAndMax (1, minNumInputs, maxNumInputs);
                }
                if (bus->isNumberOfChannelsSupported(0)) {
                    updateMinAndMax (0, minNumInputs, maxNumInputs);
                }
            }

            if (auto* bus = processor.getBus (false, 0)) {
                auto maxsup = bus->getMaxSupportedChannels(128);
                updateMinAndMax (maxsup, minNumOutputs, maxNumOutputs);
                updateMinAndMax (bus->getDefaultLayout().size(), minNumOutputs, maxNumOutputs);
                if (bus->isNumberOfChannelsSupported(1)) {
                    updateMinAndMax (1, minNumOutputs, maxNumOutputs);
                }
                if (bus->isNumberOfChannelsSupported(0)) {
                    updateMinAndMax (0, minNumOutputs, maxNumOutputs);
                }
            }


            minNumInputs  = jmin (minNumInputs,  maxNumInputs);
            minNumOutputs = jmin (minNumOutputs, maxNumOutputs);



            mAudioDeviceSelector = std::make_unique<AudioDeviceSelectorComponent>(*getAudioDeviceManager(),
                                                                                  minNumInputs, maxNumInputs,
                                                                                  minNumOutputs, maxNumOutputs,
                                                                                  false, // show MIDI input
                                                                                  false,
                                                                                  false, false);

#if JUCE_IOS || JUCE_ANDROID
            mAudioDeviceSelector->setItemHeight(44);
#endif

            mAudioOptionsViewport = std::make_unique<Viewport>();
            mAudioOptionsViewport->setViewedComponent(mAudioDeviceSelector.get(), false);


        }

        if (firsttime) {
            mSettingsTab->addTab(TRANS("AUDIO"), Colour::fromFloatRGBA(0.1, 0.1, 0.1, 1.0), mAudioOptionsViewport.get(), false);
        }

    }

    createAbout();

    mOtherOptionsViewport = std::make_unique<Viewport>();
    mOtherOptionsViewport->setViewedComponent(mOptionsComponent.get(), false);

    mSettingsTab->addTab(TRANS("OPTIONS"),Colour::fromFloatRGBA(0.1, 0.1, 0.1, 1.0), mOtherOptionsViewport.get(), false);
    mSettingsTab->addTab(TRANS("ABOUT"), Colour::fromFloatRGBA(0.1, 0.1, 0.1, 1.0), mAboutViewport.get(), false);

    setFocusContainerType(FocusContainerType::keyboardFocusContainer);

    mSettingsTab->setFocusContainerType(FocusContainerType::none);
    mSettingsTab->getTabbedButtonBar().setFocusContainerType(FocusContainerType::none);
    mSettingsTab->getTabbedButtonBar().setWantsKeyboardFocus(true);
    mSettingsTab->setWantsKeyboardFocus(true);
    for (int i=0; i < mSettingsTab->getTabbedButtonBar().getNumTabs(); ++i) {
        if (auto tabbut = mSettingsTab->getTabbedButtonBar().getTabButton(i)) {
            tabbut->setRadioGroupId(3);
            tabbut->setWantsKeyboardFocus(true);
        }
        if (auto tabcomp = mSettingsTab->getTabContentComponent(i)) {
            tabcomp->setFocusContainerType(FocusContainerType::focusContainer);
        }
    }

}

OptionsView::~OptionsView() {}

juce::Rectangle<int> OptionsView::getMinimumContentBounds() const {
    int defWidth = 200;
    int defHeight = 100;
    return Rectangle<int>(0,0,defWidth,defHeight);
}

juce::Rectangle<int> OptionsView::getPreferredContentBounds() const
{
    return Rectangle<int> (0, 0, 300, prefHeight);
}


void OptionsView::timerCallback(int timerid)
{

}

void OptionsView::grabInitialFocus()
{
    if (auto * butt = mSettingsTab->getTabbedButtonBar().getTabButton(mSettingsTab->getCurrentTabIndex())) {
        butt->setWantsKeyboardFocus(true);
        butt->grabKeyboardFocus();
    }
}


void OptionsView::configLabel(Label *label, bool val)
{
    if (val) {
        label->setFont(12);
        label->setColour(Label::textColourId, Colour(0x90eeeeee));
        label->setJustificationType(Justification::centred);
    }
    else {
        label->setFont(14);
        //label->setColour(Label::textColourId, Colour(0xaaeeeeee));
        label->setJustificationType(Justification::centredLeft);
    }
}

void OptionsView::configLevelSlider(Slider * slider)
{
    //slider->setVelocityBasedMode(true);
    //slider->setVelocityModeParameters(2.5, 1, 0.05);
    //slider->setTextBoxStyle(Slider::NoTextBox, true, 40, 18);
    slider->setSliderStyle(Slider::SliderStyle::LinearHorizontal);
    slider->setTextBoxStyle(Slider::TextBoxAbove, true, 50, 14);
    slider->setMouseDragSensitivity(128);
    slider->setScrollWheelEnabled(false);
    //slider->setPopupDisplayEnabled(true, false, this);
    slider->setColour(Slider::textBoxBackgroundColourId, Colours::transparentBlack);
    slider->setColour(Slider::textBoxOutlineColourId, Colours::transparentBlack);
    slider->setColour(Slider::textBoxTextColourId, Colour(0x90eeeeee));
    slider->setColour(TooltipWindow::textColourId, Colour(0xf0eeeeee));

    slider->setLookAndFeel(&sonoSliderLNF);
}

void OptionsView::configEditor(TextEditor *editor, bool passwd)
{
    editor->addListener(this);
    if (passwd)  {
        editor->setIndents(8, 6);
    } else {
        editor->setIndents(8, 8);
    }
}

void OptionsView::updateState(bool ignorecheck)
{
    mRecFormatChoice->setSelectedId((int)processor.getDefaultRecordingFormat(), dontSendNotification);
    mRecBitsChoice->setSelectedId((int)processor.getDefaultRecordingBitsPerSample(), dontSendNotification);

    File recdir = File(processor.getDefaultRecordingDirectory());
    String dispath = recdir.getRelativePathFrom(File::getSpecialLocation (File::userHomeDirectory));
    if (dispath.startsWith(".")) dispath = processor.getDefaultRecordingDirectory();
    mRecLocationButton->setButtonText(dispath);
    
    mOptionsLoadFileWithPluginButton->setToggleState(processor.m_load_file_with_state, dontSendNotification);
    mOptionsPlayWithTransportButton->setToggleState(processor.m_play_when_host_plays, dontSendNotification);
    mOptionsCaptureWithTransportButton->setToggleState(processor.m_capture_when_host_plays, dontSendNotification);
    mOptionsRestorePlayStateButton->setToggleState(processor.m_restore_playstate, dontSendNotification);
    mOptionsMutePassthroughWhenCaptureButton->setToggleState(processor.m_mute_while_capturing, dontSendNotification);
    mOptionsMuteProcessedWhenCaptureButton->setToggleState(processor.m_mute_processed_while_capturing, dontSendNotification);
    mOptionsSaveCaptureToDiskButton->setToggleState(processor.m_save_captured_audio, dontSendNotification);
    mOptionsEndRecordingAfterMaxButton->setToggleState(processor.m_auto_finish_record, dontSendNotification);
    mOptionsSliderSnapToMouseButton->setToggleState(processor.m_use_jumpsliders, dontSendNotification);
    mOptionsShowTechnicalInfoButton->setToggleState(processor.m_show_technical_info, dontSendNotification);

    auto caplen = processor.getFloatParameter(cpi_max_capture_len)->get();
    mCaptureBufferChoice->setSelectedId((int)caplen, dontSendNotification);
}

void OptionsView::createAbout()
{
    mAboutLabel = std::make_unique<Label>();
    
    String fftlib;
#if PS_USE_VDSP_FFT
    fftlib = "vDSP";
#elif PS_USE_PFFFT
    fftlib = "pffft";
#else
    fftlib = fftwf_version;
#endif
    String juceversiontxt = String("JUCE ") + String(JUCE_MAJOR_VERSION) + "." + String(JUCE_MINOR_VERSION);
    String title = String(JucePlugin_Name) + " " + String(JucePlugin_VersionString);
#ifdef JUCE_DEBUG
    title += " (DEBUG)";
#endif
    String vstInfo;
    if (processor.wrapperType == AudioProcessor::wrapperType_VST ||
        processor.wrapperType == AudioProcessor::wrapperType_VST3)
        vstInfo = "VST Plug-In Technology by Steinberg.\n\n";
    PluginHostType host;

    String text = title + "\n\n" +
    "Plugin/Application for extreme time stretching and other sound processing\nBuilt on " + String(__DATE__) + " " + String(__TIME__) + "\n"
    "Copyright (C) 2006-2011 Nasca Octavian Paul, Tg. Mures, Romania\n"
    "(C) 2017-2021 Xenakios\n"
    "(C) 2022 Jesse Chappell\n\n"
    +vstInfo;

    if (fftlib.isNotEmpty())
        text += String("Using ") + fftlib + String(" for FFT\n\n");


#if !JUCE_IOS
    if (PluginHostType::getPluginLoadedAs() == AudioProcessor::wrapperType_AAX) {
        text += juceversiontxt + String("\n\n");
    }
    else {
        text += juceversiontxt + String(" used under the GPL license.\n\n");
    }
#endif

    text += String("GPL licensed source code at : https://github.com/essej/paulxstretch\n");

    if (host.type != juce::PluginHostType::UnknownHost) {
        text += String("Running in : ") + host.getHostDescription()+ String("\n");
    }

    mAboutLabel->setJustificationType(Justification::centred);
    mAboutLabel->setText(text, dontSendNotification);

    mAboutViewport = std::make_unique<Viewport>();
    mAboutViewport->setViewedComponent(mAboutLabel.get(), false);

    //std::unique_ptr<SettingsComponent> contptr(content);
    int defWidth = 450;
    int defHeight = 350;
#if JUCE_IOS
    defWidth = 320;
    defHeight = 350;
#endif

    mAboutLabel->setSize (defWidth, defHeight);

}



void OptionsView::resized()
{
    int leftmargin = 10;
    int minw = 100;
    int minKnobWidth = 50;
    int minSliderWidth = 50;
    int minPannerWidth = 40;
    int maxPannerWidth = 100;
    int minitemheight = 36;
    int knobitemheight = 80;
    int minpassheight = 30;
    int setitemheight = 36;
    int minButtonWidth = 90;
    int sliderheight = 44;
    int inmeterwidth = 22 ;
    int outmeterwidth = 22 ;
    int servLabelWidth = 72;
    int iconheight = 24;
    int iconwidth = iconheight;
    int knoblabelheight = 18;
    int panbuttwidth = 26;

#if JUCE_IOS || JUCE_ANDROID
    // make the button heights a bit more for touchscreen purposes
    minitemheight = 44;
    knobitemheight = 90;
    minpassheight = 38;
    panbuttwidth = 32;
#endif

    FlexBox mainBox;
    mainBox.flexDirection = FlexBox::Direction::column;
    mainBox.items.add(FlexItem(100, minitemheight, *mSettingsTab).withMargin(0).withFlex(1));
    
    mainBox.performLayout(getLocalBounds().reduced(2, 2));

    auto innerbounds = mSettingsTab->getLocalBounds();

    if (mAudioDeviceSelector) {
        mAudioDeviceSelector->setBounds(Rectangle<int>(0,0,innerbounds.getWidth() - 10,mAudioDeviceSelector->getHeight()));
    }
    mOptionsComponent->setBounds(Rectangle<int>(0,0,innerbounds.getWidth() - 10, minOptionsHeight));

    if (mAboutLabel) {
        mAboutLabel->setBounds(Rectangle<int>(0,0,innerbounds.getWidth() - 10, mAboutLabel->getHeight()));
    }

    
    FlexBox lfwpBox;
    lfwpBox.flexDirection = FlexBox::Direction::row;
    lfwpBox.items.add(FlexItem(leftmargin, 12).withFlex(0));
    lfwpBox.items.add(FlexItem(minw, minpassheight, *mOptionsLoadFileWithPluginButton).withMargin(0).withFlex(1));

    FlexBox pwtBox;
    pwtBox.flexDirection = FlexBox::Direction::row;
    pwtBox.items.add(FlexItem(leftmargin, 12).withFlex(0));
    pwtBox.items.add(FlexItem(minw, minpassheight, *mOptionsPlayWithTransportButton).withMargin(0).withFlex(1));

    FlexBox cwtBox;
    cwtBox.flexDirection = FlexBox::Direction::row;
    cwtBox.items.add(FlexItem(leftmargin, 12).withFlex(0));
    cwtBox.items.add(FlexItem(minw, minpassheight, *mOptionsCaptureWithTransportButton).withMargin(0).withFlex(1));

    FlexBox capbufBox;
    capbufBox.flexDirection = FlexBox::Direction::row;
    capbufBox.items.add(FlexItem(leftmargin, 12).withFlex(0));
    capbufBox.items.add(FlexItem(130, minitemheight, *mOptionsCaptureBufferStaticLabel).withMargin(0).withFlex(0));
    capbufBox.items.add(FlexItem(5, 12).withFlex(0));
    capbufBox.items.add(FlexItem(minw, minitemheight, *mCaptureBufferChoice).withMargin(0).withFlex(1));

    FlexBox rpsBox;
    rpsBox.flexDirection = FlexBox::Direction::row;
    rpsBox.items.add(FlexItem(leftmargin, 12).withFlex(0));
    rpsBox.items.add(FlexItem(minw, minpassheight, *mOptionsRestorePlayStateButton).withMargin(0).withFlex(1));

    
    FlexBox mpwcBox;
    mpwcBox.flexDirection = FlexBox::Direction::row;
    mpwcBox.items.add(FlexItem(leftmargin, 12).withFlex(0));
    mpwcBox.items.add(FlexItem(minw, minpassheight, *mOptionsMutePassthroughWhenCaptureButton).withMargin(0).withFlex(1));

    FlexBox mprwcBox;
    mprwcBox.flexDirection = FlexBox::Direction::row;
    mprwcBox.items.add(FlexItem(leftmargin, 12).withFlex(0));
    mprwcBox.items.add(FlexItem(minw, minpassheight, *mOptionsMuteProcessedWhenCaptureButton).withMargin(0).withFlex(1));

    FlexBox sctdBox;
    sctdBox.flexDirection = FlexBox::Direction::row;
    sctdBox.items.add(FlexItem(leftmargin, 12).withFlex(0));
    sctdBox.items.add(FlexItem(minw, minpassheight, *mOptionsSaveCaptureToDiskButton).withMargin(0).withFlex(1));

    FlexBox eramBox;
    eramBox.flexDirection = FlexBox::Direction::row;
    eramBox.items.add(FlexItem(leftmargin, 12).withFlex(0));
    eramBox.items.add(FlexItem(minw, minpassheight, *mOptionsEndRecordingAfterMaxButton).withMargin(0).withFlex(1));

    
    FlexBox ssnapBox;
    ssnapBox.flexDirection = FlexBox::Direction::row;
    ssnapBox.items.add(FlexItem(leftmargin, 12).withFlex(0));
    ssnapBox.items.add(FlexItem(minw, minpassheight, *mOptionsSliderSnapToMouseButton).withMargin(0).withFlex(1));

    FlexBox dumpBox;
    dumpBox.flexDirection = FlexBox::Direction::row;
    dumpBox.items.add(FlexItem(leftmargin, 12).withFlex(0));
    dumpBox.items.add(FlexItem(minw, minpassheight, *mOptionsDumpPresetToClipboardButton).withMargin(0).withFlex(1));

    FlexBox resetBox;
    resetBox.flexDirection = FlexBox::Direction::row;
    resetBox.items.add(FlexItem(leftmargin, 12).withFlex(0));
    resetBox.items.add(FlexItem(minw, minpassheight, *mOptionsResetParamsButton).withMargin(0).withFlex(1));

    FlexBox showtiBox;
    showtiBox.flexDirection = FlexBox::Direction::row;
    showtiBox.items.add(FlexItem(leftmargin, 12).withFlex(0));
    showtiBox.items.add(FlexItem(minw, minpassheight, *mOptionsShowTechnicalInfoButton).withMargin(0).withFlex(1));

    FlexBox optionsRecordDirBox;
    optionsRecordDirBox.flexDirection = FlexBox::Direction::row;
    optionsRecordDirBox.items.add(FlexItem(115, minitemheight, *mRecLocationStaticLabel).withMargin(0).withFlex(0));
    optionsRecordDirBox.items.add(FlexItem(minButtonWidth, minitemheight, *mRecLocationButton).withMargin(0).withFlex(3));

    FlexBox optionsRecordFormatBox;
    optionsRecordFormatBox.flexDirection = FlexBox::Direction::row;
    optionsRecordFormatBox.items.add(FlexItem(115, minitemheight, *mRecFormatStaticLabel).withMargin(0).withFlex(0));
    optionsRecordFormatBox.items.add(FlexItem(minButtonWidth, minitemheight, *mRecFormatChoice).withMargin(0).withFlex(1));
    optionsRecordFormatBox.items.add(FlexItem(2, 4));
    optionsRecordFormatBox.items.add(FlexItem(80, minitemheight, *mRecBitsChoice).withMargin(0).withFlex(0.25));
    
    int vgap = 0;
    
    FlexBox optionsBox;
    optionsBox.flexDirection = FlexBox::Direction::column;
    optionsBox.items.add(FlexItem(4, 6));
    optionsBox.items.add(FlexItem(minw, minpassheight, lfwpBox).withMargin(2).withFlex(0));
    optionsBox.items.add(FlexItem(4, vgap));
    optionsBox.items.add(FlexItem(minw, minpassheight, pwtBox).withMargin(2).withFlex(0));
    optionsBox.items.add(FlexItem(4, vgap));
    optionsBox.items.add(FlexItem(minw, minpassheight, cwtBox).withMargin(2).withFlex(0));
    optionsBox.items.add(FlexItem(4, vgap));
    optionsBox.items.add(FlexItem(minw, minpassheight, rpsBox).withMargin(2).withFlex(0));
    optionsBox.items.add(FlexItem(4, vgap + 6));
    
    optionsBox.items.add(FlexItem(minw, minitemheight, capbufBox).withMargin(2).withFlex(0));
    optionsBox.items.add(FlexItem(4, vgap));
    optionsBox.items.add(FlexItem(minw, minpassheight, mpwcBox).withMargin(2).withFlex(0));
    optionsBox.items.add(FlexItem(4, vgap));
    optionsBox.items.add(FlexItem(minw, minpassheight, mprwcBox).withMargin(2).withFlex(0));
    optionsBox.items.add(FlexItem(4, vgap));
    optionsBox.items.add(FlexItem(minw, minpassheight, sctdBox).withMargin(2).withFlex(0));
    optionsBox.items.add(FlexItem(4, vgap));
    optionsBox.items.add(FlexItem(minw, minpassheight, eramBox).withMargin(2).withFlex(0));
    optionsBox.items.add(FlexItem(4, vgap + 6));

    optionsBox.items.add(FlexItem(minw, minpassheight, ssnapBox).withMargin(2).withFlex(0));
    optionsBox.items.add(FlexItem(4, vgap));
    optionsBox.items.add(FlexItem(minw, minpassheight, showtiBox).withMargin(2).withFlex(0));
    optionsBox.items.add(FlexItem(4, vgap + 6));

#if !(JUCE_IOS || JUCE_ANDROID)
    optionsBox.items.add(FlexItem(minw, minitemheight, optionsRecordDirBox).withMargin(2).withFlex(0));
    optionsBox.items.add(FlexItem(4, vgap + 2));
#endif
    optionsBox.items.add(FlexItem(minw, minitemheight, optionsRecordFormatBox).withMargin(2).withFlex(0));
    optionsBox.items.add(FlexItem(4, vgap));

    optionsBox.items.add(FlexItem(4, vgap + 4));
    optionsBox.items.add(FlexItem(minw, minpassheight, resetBox).withMargin(2).withFlex(0));
    optionsBox.items.add(FlexItem(4, vgap));

#if JUCE_DEBUG
    optionsBox.items.add(FlexItem(4, vgap + 4));
    optionsBox.items.add(FlexItem(minw, minpassheight, dumpBox).withMargin(2).withFlex(0));
    optionsBox.items.add(FlexItem(4, vgap));
#endif

    minOptionsHeight = 0;
    for (auto & item : optionsBox.items) {
        minOptionsHeight += item.minHeight + item.margin.top + item.margin.bottom;
    }

    optionsBox.performLayout(mOptionsComponent->getLocalBounds());

    prefHeight = minOptionsHeight + mSettingsTab->getTabBarDepth();
}

void OptionsView::showAudioTab()
{
    if (mSettingsTab->getNumTabs() == 3) {
        mSettingsTab->setCurrentTabIndex(0);
    }
}

void OptionsView::showOptionsTab()
{
    mSettingsTab->setCurrentTabIndex(mSettingsTab->getNumTabs() == 3 ? 1 : 0);
}

void OptionsView::showAboutTab()
{
    mSettingsTab->setCurrentTabIndex(mSettingsTab->getNumTabs() == 3 ? 2 : 1);
}


void OptionsView::optionsTabChanged (int newCurrentTabIndex)
{

}

void OptionsView::showWarnings()
{
}

void OptionsView::textEditorReturnKeyPressed (TextEditor& ed)
{
    DBG("Return pressed");
   // if (&ed == mOptionsUdpPortEditor.get()) {
   //     int port = mOptionsUdpPortEditor->getText().getIntValue();
   //     //changeUdpPort(port);
   // }
}

void OptionsView::textEditorEscapeKeyPressed (TextEditor& ed)
{
    DBG("escape pressed");
}

void OptionsView::textEditorTextChanged (TextEditor& ed)
{

}


void OptionsView::textEditorFocusLost (TextEditor& ed)
{
    // only one we care about live is udp port
    //if (&ed == mOptionsUdpPortEditor.get()) {
    //    int port = mOptionsUdpPortEditor->getText().getIntValue();
    //    //changeUdpPort(port);
    //}
}


void OptionsView::buttonClicked (Button* buttonThatWasClicked)
{
    if (buttonThatWasClicked == mRecLocationButton.get()) {
        // browse folder chooser
        SafePointer<OptionsView> safeThis (this);

        if (! RuntimePermissions::isGranted (RuntimePermissions::readExternalStorage))
        {
            RuntimePermissions::request (RuntimePermissions::readExternalStorage,
                                         [safeThis] (bool granted) mutable
                                         {
                if (granted)
                    safeThis->buttonClicked (safeThis->mRecLocationButton.get());
            });
            return;
        }

        chooseRecDirBrowser();
    }
    else if (buttonThatWasClicked == mOptionsDumpPresetToClipboardButton.get()) {
        ValueTree tree = processor.getStateTree(true, true);
        MemoryBlock destData;
        MemoryOutputStream stream(destData, true);
        tree.writeToStream(stream);
        String txt = Base64::toBase64(destData.getData(), destData.getSize());
        SystemClipboard::copyTextToClipboard(txt);
    }
}


void OptionsView::choiceButtonSelected(SonoChoiceButton *comp, int index, int ident)
{
    if (comp == mRecFormatChoice.get()) {
        processor.setDefaultRecordingFormat((PaulstretchpluginAudioProcessor::RecordFileFormat) ident);
    }
    else if (comp == mRecBitsChoice.get()) {
        processor.setDefaultRecordingBitsPerSample(ident);
    }
    else if (comp == mCaptureBufferChoice.get()) {
        *processor.getFloatParameter(cpi_max_capture_len) = (float) ident;
    }

}

void OptionsView::chooseRecDirBrowser()
{
    SafePointer<OptionsView> safeThis (this);

    if (FileChooser::isPlatformDialogAvailable())
    {
        File recdir = File(processor.getDefaultRecordingDirectory());

        mFileChooser.reset(new FileChooser(TRANS("Choose the folder for new recordings"),
                                           recdir,
                                           "",
                                           true, false, getTopLevelComponent()));



        mFileChooser->launchAsync (FileBrowserComponent::openMode | FileBrowserComponent::canSelectDirectories,
                                   [safeThis] (const FileChooser& chooser) mutable
                                   {
            auto results = chooser.getURLResults();
            if (safeThis != nullptr && results.size() > 0)
            {
                auto url = results.getReference (0);

                DBG("Chose directory: " <<  url.toString(false));

                if (url.isLocalFile()) {
                    File lfile = url.getLocalFile();
                    if (lfile.isDirectory()) {
                        safeThis->processor.setDefaultRecordingDirectory(lfile.getFullPathName());
                    } else {
                        safeThis->processor.setDefaultRecordingDirectory(lfile.getParentDirectory().getFullPathName());
                    }

                    safeThis->updateState();
                }
            }

            if (safeThis) {
                safeThis->mFileChooser.reset();
            }

        }, nullptr);

    }
    else {
        DBG("Need to enable code signing");
    }
}



void OptionsView::showPopTip(const String & message, int timeoutMs, Component * target, int maxwidth)
{
    popTip.reset(new BubbleMessageComponent());
    popTip->setAllowedPlacement(BubbleComponent::above);
    
    if (target) {
        if (auto * parent = target->findParentComponentOfClass<AudioProcessorEditor>()) {
            parent->addChildComponent (popTip.get());
        } else {
            addChildComponent(popTip.get());            
        }
    }
    else {
        addChildComponent(popTip.get());
    }
    
    AttributedString text(message);
    text.setJustification (Justification::centred);
    text.setColour (findColour (TextButton::textColourOffId));
    text.setFont(Font(12));
    if (target) {
        popTip->showAt(target, text, timeoutMs);
    }
    else {
        Rectangle<int> topbox(getWidth()/2 - maxwidth/2, 0, maxwidth, 2);
        popTip->showAt(topbox, text, timeoutMs);
    }
    popTip->toFront(false);
    //AccessibilityHandler::postAnnouncement(message, AccessibilityHandler::AnnouncementPriority::high);
}

void OptionsView::paint(Graphics & g)
{
    /*
    //g.fillAll (Colours::black);
    Rectangle<int> bounds = getLocalBounds();

    bounds.reduce(1, 1);
    bounds.removeFromLeft(3);
    
    g.setColour(bgColor);
    g.fillRoundedRectangle(bounds.toFloat(), 6.0f);
    g.setColour(outlineColor);
    g.drawRoundedRectangle(bounds.toFloat(), 6.0f, 0.5f);
*/
}

