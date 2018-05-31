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

#include "PluginProcessor.h"
//extern std::unique_ptr<PropertiesFile> g_propsfile;
#include "RenderSettingsComponent.h"

RenderSettingsComponent::RenderSettingsComponent (PaulstretchpluginAudioProcessor* mc)
{
    m_proc = mc;
	addAndMakeVisible(&m_labelMaxOutDuration);
	m_labelMaxOutDuration.setText("Max output duration (hours) :", dontSendNotification);
	addAndMakeVisible(&m_editorMaxOutDuration);
	m_editorMaxOutDuration.setText("1.0", dontSendNotification);
	addAndMakeVisible(&m_toggleFloatClip);
	m_toggleFloatClip.setButtonText("Clip floating point output");
	m_toggleFloatClip.setToggleState(false, dontSendNotification);
	addAndMakeVisible(&labelSamplerate);
	labelSamplerate.setText("Sample rate :", dontSendNotification);
	addAndMakeVisible(&comboBoxSamplerate);
    comboBoxSamplerate.addItem("Source sample rate", 1);
	comboBoxSamplerate.addItem("44100", 44100);
	comboBoxSamplerate.addItem("48000", 48000);
	comboBoxSamplerate.addItem("88200", 88200);
	comboBoxSamplerate.addItem("96000", 96000);

    comboBoxSamplerate.addListener (this);

	addAndMakeVisible(&labelBitDepth);
	labelBitDepth.setText("Format :", dontSendNotification);
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
	setSize (600, 400);
	comboBoxSamplerate.setSelectedId(1);
    comboBoxBitDepth.setSelectedId(3);
	String lastexportfile = ""; // g_propsfile->getValue("last_export_file");
	auto sep = File::getSeparatorChar();
	File temp(lastexportfile);
	if (temp.getParentDirectory().exists())
		outfileNameEditor.setText(lastexportfile, dontSendNotification);
	else
		outfileNameEditor.setText(File::getSpecialLocation(File::userDocumentsDirectory).getFullPathName()+sep+"untitled.wav",
			dontSendNotification);
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
    g.fillAll (Colour (0xff323e44));
}

void RenderSettingsComponent::resized()
{
	int xoffs = 8;
	int yoffs = 1;
	int labelw = 160;
	int labelh = 24;
	
	label4.setBounds(xoffs, yoffs, labelw, 24);
	outfileNameEditor.setBounds(label4.getRight()+1, yoffs, getWidth() - labelw - 34 - xoffs, 24);
	buttonSelectFile.setBounds(outfileNameEditor.getRight() + 1, yoffs, 31, 24);
	yoffs += 25;
	labelSamplerate.setBounds (xoffs, yoffs, labelw, labelh);
    comboBoxSamplerate.setBounds (labelSamplerate.getRight()+1, yoffs, 150, 24);
	yoffs += 25;
	labelBitDepth.setBounds (xoffs, yoffs, labelw, 24);
    comboBoxBitDepth.setBounds (labelBitDepth.getRight()+1, yoffs, 150, 24);
	m_toggleFloatClip.setBounds(comboBoxBitDepth.getRight() + 1, yoffs, 10, 24);
	m_toggleFloatClip.changeWidthToFitText();
	yoffs += 25;
	if (m_proc->getStretchSource()->isLoopingEnabled())
	{
		label3.setBounds(xoffs, yoffs, labelw, 48);
		numLoopsEditor.setBounds(label3.getRight() + 1, yoffs, 150, 24);
		yoffs += 50;
	}
	
	
	m_labelMaxOutDuration.setBounds(xoffs, yoffs, 220, 24);
	m_editorMaxOutDuration.setBounds(m_labelMaxOutDuration.getRight() + 1, yoffs, 50, 24);
	yoffs += 25;
	buttonRender.setBounds(getWidth() - 152, getHeight()-25, 150, 24);
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
		File outfile(outfileNameEditor.getText());
		if (outfile.getParentDirectory().exists()==false)
			buttonClicked(&buttonSelectFile);
		outfile = File(outfileNameEditor.getText());
		if (outfile.getParentDirectory().exists()==false)
			return;
		int64_t numLoops = 0; 
		if (numLoopsEditor.isVisible())
			numLoops = numLoopsEditor.getText().getLargeIntValue();
		numLoops = jlimit<int64_t>(0, 1000000, numLoops);
		int sampleRate = comboBoxSamplerate.getSelectedId();
		if (sampleRate == 1)
			sampleRate = 0;
		double maxrenderlen = m_editorMaxOutDuration.getText().getDoubleValue()*3600.0;
		maxrenderlen = jlimit(1.0, 1000000.0, maxrenderlen);
		//m_main->renderToFile(File(outfileNameEditor.getText()),numLoops,
		//	comboBoxBitDepth.getSelectedId()-1,sampleRate,m_toggleFloatClip.getToggleState(),maxrenderlen);
		auto pardlg = dynamic_cast<DialogWindow*>(getParentComponent());
		{
			if (pardlg != nullptr)
				pardlg->exitModalState(1);
		}
		return;
    }
    else if (buttonThatWasClicked == &buttonSelectFile)
    {
		File lastexportfolder; // File(g_propsfile->getValue("last_export_file")).getParentDirectory();
		FileChooser myChooser("Please select audio file to render...",
			lastexportfolder,
			"*.wav");
		if (myChooser.browseForFileToSave(true))
		{
			outfileNameEditor.setText(myChooser.getResult().getFullPathName(), dontSendNotification);
		}
    }
}

int RenderSettingsComponent::getPreferredHeight()
{
	if (m_proc->getStretchSource()->isLoopingEnabled())
		return 180;
	return 150;
}

void RenderSettingsComponent::textEditorTextChanged(TextEditor & ed)
{
	return;
	if (&ed == &outfileNameEditor)
	{
		File temp(outfileNameEditor.getText());
		if (temp.getParentDirectory().exists() == false)
		{
			Logger::writeToLog("directory does not exist");
		}
		if (temp.exists() == true)
		{
			File temp2 = temp.getNonexistentSibling();
			Logger::writeToLog(temp.getFullPathName() + " exists, will instead use " + temp2.getFullPathName());
		}
	}
}

