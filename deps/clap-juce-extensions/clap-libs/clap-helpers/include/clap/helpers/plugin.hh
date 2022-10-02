#pragma once

#include <cassert>
#include <functional>
#include <mutex>
#include <queue>
#include <string>
#include <string_view>
#include <vector>

#include <clap/clap.h>

#include "checking-level.hh"
#include "host-proxy.hh"
#include "misbehaviour-handler.hh"

namespace clap { namespace helpers {

   /// @brief C++ glue and checks
   ///
   /// @note for an higher level implementation, see @ref PluginHelper
   template <MisbehaviourHandler h, CheckingLevel l>
   class Plugin {
   public:
      const clap_plugin *clapPlugin() noexcept { return &_plugin; }

   protected:
      Plugin(const clap_plugin_descriptor *desc, const clap_host *host);
      virtual ~Plugin() = default;

      // not copyable, not moveable
      Plugin(const Plugin &) = delete;
      Plugin(Plugin &&) = delete;
      Plugin &operator=(const Plugin &) = delete;
      Plugin &operator=(Plugin &&) = delete;

      /////////////////////////
      // Methods to override //
      /////////////////////////

      //-------------//
      // clap_plugin //
      //-------------//
      virtual bool init() noexcept { return true; }
      virtual bool
      activate(double sampleRate, uint32_t minFrameCount, uint32_t maxFrameCount) noexcept {
         return true;
      }
      virtual void deactivate() noexcept {}
      virtual bool startProcessing() noexcept { return true; }
      virtual void stopProcessing() noexcept {}
      virtual clap_process_status process(const clap_process *process) noexcept {
         return CLAP_PROCESS_SLEEP;
      }
      virtual void reset() noexcept {}
      virtual void onMainThread() noexcept {}
      virtual const void *extension(const char *id) noexcept { return nullptr; }

      //---------------------//
      // clap_plugin_latency //
      //---------------------//
      virtual bool implementsLatency() const noexcept { return false; }
      virtual uint32_t latencyGet() const noexcept { return 0; }

      //------------------//
      // clap_plugin_tail //
      //------------------//
      virtual bool implementsTail() const noexcept { return false; }
      virtual uint32_t tailGet() const noexcept { return 0; }

      //--------------------//
      // clap_plugin_render //
      //--------------------//
      virtual bool implementsRender() const noexcept { return false; }
      virtual bool renderHasHardRealtimeRequirement() noexcept { return false; }
      virtual bool renderSetMode(clap_plugin_render_mode mode) noexcept { return false; }

      //-------------------------//
      // clap_plugin_thread_pool //
      //-------------------------//
      virtual bool implementsThreadPool() const noexcept { return false; }
      virtual void threadPoolExec(uint32_t taskIndex) noexcept {}

      //-------------------//
      // clap_plugin_state //
      //-------------------//
      virtual bool implementsState() const noexcept { return false; }
      virtual bool stateSave(const clap_ostream *stream) noexcept { return false; }
      virtual bool stateLoad(const clap_istream *stream) noexcept { return false; }

      //---------------------------//
      // clap_plugin_state_context //
      //---------------------------//
      virtual bool implementsStateContext() const noexcept { return false; }
      virtual bool stateContextSave(const clap_ostream *stream, uint32_t context) noexcept {
         return stateSave(stream);
      }
      virtual bool stateContextLoad(const clap_istream *stream, uint32_t context) noexcept {
         return stateLoad(stream);
      }

      //-------------------------//
      // clap_plugin_preset_load //
      //-------------------------//
      virtual bool implementsPresetLoad() const noexcept { return false; }
      virtual bool presetLoadFromFile(const char *path) noexcept { return false; }

      //------------------------//
      // clap_plugin_track_info //
      //------------------------//
      virtual bool implementsTrackInfo() const noexcept { return false; }
      virtual void trackInfoChanged() noexcept {}

      //-------------------------//
      // clap_plugin_audio_ports //
      //-------------------------//
      virtual bool implementsAudioPorts() const noexcept { return false; }
      virtual uint32_t audioPortsCount(bool isInput) const noexcept { return 0; }
      virtual bool
      audioPortsInfo(uint32_t index, bool isInput, clap_audio_port_info *info) const noexcept {
         return false;
      }

      //--------------------------------//
      // clap_plugin_audio_ports_config //
      //--------------------------------//

      virtual bool  implementsAudioPortsConfig() const noexcept { return false; }
      virtual uint32_t audioPortsConfigCount() const noexcept { return 0; }
      virtual bool audioPortsGetConfig(uint32_t index,
                                       clap_audio_ports_config *config) const noexcept {
         return false;
      }
      virtual bool audioPortsSetConfig(clap_id configId) noexcept { return false; }

      //--------------------//
      // clap_plugin_params //
      //--------------------//
      virtual bool implementsParams() const noexcept { return false; }
      virtual uint32_t paramsCount() const noexcept { return 0; }
      virtual bool paramsInfo(uint32_t paramIndex, clap_param_info *info) const noexcept {
         return false;
      }
      virtual bool paramsValue(clap_id paramId, double *value) noexcept { return false; }
      virtual bool
      paramsValueToText(clap_id paramId, double value, char *display, uint32_t size) noexcept {
         return false;
      }
      virtual bool paramsTextToValue(clap_id paramId, const char *display, double *value) noexcept {
         return false;
      }
      virtual void paramsFlush(const clap_input_events *in,
                               const clap_output_events *out) noexcept {}
      virtual bool isValidParamId(clap_id paramId) const noexcept;

      //----------------------------//
      // clap_plugin_quick_controls //
      //----------------------------//
      virtual bool implementQuickControls() const noexcept { return false; }
      virtual uint32_t quickControlsPageCount() noexcept { return 0; }
      virtual bool quickControlsPageGet(uint32_t pageIndex,
                                        clap_quick_controls_page *page) noexcept {
         return false;
      }
      virtual void quickControlsSelectPage(clap_id pageId) noexcept {}
      virtual clap_id quickControlsSelectedPage() noexcept { return CLAP_INVALID_ID; }

      //------------------------//
      // clap_plugin_note_ports //
      //------------------------//
      virtual bool implementsNotePorts() const noexcept { return false; }
      virtual uint32_t notePortsCount(bool isInput) const noexcept { return 0; }
      virtual bool
      notePortsInfo(uint32_t index, bool isInput, clap_note_port_info *info) const noexcept {
         return false;
      }

      //-----------------------//
      // clap_plugin_note_name //
      //-----------------------//
      virtual bool implementsNoteName() const noexcept { return false; }
      virtual int noteNameCount() noexcept { return 0; }
      virtual bool noteNameGet(int index, clap_note_name *noteName) noexcept { return false; }

      //---------------------------//
      // clap_plugin_timer_support //
      //---------------------------//
      virtual bool implementsTimerSupport() const noexcept { return false; }
      virtual void onTimer(clap_id timerId) noexcept {}

      //------------------------------//
      // clap_plugin_posix_fd_support //
      //------------------------------//
      virtual bool implementsPosixFdSupport() const noexcept { return false; }
      virtual void onPosixFd(int fd, int flags) noexcept {}

      //-----------------//
      // clap_plugin_gui //
      //-----------------//
      virtual bool implementsGui() const noexcept { return false; }
      virtual bool guiIsApiSupported(const char *api, bool isFloating) noexcept { return false; }
      virtual bool guiGetPreferredApi(const char **api, bool *is_floating) noexcept {
         return false;
      }
      virtual bool guiCreate(const char *api, bool isFloating) noexcept { return false; }
      virtual void guiDestroy() noexcept {}
      virtual bool guiSetScale(double scale) noexcept { return false; }
      virtual bool guiShow() noexcept { return false; }
      virtual bool guiHide() noexcept { return false; }
      virtual bool guiGetSize(uint32_t *width, uint32_t *height) noexcept { return false; }
      virtual bool guiCanResize() const noexcept { return false; }
      virtual bool guiGetResizeHints(clap_gui_resize_hints_t *hints) noexcept { return false; }
      virtual bool guiAdjustSize(uint32_t *width, uint32_t *height) noexcept {
         return guiGetSize(width, height);
      }
      virtual bool guiSetSize(uint32_t width, uint32_t height) noexcept { return false; }
      virtual void guiSuggestTitle(const char *title) noexcept {}
      virtual bool guiSetParent(const clap_window *window) noexcept { return false; }
      virtual bool guiSetTransient(const clap_window *window) noexcept { return false; }

      //------------------------//
      // clap_plugin_voice_info //
      //------------------------//
      virtual bool implementsVoiceInfo() const noexcept { return false; }
      virtual bool voiceInfoGet(clap_voice_info *info) noexcept { return false; }

      /////////////
      // Logging //
      /////////////
      void log(clap_log_severity severity, const char *msg) const noexcept;
      void hostMisbehaving(const char *msg) const noexcept;
      void hostMisbehaving(const std::string &msg) const noexcept { hostMisbehaving(msg.c_str()); }

      // Receives a copy of all the logging messages sent to the host.
      // This is useful to have the messages in both the host's logs and the plugin's logs.
      virtual void logTee(clap_log_severity severity, const char *msg) const noexcept {}

      /////////////////////
      // Thread Checking //
      /////////////////////
      void checkMainThread() const noexcept;
      void checkAudioThread() const noexcept;
      void checkParamThread() const noexcept;
      void ensureMainThread(const char *method) const noexcept;
      void ensureAudioThread(const char *method) const noexcept;
      void ensureParamThread(const char *method) const noexcept;

      ///////////////
      // Utilities //
      ///////////////
      static Plugin &from(const clap_plugin *plugin, bool requireInitialized = true) noexcept;

      // runs the callback immediately if on the main thread, otherwise queue it.
      // be aware that the callback may be ran during the plugin destruction phase,
      // so check isBeingDestroyed() and ajust your code.
      void runOnMainThread(std::function<void()> callback);

      // This actually runs callbacks on the main thread, you should not need to call it
      void runCallbacksOnMainThread();

      static uint32_t compareAudioPortsInfo(const clap_audio_port_info &a,
                                            const clap_audio_port_info &b) noexcept;

      //////////////////////
      // Processing State //
      //////////////////////
      bool isActive() const noexcept { return _isActive; }
      bool isProcessing() const noexcept { return _isProcessing; }
      int sampleRate() const noexcept {
         assert(_isActive && "sample rate is only known if the plugin is active");
         assert(_sampleRate > 0);
         return _sampleRate;
      }

      bool isBeingDestroyed() const noexcept { return _isBeingDestroyed; }

   protected:
      HostProxy<h, l> _host;

   private:
      void ensureInitialized(const char *method) const noexcept;

      /////////////////////
      // CLAP Interfaces //
      /////////////////////

      clap_plugin _plugin;
      // clap_plugin
      static bool clapInit(const clap_plugin *plugin) noexcept;
      static void clapDestroy(const clap_plugin *plugin) noexcept;
      static bool clapActivate(const clap_plugin *plugin,
                               double sample_rate,
                               uint32_t minFrameCount,
                               uint32_t maxFrameCount) noexcept;
      static void clapDeactivate(const clap_plugin *plugin) noexcept;
      static bool clapStartProcessing(const clap_plugin *plugin) noexcept;
      static void clapStopProcessing(const clap_plugin *plugin) noexcept;
      static void clapReset(const clap_plugin *plugin) noexcept;
      static clap_process_status clapProcess(const clap_plugin *plugin,
                                             const clap_process *process) noexcept;
      static void clapOnMainThread(const clap_plugin *plugin) noexcept;
      static const void *clapExtension(const clap_plugin *plugin, const char *id) noexcept;

      // latency
      static uint32_t clapLatencyGet(const clap_plugin *plugin) noexcept;

      // clap_plugin_tail
      static uint32_t clapTailGet(const clap_plugin_t *plugin) noexcept;

      // clap_plugin_render
      static bool clapRenderHasHardRealtimeRequirement(const clap_plugin_t *plugin) noexcept;
      static bool clapRenderSetMode(const clap_plugin *plugin,
                                    clap_plugin_render_mode mode) noexcept;

      // clap_plugin_thread_pool
      static void clapThreadPoolExec(const clap_plugin *plugin, uint32_t task_index) noexcept;

      // clap_plugin_state
      static bool clapStateSave(const clap_plugin *plugin, const clap_ostream *stream) noexcept;
      static bool clapStateLoad(const clap_plugin *plugin, const clap_istream *stream) noexcept;

      // clap_plugin_state_context
      static bool clapStateContextSave(const clap_plugin *plugin,
                                       const clap_ostream *stream,
                                       uint32_t context) noexcept;
      static bool clapStateContextLoad(const clap_plugin *plugin,
                                       const clap_istream *stream,
                                       uint32_t context) noexcept;

      // clap_plugin_preset
      static bool clapPresetLoadFromFile(const clap_plugin *plugin, const char *path) noexcept;

      // clap_plugin_track_info
      static void clapTrackInfoChanged(const clap_plugin *plugin) noexcept;

      // clap_plugin_audio_ports
      static uint32_t clapAudioPortsCount(const clap_plugin *plugin, bool is_input) noexcept;
      static bool clapAudioPortsInfo(const clap_plugin *plugin,
                                     uint32_t index,
                                     bool is_input,
                                     clap_audio_port_info *info) noexcept;
      static uint32_t clapAudioPortsConfigCount(const clap_plugin *plugin) noexcept;
      static bool clapAudioPortsGetConfig(const clap_plugin *plugin,
                                          uint32_t index,
                                          clap_audio_ports_config *config) noexcept;
      static bool clapAudioPortsSetConfig(const clap_plugin *plugin, clap_id config_id) noexcept;

      // clap_plugin_params
      static uint32_t clapParamsCount(const clap_plugin *plugin) noexcept;
      static bool clapParamsInfo(const clap_plugin *plugin,
                                 uint32_t param_index,
                                 clap_param_info *param_info) noexcept;
      static bool
      clapParamsValue(const clap_plugin *plugin, clap_id param_id, double *value) noexcept;
      static bool clapParamsValueToText(const clap_plugin *plugin,
                                        clap_id param_id,
                                        double value,
                                        char *display,
                                        uint32_t size) noexcept;
      static bool clapParamsTextToValue(const clap_plugin *plugin,
                                        clap_id param_id,
                                        const char *display,
                                        double *value) noexcept;
      static void clapParamsFlush(const clap_plugin *plugin,
                                  const clap_input_events *in,
                                  const clap_output_events *out) noexcept;

      // clap_plugin_quick_controls
      static uint32_t clapQuickControlsPageCount(const clap_plugin *plugin) noexcept;
      static bool clapQuickControlsPageGet(const clap_plugin *plugin,
                                           uint32_t page_index,
                                           clap_quick_controls_page *page) noexcept;

      // clap_plugin_note_port
      static uint32_t clapNotePortsCount(const clap_plugin *plugin, bool is_input) noexcept;
      static bool clapNotePortsInfo(const clap_plugin *plugin,
                                    uint32_t index,
                                    bool is_input,
                                    clap_note_port_info *info) noexcept;

      // clap_plugin_note_name
      static uint32_t clapNoteNameCount(const clap_plugin *plugin) noexcept;
      static bool clapNoteNameGet(const clap_plugin *plugin,
                                  uint32_t index,
                                  clap_note_name *note_name) noexcept;

      // clap_plugin_timer_support
      static void clapOnTimer(const clap_plugin *plugin, clap_id timer_id) noexcept;

      // clap_plugin_fd_support
      static void
      clapOnPosixFd(const clap_plugin *plugin, int fd, clap_posix_fd_flags_t flags) noexcept;

      // clap_plugin_gui
      static bool
      clapGuiIsApiSupported(const clap_plugin *plugin, const char *api, bool isFloating) noexcept;
      static bool clapGuiGetPreferredApi(const clap_plugin_t *plugin,
                                         const char **api,
                                         bool *is_floating) noexcept;
      static bool
      clapGuiCreate(const clap_plugin *plugin, const char *api, bool isFloating) noexcept;
      static void clapGuiDestroy(const clap_plugin *plugin) noexcept;
      static bool clapGuiSetScale(const clap_plugin *plugin, double scale) noexcept;
      static bool
      clapGuiGetSize(const clap_plugin *plugin, uint32_t *width, uint32_t *height) noexcept;
      static bool
      clapGuiSetSize(const clap_plugin *plugin, uint32_t width, uint32_t height) noexcept;
      static bool clapGuiCanResize(const clap_plugin *plugin) noexcept;
      static bool clapGuiGetResizeHints(const clap_plugin_t *plugin,
                                        clap_gui_resize_hints_t *hints) noexcept;
      static bool
      clapGuiAdjustSize(const clap_plugin *plugin, uint32_t *width, uint32_t *height) noexcept;
      static bool clapGuiShow(const clap_plugin *plugin) noexcept;
      static bool clapGuiHide(const clap_plugin *plugin) noexcept;
      static bool clapGuiSetParent(const clap_plugin *plugin, const clap_window *window) noexcept;
      static bool clapGuiSetTransient(const clap_plugin *plugin,
                                      const clap_window *window) noexcept;

      static void clapGuiSuggestTitle(const clap_plugin *plugin, const char *title) noexcept;

      static bool clapVoiceInfoGet(const clap_plugin *plugin, clap_voice_info *info) noexcept;

      // interfaces
      static const clap_plugin_render _pluginRender;
      static const clap_plugin_thread_pool _pluginThreadPool;
      static const clap_plugin_state _pluginState;
      static const clap_plugin_state_context _pluginStateContext;
      static const clap_plugin_preset_load _pluginPresetLoad;
      static const clap_plugin_track_info _pluginTrackInfo;
      static const clap_plugin_audio_ports _pluginAudioPorts;
      static const clap_plugin_audio_ports_config _pluginAudioPortsConfig;
      static const clap_plugin_params _pluginParams;
      static const clap_plugin_quick_controls _pluginQuickControls;
      static const clap_plugin_latency _pluginLatency;
      static const clap_plugin_note_ports _pluginNotePorts;
      static const clap_plugin_note_name _pluginNoteName;
      static const clap_plugin_timer_support _pluginTimerSupport;
      static const clap_plugin_posix_fd_support _pluginPosixFdSupport;
      static const clap_plugin_gui _pluginGui;
      static const clap_plugin_voice_info _pluginVoiceInfo;
      static const clap_plugin_tail _pluginTail;

      // state
      bool _wasInitialized = false;
      bool _isActive = false;
      bool _isProcessing = false;
      bool _isBeingDestroyed = false;
      double _sampleRate = 0;

      std::string _guiApi;
      bool _isGuiCreated = false;
      bool _isGuiFloating = false;
      bool _isGuiEmbedded = false;

      std::mutex _mainThredCallbacksLock;
      std::queue<std::function<void()>> _mainThredCallbacks;
   };
}} // namespace clap::helpers
