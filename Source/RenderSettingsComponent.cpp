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


#include "PluginProcessor.h"
//extern std::unique_ptr<PropertiesFile> g_propsfile;
#include "RenderSettingsComponent.h"

RenderSettingsComponent::RenderSettingsComponent (PaulstretchpluginAudioProcessor* mc)
{
    m_proc = mc;
	addAndMakeVisible(&m_labelMaxOutDuration);
	m_labelMaxOutDuration.setText("Max output duration (hours) :", dontSendNotification);
    m_labelMaxOutDuration.setJustificationType(Justification::centredRight);
	addAndMakeVisible(&m_editorMaxOutDuration);
	m_editorMaxOutDuration.setText("1.0", dontSendNotification);
	addAndMakeVisible(&m_toggleFloatClip);
	m_toggleFloatClip.setButtonText("Clip");
	m_toggleFloatClip.setToggleState(false, dontSendNotification);
	addAndMakeVisible(&labelSamplerate);
	labelSamplerate.setText("Sample rate :", dontSendNotification);
    labelSamplerate.setJustificationType(Justification::centredRight);
	addAndMakeVisible(&comboBoxSamplerate);
    comboBoxSamplerate.addItem("Source sample rate", 1);
	comboBoxSamplerate.addItem("44100", 44100);
	comboBoxSamplerate.addItem("48000", 48000);
	comboBoxSamplerate.addItem("88200", 88200);
	comboBoxSamplerate.addItem("96000", 96000);

    comboBoxSamplerate.addListener (this);

	addAndMakeVisible(&labelBitDepth);
	labelBitDepth.setText("Format :", dontSendNotification);
    labelBitDepth.setJustificationType(Justification::centredRight);
	addAndMakeVisible(&comboBoxBitDepth);
    comboBoxBitDepth.addItem (TRANS("16 bit PCM"), 1);
    comboBoxBitDepth.addItem (TRANS("24 bit PCM"), 2);
    comboBoxBitDepth.addItem (TRANS("32 bit floating point"), 3);
    comboBoxBitDepth.addListener (this);

	addAndMakeVisible(&buttonRender);
    buttonRender.setButtonText (TRANS("Render"));
    buttonRender.addListener (this);

	addAndMakeVisible(&label3);
	label3.setText("Number of loops\n(approximate) :", dontSendNotification);
	addAndMakeVisible(&numLoopsEditor);
    numLoopsEditor.setMultiLine (false);
    numLoopsEditor.setReturnKeyStartsNewLine (false);
    numLoopsEditor.setReadOnly (false);
    numLoopsEditor.setCaretVisible (true);
    numLoopsEditor.setText (TRANS("1"));

	addAndMakeVisible(&label4);
	label4.setText("Output file :\n", dontSendNotification);
    label4.setJustificationType(Justification::centredRight);

#if JUCE_IOS
    addAndMakeVisible(&m_shareAfterRenderToggle);
    m_shareAfterRenderToggle.setButtonText("Share after render");
    bool lastshare = m_proc->m_propsfile->m_props_file->getBoolValue(ID_lastrendershare, false);
    m_shareAfterRenderToggle.setToggleState(lastshare, dontSendNotification);
#endif

	addAndMakeVisible(&outfileNameEditor);
    outfileNameEditor.setMultiLine (false);
    outfileNameEditor.setReturnKeyStartsNewLine (false);
    outfileNameEditor.setReadOnly (false);
    outfileNameEditor.setScrollbarsShown (true);
    outfileNameEditor.setCaretVisible (true);
	outfileNameEditor.addListener(this);

	addAndMakeVisible(&buttonSelectFile);
	buttonSelectFile.setTooltip("Open dialog to choose file to render to");
	buttonSelectFile.setButtonText (TRANS("..."));
    buttonSelectFile.addListener (this);
	setSize (prefWidth, prefHeight);
	comboBoxSamplerate.setSelectedId(1);
    comboBoxBitDepth.setSelectedId(3);
	String lastexportfile = m_proc->m_propsfile->m_props_file->getValue(ID_lastrenderpath);
	auto sep = File::getSeparatorChar();
	File temp(lastexportfile);

#if JUCE_IOS
    outfileNameEditor.setText(temp.getFileName(), dontSendNotification);
#else
	if (temp.getParentDirectory().exists())
		outfileNameEditor.setText(lastexportfile, dontSendNotification);
	else
		outfileNameEditor.setText(File::getSpecialLocation(File::userDocumentsDirectory).getFullPathName()+sep+"pxsrender.wav",
			dontSendNotification);
#endif

    numLoopsEditor.setVisible(m_proc->getStretchSource()->isLoopingEnabled());
	label3.setVisible(m_proc->getStretchSource()->isLoopingEnabled());
}

RenderSettingsComponent::~RenderSettingsComponent()
{
	//g_propsfile->setValue("last_export_file",outfileNameEditor.getText());
}

//==============================================================================
void RenderSettingsComponent::paint (Graphics& g)
{
    //g.fillAll (Colour (0xff323e44));
    g.fillAll(Colour::fromFloatRGBA(0.1, 0.1, 0.1, 1.0));
}

void RenderSettingsComponent::resized()
{
	int labelw = 100;
    int medlabelw = 150;
    int widelabelw = 210;
	int itemh = 28;
    int tallitemh = 40;
    int minitemw = 150;
    int smallitemw = 50;
    int margin = 2;
#if JUCE_IOS
    itemh = 36;
    tallitemh = 42;
#endif


    FlexBox mainbox;
    mainbox.flexDirection = FlexBox::Direction::column;


    FlexBox outbox;
    outbox.flexDirection = FlexBox::Direction::row;
    outbox.items.add(FlexItem(labelw, itemh, label4).withMargin(margin));
    outbox.items.add(FlexItem(minitemw, itemh, outfileNameEditor).withMargin(margin).withFlex(1));
#if !JUCE_IOS
    outbox.items.add(FlexItem(36, itemh, buttonSelectFile).withMargin(margin));
#endif

    FlexBox srbox;
    srbox.flexDirection = FlexBox::Direction::row;
    srbox.items.add(FlexItem(labelw, itemh, labelSamplerate).withMargin(margin));
    srbox.items.add(FlexItem(minitemw, itemh, comboBoxSamplerate).withMargin(margin));

    FlexBox forbox;
    forbox.flexDirection = FlexBox::Direction::row;
    forbox.items.add(FlexItem(labelw, itemh, labelBitDepth).withMargin(margin));
    forbox.items.add(FlexItem(minitemw, itemh, comboBoxBitDepth).withMargin(margin));
    forbox.items.add(FlexItem(minitemw, itemh, m_toggleFloatClip).withMargin(margin).withFlex(1));

    FlexBox loopbox;
    loopbox.flexDirection = FlexBox::Direction::row;
    loopbox.items.add(FlexItem(medlabelw, tallitemh, label3).withMargin(margin));
    loopbox.items.add(FlexItem(smallitemw, itemh, numLoopsEditor).withMargin(margin).withMaxHeight(itemh).withAlignSelf(FlexItem::AlignSelf::flexStart));

    FlexBox maxbox;
    maxbox.flexDirection = FlexBox::Direction::row;
    maxbox.items.add(FlexItem(widelabelw, itemh, m_labelMaxOutDuration).withMargin(margin));
    maxbox.items.add(FlexItem(smallitemw, itemh, m_editorMaxOutDuration).withMargin(margin));

    FlexBox buttonbox;
    buttonbox.flexDirection = FlexBox::Direction::row;
    buttonbox.items.add(FlexItem(2, itemh).withFlex(1));
#if JUCE_IOS
    buttonbox.items.add(FlexItem(labelw, itemh, m_shareAfterRenderToggle).withMargin(margin).withFlex(1));
    buttonbox.items.add(FlexItem(4, itemh).withFlex(0.1).withMaxWidth(20));
#endif
    buttonbox.items.add(FlexItem(minitemw, itemh, buttonRender).withMargin(margin));



    mainbox.items.add(FlexItem(minitemw, itemh, outbox).withMargin(margin));
    mainbox.items.add(FlexItem(minitemw, itemh, srbox).withMargin(margin));
    mainbox.items.add(FlexItem(minitemw, itemh, forbox).withMargin(margin));

    if (m_proc->getStretchSource()->isLoopingEnabled()) {
        mainbox.items.add(FlexItem(minitemw, tallitemh, loopbox).withMargin(margin));
    }

    mainbox.items.add(FlexItem(minitemw, itemh, maxbox).withMargin(margin));
    mainbox.items.add(FlexItem(minitemw, 2).withFlex(0.1));
    mainbox.items.add(FlexItem(minitemw, itemh, buttonbox).withMargin(margin));

    mainbox.performLayout(getLocalBounds().reduced(2));

    prefHeight = 4;

    for (auto & item : mainbox.items) {
        prefHeight += item.minHeight + item.margin.top + item.margin.bottom;
    }
}

void RenderSettingsComponent::comboBoxChanged (ComboBox* comboBoxThatHasChanged)
{
	if (comboBoxThatHasChanged == &comboBoxBitDepth)
	{
		if (comboBoxBitDepth.getSelectedId() == 3)
			m_toggleFloatClip.setEnabled(true);
		else m_toggleFloatClip.setEnabled(false);
	}

}

void RenderSettingsComponent::buttonClicked (Button* buttonThatWasClicked)
{
    if (buttonThatWasClicked == &buttonRender)
    {
        File outfile;

#if JUCE_IOS
        // force outfile to be in Documents
        outfile = File::getSpecialLocation(File::SpecialLocationType::userDocumentsDirectory).getChildFile(outfileNameEditor.getText());
#else
        outfile = File(outfileNameEditor.getText());
#endif

        if (!pendingRender && outfile.getParentDirectory().exists()==false) {
			buttonClicked(&buttonSelectFile);
            pendingRender = true;
            return;
        }
        else if (outfile.getParentDirectory().exists()==false) {
            pendingRender = false;
            return;
        }

        if (!outfile.getFileExtension().equalsIgnoreCase(".wav")) {
            outfile = outfile.withFileExtension(".wav");
        }

		int numLoops = 0; 
		if (numLoopsEditor.isVisible())
			numLoops = (int) numLoopsEditor.getText().getLargeIntValue();
		numLoops = jlimit<int>(0, 1000000, numLoops);
		int sampleRate = comboBoxSamplerate.getSelectedId();
		if (sampleRate == 1)
			sampleRate = 0;
		double maxrenderlen = m_editorMaxOutDuration.getText().getDoubleValue()*3600.0;
		maxrenderlen = jlimit(1.0, 1000000.0, maxrenderlen);
		int oformat = comboBoxBitDepth.getSelectedId() - 1;
		if (oformat == 2 && m_toggleFloatClip.getToggleState())
			oformat = 3;

        std::function<void(bool,File file)> completion;

#if JUCE_IOS
        if (m_shareAfterRenderToggle.getToggleState()) {
            completion = [](bool status,File file) {
                // this completion handler will be called from another thread
                MessageManager::callAsync([status,file]() {

                    if (status) {
                        DBG("Finished render, sharing");
                        Array<URL> files;
                        files.add(URL(file));

                        ContentSharer::getInstance()->shareFiles(files, [](bool status, const String & message) {
                            if (status) {
                                DBG("Finished share");
                            } else {
                                DBG("Error sharing: " << message);
                            }
                        });
                    } else {
                        DBG("error rendering");
                    }
                });
            };
        }
#endif

		OfflineRenderParams renderpars{ outfile,(double)comboBoxSamplerate.getSelectedId(),
			oformat,maxrenderlen,numLoops, nullptr, completion };
		m_proc->m_propsfile->m_props_file->setValue(ID_lastrenderpath, outfile.getFullPathName());
        m_proc->m_propsfile->m_props_file->setValue(ID_lastrendershare, m_shareAfterRenderToggle.getToggleState());
		m_proc->offlineRender(renderpars);
		if (auto pardlg = dynamic_cast<CallOutBox*>(getParentComponent()); pardlg!=nullptr)
		{
            pardlg->dismiss();
            //pardlg->exitModalState(1);
		}
		return;
    }
    else if (buttonThatWasClicked == &buttonSelectFile)
    {
		File lastexportfolder; // File(g_propsfile->getValue("last_export_file")).getParentDirectory();

        m_filechooser = std::make_unique<FileChooser>("Please select audio file to render...",
                                                      lastexportfolder,
                                                      "*.wav");
        m_filechooser->launchAsync(FileBrowserComponent::saveMode, [this](const FileChooser &chooser) {
            String newpath = chooser.getResult().getFullPathName();
#if JUCE_IOS
            // not actually used here, but just in case for later
            newpath = chooser.getResult().getFileName();
#endif
			outfileNameEditor.setText(newpath, dontSendNotification);
            if (newpath.isNotEmpty() && pendingRender) {
                buttonClicked(&buttonRender);
            }
        });
    }
}

int RenderSettingsComponent::getPreferredHeight() const
{
    return prefHeight;

#if JUCE_IOS
    if (m_proc->getStretchSource()->isLoopingEnabled())
        return 300;
    return 260;
#else
    if (m_proc->getStretchSource()->isLoopingEnabled())
        return 230;
    return 190;
#endif
}

int RenderSettingsComponent::getPreferredWidth() const
{
    return prefWidth;
}


void RenderSettingsComponent::textEditorTextChanged(TextEditor & ed)
{
	return;
}

