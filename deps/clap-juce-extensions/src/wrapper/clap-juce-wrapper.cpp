/*
 * BaconPaul's running todo
 *
 * - We always say we are an instrument....
 * - midi out (try stochas perhaps?)
 * - why does dexed not work?
 * - Finish populating the desc
 * - Cleanup and comment of course (including the CMake) including what's skipped
 */

#include <memory>
#include <unordered_map>
#include <unordered_set>

#define JUCE_GUI_BASICS_INCLUDE_XHEADERS 1
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/format_types/juce_LegacyAudioParameter.cpp>

JUCE_BEGIN_IGNORE_WARNINGS_GCC_LIKE("-Wunused-parameter", "-Wsign-conversion")
JUCE_BEGIN_IGNORE_WARNINGS_MSVC(4100 4127 4244)
// Sigh - X11.h eventually does a #define None 0L which doesn't work
// with an enum in clap land being called None, so just undef it
// post the JUCE installs
#ifdef None
#undef None
#endif
#include <clap/helpers/checking-level.hh>
#include <clap/helpers/host-proxy.hh>
#include <clap/helpers/host-proxy.hxx>
#include <clap/helpers/plugin.hh>
#include <clap/helpers/plugin.hxx>
JUCE_END_IGNORE_WARNINGS_MSVC
JUCE_END_IGNORE_WARNINGS_GCC_LIKE

#include <clap-juce-extensions/clap-juce-extensions.h>

#if JUCE_LINUX
#if JUCE_VERSION > 0x060008
#include <juce_audio_plugin_client/utility/juce_LinuxMessageThread.h>
#endif
#endif

#define FIXME(x)                                                                                   \
    {                                                                                              \
        static bool onetime_ = false;                                                              \
        if (!onetime_)                                                                             \
        {                                                                                          \
            std::ostringstream oss;                                                                \
            oss << "FIXME: " << x << " @" << __LINE__;                                             \
            DBG(oss.str());                                                                        \
        }                                                                                          \
        jassert(onetime_);                                                                         \
        onetime_ = true;                                                                           \
    }

/*
 * This is a utility lock free queue based on the JUCE abstract fifo
 */
template <typename T, int qSize = 4096> class PushPopQ
{
  public:
    PushPopQ() : af(qSize) {}
    bool push(const T &ad)
    {
        auto ret = false;
        int start1, size1, start2, size2;
        af.prepareToWrite(1, start1, size1, start2, size2);
        if (size1 > 0)
        {
            dq[start1] = ad;
            ret = true;
        }
        af.finishedWrite(size1 + size2);
        return ret;
    }
    bool pop(T &ad)
    {
        bool ret = false;
        int start1, size1, start2, size2;
        af.prepareToRead(1, start1, size1, start2, size2);
        if (size1 > 0)
        {
            ad = dq[start1];
            ret = true;
        }
        af.finishedRead(size1 + size2);
        return ret;
    }
    juce::AbstractFifo af;
    T dq[qSize];
};

/*
 * These functions are the JUCE VST2/3 NSView attachment functions. We compile them into
 * our clap dll by, on macos, also linking clap_juce_mac.mm
 */
namespace juce
{
extern JUCE_API void initialiseMacVST();
extern JUCE_API void *attachComponentToWindowRefVST(Component *, void *parentWindowOrView,
                                                    bool isNSView);
} // namespace juce

JUCE_BEGIN_IGNORE_WARNINGS_MSVC(4996) // allow strncpy

/*
 * The ClapJuceWrapper is a class which immplements a collection
 * of CLAP and JUCE APIs
 */
class ClapJuceWrapper : public clap::helpers::Plugin<clap::helpers::MisbehaviourHandler::Terminate,
                                                     clap::helpers::CheckingLevel::Minimal>,
                        public juce::AudioProcessorListener,
                        public juce::AudioPlayHead,
                        public juce::AudioProcessorParameter::Listener,
                        public juce::ComponentListener
{
  public:
    // this needs to be the very last thing to get deleted!
    juce::ScopedJuceInitialiser_GUI libraryInitializer;

    static clap_plugin_descriptor desc;
    std::unique_ptr<juce::AudioProcessor> processor;
    clap_juce_extensions::clap_properties *processorAsClapProperties{nullptr};
    clap_juce_extensions::clap_extensions *processorAsClapExtensions{nullptr};

    ClapJuceWrapper(const clap_host *host, juce::AudioProcessor *p)
        : clap::helpers::Plugin<clap::helpers::MisbehaviourHandler::Terminate,
                                clap::helpers::CheckingLevel::Minimal>(&desc, host),
          processor(p)
    {
        processor->setRateAndBufferSizeDetails(0, 0);
        processor->setPlayHead(this);
        processor->addListener(this);

        processorAsClapProperties = dynamic_cast<clap_juce_extensions::clap_properties *>(p);
        processorAsClapExtensions = dynamic_cast<clap_juce_extensions::clap_extensions *>(p);

        const bool forceLegacyParamIDs = false;

        juceParameters.update(*processor, forceLegacyParamIDs);

        for (auto *juceParam :
#if JUCE_VERSION >= 0x060103
             juceParameters
#else
             juceParameters.params
#endif

        )
        {
            uint32_t clap_id = generateClapIDForJuceParam(juceParam);

            allClapIDs.insert(clap_id);
            paramPtrByClapID[clap_id] = juceParam;
            clapIDByParamPtr[juceParam] = clap_id;
        }
    }

    ~ClapJuceWrapper() override
    {
        processor->editorBeingDeleted(editor.get());

#if JUCE_LINUX
        if (_host.canUseTimerSupport())
        {
            _host.timerSupportUnregister(idleTimer);
        }
#endif
    }

    bool init() noexcept override
    {
#if JUCE_LINUX
        if (_host.canUseTimerSupport())
        {
            _host.timerSupportRegister(1000 / 50, &idleTimer);
        }
#endif
        return true;
    }

  public:
    bool implementsTimerSupport() const noexcept override { return true; }
    void onTimer(clap_id timerId) noexcept override
    {
        juce::ignoreUnused(timerId);
#if LINUX
        juce::ScopedJuceInitialiser_GUI libraryInitialiser;
        const juce::MessageManagerLock mmLock;

#if JUCE_VERSION > 0x060008
        while (juce::dispatchNextMessageOnSystemQueue(true))
        {
        }
#else
        auto mm = juce::MessageManager::getInstance();
        mm->runDispatchLoopUntil(0);
#endif
#endif
    }

    clap_id idleTimer{0};

    uint32_t generateClapIDForJuceParam(juce::AudioProcessorParameter *param) const
    {
        auto juceParamID = juce::LegacyAudioParameter::getParamID(param, false);
        auto clap_id = static_cast<uint32_t>(juceParamID.hashCode());
        return clap_id;
    }

#if JUCE_VERSION >= 0x060008
    void audioProcessorChanged(juce::AudioProcessor *proc, const ChangeDetails &details) override
    {
        juce::ignoreUnused(proc);
        if (details.latencyChanged)
        {
            runOnMainThread([this] {
                if (isBeingDestroyed())
                    return;

                _host.latencyChanged();
            });
        }
        if (details.programChanged)
        {
            // At the moment, CLAP doesn't have a sense of programs (to my knowledge).
            // (I think) what makes most sense is to tell the host to update the parameters
            // as though a preset has been loaded.
            runOnMainThread([this] {
                if (isBeingDestroyed())
                    return;

                _host.paramsRescan(CLAP_PARAM_RESCAN_VALUES);
            });
        }
#if JUCE_VERSION >= 0x060103
        if (details.nonParameterStateChanged)
        {
            runOnMainThread([this] {
                if (isBeingDestroyed())
                    return;

                _host.stateMarkDirty();
            });
        }
#endif
        if (details.parameterInfoChanged)
        {
            // JUCE documentations states that, `parameterInfoChanged` means
            // "Indicates that some attributes of the AudioProcessor's parameters have changed."
            // For now, I'm going to assume this means the parameter's name or value->text
            // conversion has changed, and tell the clap host to rescan those.
            //
            // We could do CLAP_PARAM_RESCAN_ALL, but then the plugin would have to be deactivated.
            runOnMainThread([this] {
                if (isBeingDestroyed())
                    return;

                _host.paramsRescan(CLAP_PARAM_RESCAN_VALUES | CLAP_PARAM_RESCAN_TEXT);
            });
        }
    }
#else
    void audioProcessorChanged(juce::AudioProcessor *proc) override {
       /*
        * Before 6.0.8 it was unclear what changed. For now make the approximating decision to just
        * rescan values and text.
        */
       runOnMainThread([this] {
           if (isBeingDestroyed())
              return;

           _host.paramsRescan(CLAP_PARAM_RESCAN_VALUES | CLAP_PARAM_RESCAN_TEXT);
       });
    }
#endif


    clap_id clapIdFromParameterIndex(int index)
    {
        auto pbi = juceParameters.getParamForIndex(index);
        auto pf = clapIDByParamPtr.find(pbi);
        if (pf != clapIDByParamPtr.end())
            return pf->second;

        auto id = generateClapIDForJuceParam(pbi); // a lookup obviously
        return id;
    }

    void audioProcessorParameterChanged(juce::AudioProcessor *, int index, float newValue) override
    {
        auto id = clapIdFromParameterIndex(index);
        uiParamChangeQ.push({CLAP_EVENT_PARAM_VALUE, 0, id, newValue});
    }

    void audioProcessorParameterChangeGestureBegin(juce::AudioProcessor *, int index) override
    {
        auto id = clapIdFromParameterIndex(index);
        auto p = paramPtrByClapID[id];
        uiParamChangeQ.push({CLAP_EVENT_PARAM_GESTURE_BEGIN, 0, id, p->getValue()});
    }

    void audioProcessorParameterChangeGestureEnd(juce::AudioProcessor *, int index) override
    {
        auto id = clapIdFromParameterIndex(index);
        auto p = paramPtrByClapID[id];
        uiParamChangeQ.push({CLAP_EVENT_PARAM_GESTURE_END, 0, id, p->getValue()});
    }

    /*
     * According to the JUCE docs this is *only* called on the processing thread
     */
    bool getCurrentPosition(juce::AudioPlayHead::CurrentPositionInfo &info) override
    {
        if (hasTransportInfo && transportInfo)
        {
            auto flags = transportInfo->flags;

            if (flags & CLAP_TRANSPORT_HAS_TEMPO)
                info.bpm = transportInfo->tempo;
            if (flags & CLAP_TRANSPORT_HAS_TIME_SIGNATURE)
            {
                info.timeSigNumerator = transportInfo->tsig_num;
                info.timeSigDenominator = transportInfo->tsig_denom;
            }

            if (flags & CLAP_TRANSPORT_HAS_BEATS_TIMELINE)
            {
                info.ppqPosition = 1.0 * transportInfo->song_pos_beats / CLAP_BEATTIME_FACTOR;
                info.ppqPositionOfLastBarStart =
                    1.0 * transportInfo->bar_start / CLAP_BEATTIME_FACTOR;
            }
            if (flags & CLAP_TRANSPORT_HAS_SECONDS_TIMELINE)
            {
                info.timeInSeconds = 1.0 * (double)transportInfo->song_pos_seconds / CLAP_SECTIME_FACTOR;
                info.timeInSamples = (int64_t)(info.timeInSeconds * sampleRate());
            }
            info.isPlaying = flags & CLAP_TRANSPORT_IS_PLAYING;
            info.isRecording = flags & CLAP_TRANSPORT_IS_RECORDING;
            info.isLooping = flags & CLAP_TRANSPORT_IS_LOOP_ACTIVE;
        }
        return hasTransportInfo;
    }

    void parameterValueChanged(int, float newValue) override
    {
        juce::ignoreUnused(newValue);
        FIXME("parameter value changed");
        // this can only come from the bypass parameter
    }

    void parameterGestureChanged(int, bool) override { FIXME("parameter gesture changed"); }

    bool activate(double sampleRate, uint32_t minFrameCount,
                  uint32_t maxFrameCount) noexcept override
    {
        juce::ignoreUnused(minFrameCount);
        processor->setRateAndBufferSizeDetails(sampleRate, maxFrameCount);
        processor->prepareToPlay(sampleRate, (int)maxFrameCount);
        midiBuffer.ensureSize(2048);
        midiBuffer.clear();
        return true;
    }

    /* CLAP API */

    bool implementsAudioPorts() const noexcept override { return true; }
    uint32_t audioPortsCount(bool isInput) const noexcept override
    {
        return (uint32_t)processor->getBusCount(isInput);
    }

    bool audioPortsInfo(uint32_t index, bool isInput,
                        clap_audio_port_info *info) const noexcept override
    {
        // For now hardcode to stereo out. Fix this obviously.
        auto bus = processor->getBus(isInput, (int)index);
        auto clob = bus->getDefaultLayout();

        // For now we only support stereo channels
        jassert(clob.size() == 1 || clob.size() == 2);
        jassert(clob.size() == 1 || (clob.getTypeOfChannel(0) == juce::AudioChannelSet::left &&
                                     clob.getTypeOfChannel(1) == juce::AudioChannelSet::right));
        // if (isInput || index != 0) return false;
        info->id = (isInput ? 1 << 15 : 1) + index;
        strncpy(info->name, bus->getName().toRawUTF8(), sizeof(info->name));

        bool couldBeMain = true;
        if (isInput && processorAsClapExtensions)
            couldBeMain = processorAsClapExtensions->isInputMain(index);
        if (index == 0 && couldBeMain)
        {
            info->flags = CLAP_AUDIO_PORT_IS_MAIN;
        }
        else
        {
            info->flags = 0;
        }

        info->in_place_pair = info->id;
        info->channel_count = (uint32_t)clob.size();

        if (clob.size() == 1)
            info->port_type = CLAP_PORT_MONO;
        else
            info->port_type = CLAP_PORT_STEREO;

        auto requested = processor->getBusesLayout();
        if (clob.size() == 1)
            requested.getChannelSet(isInput, (int)index) = juce::AudioChannelSet::mono();
        if (clob.size() == 2)
            requested.getChannelSet(isInput, (int)index) = juce::AudioChannelSet::stereo();

        processor->setBusesLayoutWithoutEnabling(requested);

        DBG("audioPortsInfo " << (isInput ? "INPUT " : "OUTPUT ") << (int)index << " '"
                              << bus->getName()
                              << "' isMain=" << ((index == 0 && couldBeMain) ? "T" : "F")
                              << " sz=" << clob.size() << " enabled=" << (int)(bus->isEnabled()));

        return true;
    }
    uint32_t audioPortsConfigCount() const noexcept override
    {
        DBG("audioPortsConfigCount CALLED - returning 0");
        return 0;
    }
    bool audioPortsGetConfig(uint32_t /*index*/,
                             clap_audio_ports_config * /*config*/) const noexcept override
    {
        return false;
    }
    bool audioPortsSetConfig(clap_id /*configId*/) noexcept override { return false; }

    bool implementsNotePorts() const noexcept override { return true; }
    uint32_t notePortsCount(bool is_input) const noexcept override
    {
        if (is_input)
        {
            if (processor->acceptsMidi())
                return 1;
        }
        else
        {
            if (processor->producesMidi())
                return 1;
        }
        return 0;
    }
    bool notePortsInfo(uint32_t index, bool is_input,
                       clap_note_port_info *info) const noexcept override
    {
        juce::ignoreUnused(index);

        if (is_input)
        {
            info->id = 1 << 5U;
            info->supported_dialects = CLAP_NOTE_DIALECT_MIDI;
            if (processor->supportsMPE())
                info->supported_dialects |= CLAP_NOTE_DIALECT_MIDI_MPE;

            info->preferred_dialect = CLAP_NOTE_DIALECT_MIDI;

            if (processorAsClapExtensions)
            {
                if (processorAsClapExtensions->supportsNoteDialectClap(true))
                {
                    info->supported_dialects |= CLAP_NOTE_DIALECT_CLAP;
                }
                if (processorAsClapExtensions->prefersNoteDialectClap(true))
                {
                    info->preferred_dialect = CLAP_NOTE_DIALECT_CLAP;
                }
            }
            strncpy(info->name, "JUCE Note Input", CLAP_NAME_SIZE);
        }
        else
        {
            info->id = 1 << 2U;
            info->supported_dialects = CLAP_NOTE_DIALECT_MIDI;
            if (processor->supportsMPE())
                info->supported_dialects |= CLAP_NOTE_DIALECT_MIDI_MPE;
            info->preferred_dialect = CLAP_NOTE_DIALECT_MIDI;

            if (processorAsClapExtensions)
            {
                if (processorAsClapExtensions->supportsNoteDialectClap(false))
                {
                    info->supported_dialects |= CLAP_NOTE_DIALECT_CLAP;
                }
                if (processorAsClapExtensions->prefersNoteDialectClap(false))
                {
                    info->preferred_dialect = CLAP_NOTE_DIALECT_CLAP;
                }
            }

            strncpy(info->name, "JUCE Note Output", CLAP_NAME_SIZE);
        }
        return true;
    }

    bool implementsVoiceInfo() const noexcept override
    {
        if (processorAsClapExtensions)
            return processorAsClapExtensions->supportsVoiceInfo();
        return false;
    }

    bool voiceInfoGet(clap_voice_info *info) noexcept override
    {
        if (processorAsClapExtensions)
            return processorAsClapExtensions->voiceInfoGet(info);
        return Plugin::voiceInfoGet(info);
    }

  public:
    bool implementsParams() const noexcept override { return true; }
    bool isValidParamId(clap_id paramId) const noexcept override
    {
        return allClapIDs.find(paramId) != allClapIDs.end();
    }
    uint32_t paramsCount() const noexcept override { return (uint32_t)allClapIDs.size(); }
    bool paramsInfo(uint32_t paramIndex, clap_param_info *info) const noexcept override
    {
        auto pbi = juceParameters.getParamForIndex((int)paramIndex);

        auto *parameterGroup = processor->getParameterTree().getGroupsForParameter(pbi).getLast();
        juce::String group = "";
        while (parameterGroup && parameterGroup->getParent() &&
               parameterGroup->getParent()->getName().isNotEmpty())
        {
            group = parameterGroup->getName() + "/" + group;
            parameterGroup = parameterGroup->getParent();
        }

        if (group.isNotEmpty())
            group = "/" + group;

        // Fixme - using parameter groups here would be lovely but until then
        info->id = generateClapIDForJuceParam(pbi);
        strncpy(info->name, (pbi->getName(CLAP_NAME_SIZE)).toRawUTF8(), CLAP_NAME_SIZE);
        strncpy(info->module, group.toRawUTF8(), CLAP_NAME_SIZE);

        info->min_value = 0; // FIXME
        info->max_value = 1;
        info->default_value = pbi->getDefaultValue();
        info->cookie = pbi;
        info->flags = 0;

        if (pbi->isAutomatable())
            info->flags = info->flags | CLAP_PARAM_IS_AUTOMATABLE;

        if (pbi->isBoolean() || pbi->isDiscrete())
        {
            info->flags = info->flags | CLAP_PARAM_IS_STEPPED;
        }

        auto cpe = dynamic_cast<clap_juce_extensions::clap_param_extensions *>(pbi);
        if (cpe)
        {
            if (cpe->supportsMonophonicModulation())
            {
                info->flags = info->flags | CLAP_PARAM_IS_MODULATABLE;
            }
            if (cpe->supportsPolyphonicModulation())
            {
                info->flags =
                    info->flags | CLAP_PARAM_IS_MODULATABLE |
                    CLAP_PARAM_IS_MODULATABLE_PER_CHANNEL | CLAP_PARAM_IS_MODULATABLE_PER_KEY |
                    CLAP_PARAM_IS_AUTOMATABLE_PER_NOTE_ID | CLAP_PARAM_IS_AUTOMATABLE_PER_PORT;
            }
        }

        return true;
    }

    bool paramsValue(clap_id paramId, double *value) noexcept override
    {
        auto pbi = paramPtrByClapID[paramId];
        *value = pbi->getValue();
        return true;
    }

    bool paramsValueToText(clap_id paramId, double value, char *display,
                           uint32_t size) noexcept override
    {
        auto pbi = paramPtrByClapID[paramId];
        auto res = pbi->getText((float)value, (int)size);
        strncpy(display, res.toStdString().c_str(), size);
        return true;
    }

    bool paramsTextToValue(clap_id paramId, const char *display, double *value) noexcept override
    {
        auto pbi = paramPtrByClapID[paramId];
        *value = pbi->getValueForText(display);
        return true;
    }

    void paramSetValueAndNotifyIfChanged(juce::AudioProcessorParameter &param, float newValue)
    {
        if (param.getValue() == newValue)
            return;

        param.setValueNotifyingHost(newValue);
    }

    bool implementsLatency() const noexcept override { return true; }
    uint32_t latencyGet() const noexcept override
    {
        return (uint32_t)processor->getLatencySamples();
    }

    bool implementsTail() const noexcept override { return true; }
    uint32_t tailGet(const clap_plugin_t *) const noexcept override
    {
        return uint32_t(
            juce::roundToIntAccurate((double)sampleRate() * processor->getTailLengthSeconds()));
    }

    juce::MidiBuffer midiBuffer;

    clap_process_status process(const clap_process *process) noexcept override
    {
        // Since the playhead is *only* good inside juce audio processor process,
        // we can just keep this little transient pointer here
        if (process->transport)
        {
            hasTransportInfo = true;
            transportInfo = process->transport;
        }
        else
        {
            hasTransportInfo = false;
            transportInfo = nullptr;
        }

        if (processorAsClapProperties)
            processorAsClapProperties->clap_transport = process->transport;

        auto pc = ParamChange();
        auto ov = process->out_events;

        while (uiParamChangeQ.pop(pc))
        {
            if (pc.type == CLAP_EVENT_PARAM_VALUE)
            {
                auto evt = clap_event_param_value();
                evt.header.size = sizeof(clap_event_param_value);
                evt.header.type = (uint16_t)CLAP_EVENT_PARAM_VALUE;
                evt.header.time = 0; // for now
                evt.header.space_id = CLAP_CORE_EVENT_SPACE_ID;
                evt.header.flags = pc.flag;
                evt.param_id = pc.id;
                evt.value = pc.newval;
                ov->try_push(ov, reinterpret_cast<const clap_event_header *>(&evt));
            }

            if (pc.type == CLAP_EVENT_PARAM_GESTURE_END ||
                pc.type == CLAP_EVENT_PARAM_GESTURE_BEGIN)
            {
                auto evt = clap_event_param_gesture();
                evt.header.size = sizeof(clap_event_param_gesture);
                evt.header.type = (uint16_t)pc.type;
                evt.header.time = 0; // for now
                evt.header.space_id = CLAP_CORE_EVENT_SPACE_ID;
                evt.header.flags = pc.flag;
                evt.param_id = pc.id;
                ov->try_push(ov, reinterpret_cast<const clap_event_header *>(&evt));
            }
        }

        if (processorAsClapExtensions && processorAsClapExtensions->supportsDirectProcess())
            return processorAsClapExtensions->clap_direct_process(process);

        auto ev = process->in_events;
        auto sz = ev->size(ev);

        if (sz != 0)
        {
            for (uint32_t i = 0; i < sz; ++i)
            {
                auto evt = ev->get(ev, i);

                if (evt->space_id != CLAP_CORE_EVENT_SPACE_ID)
                    continue;

                switch (evt->type)
                {
                case CLAP_EVENT_NOTE_ON:
                {
                    auto nevt = reinterpret_cast<const clap_event_note *>(evt);

                    midiBuffer.addEvent(juce::MidiMessage::noteOn(nevt->channel + 1, nevt->key,
                                                                  (float)nevt->velocity),
                                        (int)nevt->header.time);
                }
                break;
                case CLAP_EVENT_NOTE_OFF:
                {
                    auto nevt = reinterpret_cast<const clap_event_note *>(evt);
                    midiBuffer.addEvent(juce::MidiMessage::noteOff(nevt->channel + 1, nevt->key,
                                                                   (float)nevt->velocity),
                                        (int)nevt->header.time);
                }
                break;
                case CLAP_EVENT_MIDI:
                {
                    auto mevt = reinterpret_cast<const clap_event_midi *>(evt);
                    midiBuffer.addEvent(juce::MidiMessage(mevt->data[0], mevt->data[1],
                                                          mevt->data[2], mevt->header.time),
                                        (int)mevt->header.time);
                }
                break;
                case CLAP_EVENT_TRANSPORT:
                {
                    // handle this case
                }
                break;
                case CLAP_EVENT_PARAM_VALUE:
                {
                    auto pevt = reinterpret_cast<const clap_event_param_value *>(evt);

                    auto id = pevt->param_id;
                    auto nf = pevt->value;
                    jassert(pevt->cookie == paramPtrByClapID[id]);
                    auto jp = static_cast<juce::AudioProcessorParameter *>(pevt->cookie);
                    paramSetValueAndNotifyIfChanged(*jp, (float)nf);
                }
                break;
                case CLAP_EVENT_PARAM_MOD:
                {
                }
                break;
                case CLAP_EVENT_NOTE_END:
                {
                    // Why do you send me this, Alex?
                }
                break;
                default:
                {
                    DBG("Unknown message type " << (int)evt->type);
                    // In theory I should never get this.
                    // jassertfalse
                }
                break;
                }
            }
        }

        // We process in place so
        static constexpr uint32_t maxBuses = 128;
        std::array<float *, maxBuses> busses{};
        busses.fill(nullptr);

        /*DBG("IO Configuration: I=" << (int)process->audio_inputs_count << " O="
                                   << (int)process->audio_outputs_count << " MX=" << (int)mx);
        DBG("Plugin Configuration: IC=" << processor->getTotalNumInputChannels()
                                        << " OC=" << processor->getTotalNumOutputChannels());
        */

        /*
         * OK so here is what JUCE expects in its audio buffer. It *always* uses input as output
         * buffer so we need to create a buffer where each channel is the channel of the associated
         * output pointer (fine) and then the inputs need to either check they are the same or copy.
         */

        /*
         * So first lets load up with our outputs
         */
        uint32_t ochans = 0;
        for (uint32_t idx = 0; idx < process->audio_outputs_count && ochans < maxBuses; ++idx)
        {
            for (uint32_t ch = 0; ch < process->audio_outputs[idx].channel_count; ++ch)
            {
                busses[ochans] = process->audio_outputs[idx].data32[ch];
                ochans++;
            }
        }

        uint32_t ichans = 0;
        for (uint32_t idx = 0; idx < process->audio_inputs_count && ichans < maxBuses; ++idx)
        {
            for (uint32_t ch = 0; ch < process->audio_inputs[idx].channel_count; ++ch)
            {
                auto *ic = process->audio_inputs[idx].data32[ch];
                if (ichans < ochans)
                {
                    if (ic == busses[ichans])
                    {
                        // The buffers overlap - no need to do anything
                    }
                    else
                    {
                        juce::FloatVectorOperations::copy(busses[ichans], ic,
                                                          (int)process->frames_count);
                    }
                }
                else
                {
                    busses[ichans] = ic;
                }
                ichans++;
            }
        }

        auto totalChans = std::max(ichans, ochans);
        juce::AudioBuffer<float> buf(busses.data(), (int)totalChans, (int)process->frames_count);

        FIXME("Handle bypass and deactivated states");
        processor->processBlock(buf, midiBuffer);

        if (processor->producesMidi())
        {
            for (auto meta : midiBuffer)
            {
                auto msg = meta.getMessage();
                if (msg.getRawDataSize() == 3)
                {
                    auto evt = clap_event_midi();
                    evt.header.size = sizeof(clap_event_midi);
                    evt.header.type = (uint16_t)CLAP_EVENT_MIDI;
                    evt.header.time = meta.samplePosition; // for now
                    evt.header.space_id = CLAP_CORE_EVENT_SPACE_ID;
                    evt.header.flags = 0;
                    evt.port_index = 0;
                    memcpy(&evt.data, msg.getRawData(), 3 * sizeof(uint8_t));
                    ov->try_push(ov, reinterpret_cast<const clap_event_header *>(&evt));
                }
            }
        }

        if (!midiBuffer.isEmpty())
            midiBuffer.clear();

        return CLAP_PROCESS_CONTINUE;
    }

    void componentMovedOrResized(juce::Component &component, bool wasMoved,
                                 bool wasResized) override
    {
        juce::ignoreUnused(wasMoved);
        if (wasResized)
            _host.guiRequestResize((uint32_t)component.getWidth(), (uint32_t)component.getHeight());
    }

    std::unique_ptr<juce::AudioProcessorEditor> editor;
    bool implementsGui() const noexcept override { return processor->hasEditor(); }
    bool guiCanResize() const noexcept override
    {
        if (editor)
            return editor->isResizable();
        return true;
    }
    bool guiAdjustSize(uint32_t *w, uint32_t *h) noexcept override
    {
        if (!editor)
            return false;

        if (!editor->isResizable())
            return false;

        editor->setSize(*w, *h);
        return true;
    }

    bool guiIsApiSupported(const char *api, bool isFloating) noexcept override
    {
        if (isFloating)
            return false;

        if (strcmp(api, CLAP_WINDOW_API_WIN32) == 0 || strcmp(api, CLAP_WINDOW_API_COCOA) == 0 ||
            strcmp(api, CLAP_WINDOW_API_X11) == 0)
            return true;

        return false;
    }

    bool guiCreate(const char *api, bool isFloating) noexcept override
    {
        juce::ignoreUnused(api);

        // Should never happen
        if (isFloating)
            return false;

        const juce::MessageManagerLock mmLock;
        editor.reset(processor->createEditorIfNeeded());
        editor->addComponentListener(this);
        return editor != nullptr;
    }

    void guiDestroy() noexcept override
    {
        processor->editorBeingDeleted(editor.get());
        editor.reset(nullptr);
    }

    bool guiSetParent(const clap_window *window) noexcept override
    {
#if JUCE_MAC
        return guiCocoaAttach(window->cocoa);
#elif JUCE_LINUX
        return guiX11Attach(NULL, window->x11);
#elif JUCE_WINDOWS
        return guiWin32Attach(window->win32);
#else
        return false;
#endif
    }

    bool guiGetSize(uint32_t *width, uint32_t *height) noexcept override
    {
        const juce::MessageManagerLock mmLock;
        if (editor)
        {
            auto b = editor->getBounds();
            *width = (uint32_t)b.getWidth();
            *height = (uint32_t)b.getHeight();
            return true;
        }
        else
        {
            *width = 1000;
            *height = 800;
        }
        return false;
    }

  protected:
    juce::CriticalSection stateInformationLock;
    juce::MemoryBlock chunkMemory;

  public:
    bool implementsState() const noexcept override { return true; }
    bool stateSave(const clap_ostream *stream) noexcept override
    {
        if (processor == nullptr)
            return false;

        juce::ScopedLock lock(stateInformationLock);
        chunkMemory.reset();

        processor->getStateInformation(chunkMemory);

        auto written = stream->write(stream, chunkMemory.getData(), chunkMemory.getSize());
        return written == (int64_t)chunkMemory.getSize();
    }
    bool stateLoad(const clap_istream *stream) noexcept override
    {
        if (processor == nullptr)
            return false;

        juce::ScopedLock lock(stateInformationLock);
        chunkMemory.reset();
        // There must be a better way
        char block[256];
        int64_t rd;
        while ((rd = stream->read(stream, block, 256)) > 0)
            chunkMemory.append(block, (size_t)rd);

        processor->setStateInformation(chunkMemory.getData(), (int)chunkMemory.getSize());
        chunkMemory.reset();
        return true;
    }

  public:
#if JUCE_MAC
    bool guiCocoaAttach(void *nsView) noexcept
    {
        juce::initialiseMacVST();
        auto hostWindow = juce::attachComponentToWindowRefVST(editor.get(), nsView, true);
        juce::ignoreUnused(hostWindow);
        return true;
    }
#endif

#if JUCE_LINUX
    bool guiX11Attach(const char *displayName, unsigned long window) noexcept
    {
        const juce::MessageManagerLock mmLock;
        editor->setVisible(false);
        editor->addToDesktop(0, (void *)window);
        auto *display = juce::XWindowSystem::getInstance()->getDisplay();
        juce::X11Symbols::getInstance()->xReparentWindow(display, (Window)editor->getWindowHandle(),
                                                         window, 0, 0);
        editor->setVisible(true);
        return true;
    }
#endif

#if JUCE_WINDOWS
    bool guiWin32Attach(clap_hwnd window) noexcept
    {
        editor->setVisible(false);
        editor->setOpaque(true);
        editor->setTopLeftPosition(0, 0);
        editor->addToDesktop(0, (void *)window);
        editor->setVisible(true);
        return true;
    }
#endif

  private:
    struct ParamChange
    {
        int type;
        int flag;
        uint32_t id;
        float newval{0};
    };
    PushPopQ<ParamChange, 4096 * 16> uiParamChangeQ;

    /*
     * Various maps for ID lookups
     */
    // clap_id to param *
    std::unordered_map<clap_id, juce::AudioProcessorParameter *> paramPtrByClapID;
    // param * to clap_id
    std::unordered_map<juce::AudioProcessorParameter *, clap_id> clapIDByParamPtr;
    // Every id we have issued
    std::unordered_set<clap_id> allClapIDs;

    juce::LegacyAudioParametersWrapper juceParameters;

    const clap_event_transport *transportInfo{nullptr};
    bool hasTransportInfo{false};
};

JUCE_END_IGNORE_WARNINGS_MSVC

const char *features[] = {CLAP_FEATURES, nullptr};
clap_plugin_descriptor ClapJuceWrapper::desc = {CLAP_VERSION,
                                                CLAP_ID,
                                                JucePlugin_Name,
                                                JucePlugin_Manufacturer,
                                                JucePlugin_ManufacturerWebsite,
                                                CLAP_MANUAL_URL,
                                                CLAP_SUPPORT_URL,
                                                JucePlugin_VersionString,
                                                JucePlugin_Desc,
                                                features};

juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter();

namespace ClapAdapter
{
bool clap_init(const char *) { return true; }

void clap_deinit(void) {}

uint32_t clap_get_plugin_count(const struct clap_plugin_factory *) { return 1; }

const clap_plugin_descriptor *clap_get_plugin_descriptor(const struct clap_plugin_factory *,
                                                         uint32_t)
{
    return &ClapJuceWrapper::desc;
}

static const clap_plugin *clap_create_plugin(const struct clap_plugin_factory *,
                                             const clap_host *host, const char *plugin_id)
{
    juce::ScopedJuceInitialiser_GUI libraryInitialiser;

    if (strcmp(plugin_id, ClapJuceWrapper::desc.id))
    {
        std::cout << "Warning: CLAP asked for plugin_id '" << plugin_id
                  << "' and JuceCLAPWrapper ID is '" << ClapJuceWrapper::desc.id << "'"
                  << std::endl;
        return nullptr;
    }
    clap_juce_extensions::clap_properties::building_clap = true;
    clap_juce_extensions::clap_properties::clap_version_major = CLAP_VERSION_MAJOR;
    clap_juce_extensions::clap_properties::clap_version_minor = CLAP_VERSION_MINOR;
    clap_juce_extensions::clap_properties::clap_version_revision = CLAP_VERSION_REVISION;
    auto *const pluginInstance = ::createPluginFilter();
    clap_juce_extensions::clap_properties::building_clap = false;
    auto *wrapper = new ClapJuceWrapper(host, pluginInstance);
    return wrapper->clapPlugin();
}

const struct clap_plugin_factory juce_clap_plugin_factory = {
    ClapAdapter::clap_get_plugin_count,
    ClapAdapter::clap_get_plugin_descriptor,
    ClapAdapter::clap_create_plugin,
};

const void *clap_get_factory(const char *factory_id)
{
    if (strcmp(factory_id, CLAP_PLUGIN_FACTORY_ID) == 0)
    {
        return &juce_clap_plugin_factory;
    }

    return nullptr;
}

} // namespace ClapAdapter

extern "C"
{
#if JUCE_LINUX
#pragma GCC diagnostic ignored "-Wattributes"
#endif
    const CLAP_EXPORT struct clap_plugin_entry clap_entry = {CLAP_VERSION, ClapAdapter::clap_init,
                                                             ClapAdapter::clap_deinit,
                                                             ClapAdapter::clap_get_factory};
}
