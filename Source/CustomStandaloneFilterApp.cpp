// SPDX-License-Identifier: GPLv3-or-later WITH Appstore-exception

#include "../JuceLibraryCode/JuceHeader.h"

#include "juce_core/system/juce_TargetPlatform.h"
//#include "juce_audio_plugin_client/utility/juce_CheckSettingMacros.h"

//#include "juce_audio_plugin_client/utility/juce_IncludeSystemHeaders.h"
//#include "juce_audio_plugin_client/utility/juce_IncludeModuleHeaders.h"
//#include "juce_audio_plugin_client/utility/juce_FakeMouseMoveGenerator.h"
//#include "juce_audio_plugin_client/utility/juce_WindowsHooks.h"

#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include <juce_audio_utils/juce_audio_utils.h>

#include "PluginProcessor.h"

// #include "DebugLogC.h"

// You can set this flag in your build if you need to specify a different
// standalone JUCEApplication class for your app to use. If you don't
// set it then by default we'll just create a simple one as below.
//#if ! JUCE_USE_CUSTOM_PLUGIN_STANDALONE_APP

extern juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter();

#include "CustomStandaloneFilterWindow.h"
#include "CustomLookAndFeel.h"

namespace juce
{

//==============================================================================
class StandaloneFilterApp  : public JUCEApplication
{
public:
    StandaloneFilterApp()
    {
        PluginHostType::jucePlugInClientCurrentWrapperType = AudioProcessor::wrapperType_Standalone;

        PropertiesFile::Options options;

        options.applicationName     = getApplicationName();
        options.filenameSuffix      = ".settings";
        options.osxLibrarySubFolder = "Application Support/" + getApplicationName();
       #if JUCE_LINUX
        options.folderName          = "~/.config/paulxstretch";
       #else
        options.folderName          = "";
       #endif

        appProperties.setStorageParameters (options);

        LookAndFeel::setDefaultLookAndFeel(&sonoLNF);
        
    }

    const String getApplicationName() override              { return JucePlugin_Name; }
    const String getApplicationVersion() override           { return JucePlugin_VersionString; }
    bool moreThanOneInstanceAllowed() override              { return true; }
    void anotherInstanceStarted (const String& cmdline) override    {

        DBG("Another instance started: " << cmdline);

    }

    bool urlOpened(const URL& url) override    {

        DBG("URL opened: " << url.toString(false));
        if (mainWindow.get() != nullptr)
        {
            mainWindow->pluginHolder->urlOpened(url);
            return true;
        }
        return false;
    }

    CustomLookAndFeel  sonoLNF;

    
    virtual StandaloneFilterWindow* createWindow()
    {
       #ifdef JucePlugin_PreferredChannelConfigurations
        StandalonePluginHolder::PluginInOuts channels[] = { JucePlugin_PreferredChannelConfigurations };
       #endif

        AudioDeviceManager::AudioDeviceSetup setupOptions;
        setupOptions.bufferSize = 512;
        
        return new StandaloneFilterWindow (getApplicationName(),
                                           LookAndFeel::getDefaultLookAndFeel().findColour (ResizableWindow::backgroundColourId),
                                           appProperties.getUserSettings(),
                                           false, {}, &setupOptions
                                          #ifdef JucePlugin_PreferredChannelConfigurations
                                           , juce::Array<StandalonePluginHolder::PluginInOuts> (channels, juce::numElementsInArray (channels))
                                          #else
                                           , {}
                                          #endif
                                          #if JUCE_DONT_AUTO_OPEN_MIDI_DEVICES_ON_MOBILE
                                           , false
                                          #endif
                                           );
    }

    //==============================================================================
    void initialise (const String&) override
    {
        mainWindow.reset (createWindow());

       #if JUCE_STANDALONE_FILTER_WINDOW_USE_KIOSK_MODE
        Desktop::getInstance().setKioskModeComponent (mainWindow.get(), false);
       #endif

        mainWindow->setVisible (true);
        
        Desktop::getInstance().setScreenSaverEnabled(false);
    }

    void shutdown() override
    {
        DBG("shutdown");
        if (mainWindow.get() != nullptr) {
            mainWindow->pluginHolder->savePluginState();
            mainWindow->pluginHolder->saveAudioDeviceState();
        }
        
        mainWindow = nullptr;
        appProperties.saveIfNeeded();
    }

    void suspended() override
    {
        DBG("suspended");
        if (mainWindow.get() != nullptr) {
            mainWindow->pluginHolder->savePluginState();
            mainWindow->pluginHolder->saveAudioDeviceState();

            if (auto * sonoproc = dynamic_cast<PaulstretchpluginAudioProcessor*>(mainWindow->pluginHolder->processor.get())) {
                if (sonoproc->getBoolParameter(cpi_pause_enabled)->get() && !sonoproc->isInputRecordingEnabled()
                    && !sonoproc->isRecordingToFile() && !mainWindow->pluginHolder->isInterAppAudioConnected()) {
                    // shutdown audio engine
                    DBG("not active, shutting down audio");
                    mainWindow->getDeviceManager().closeAudioDevice();
                    sonoproc->setPlayHead (nullptr);
                }
            }
        }

        appProperties.saveIfNeeded();

        Desktop::getInstance().setScreenSaverEnabled(true);        
    }
    
    void resumed() override
    {
        Desktop::getInstance().setScreenSaverEnabled(false);
        if (auto * dev = mainWindow->getDeviceManager().getCurrentAudioDevice()) {
            if (!dev->isPlaying()) {
                DBG("dev not playing, restarting");
                mainWindow->getDeviceManager().restartLastAudioDevice();
            }
        }
        else {
            DBG("was not actve: restarting");
            mainWindow->getDeviceManager().restartLastAudioDevice();
        }
    }
    
    //==============================================================================
    void systemRequestedQuit() override
    {
        DBG("Requested quit");
        if (mainWindow.get() != nullptr) {
            mainWindow->pluginHolder->savePluginState();
            mainWindow->pluginHolder->saveAudioDeviceState();
        }

        appProperties.saveIfNeeded();

        if (ModalComponentManager::getInstance()->cancelAllModalComponents())
        {
            Timer::callAfterDelay (100, []()
            {
                if (auto app = JUCEApplicationBase::getInstance())
                    app->systemRequestedQuit();
            });
        }
        else
        {
            quit();
        }
    }

    void memoryWarningReceived()  override
    {
        DBG("Memory warning");
    }
    
protected:
    ApplicationProperties appProperties;
    std::unique_ptr<StandaloneFilterWindow> mainWindow;
};

} // namespace juce

#if JucePlugin_Build_Standalone && JUCE_IOS

using namespace juce;

bool JUCE_CALLTYPE juce_isInterAppAudioConnected()
{
    if (auto holder = StandalonePluginHolder::getInstance())
        return holder->isInterAppAudioConnected();

    return false;
}

void JUCE_CALLTYPE juce_switchToHostApplication()
{
    if (auto holder = StandalonePluginHolder::getInstance())
        holder->switchToHostApplication();
}

#if JUCE_MODULE_AVAILABLE_juce_gui_basics
Image JUCE_CALLTYPE juce_getIAAHostIcon (int size)
{
    if (auto holder = StandalonePluginHolder::getInstance())
        return holder->getIAAHostIcon (size);

    return Image();
}
#endif
#endif

START_JUCE_APPLICATION (StandaloneFilterApp);

//#endif
