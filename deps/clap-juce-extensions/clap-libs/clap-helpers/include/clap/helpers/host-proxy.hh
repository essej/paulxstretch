#pragma once

#include <string>

#include <clap/clap.h>

#include "checking-level.hh"
#include "misbehaviour-handler.hh"

namespace clap { namespace helpers {
   template <MisbehaviourHandler h, CheckingLevel l>
   class HostProxy {
   public:
      HostProxy(const clap_host *host);

      void init();

      ///////////////
      // clap_host //
      ///////////////
      template <typename T>
      void getExtension(const T *&ptr, const char *id) noexcept;
      void requestCallback() noexcept;
      void requestRestart() noexcept;
      void requestProcess() noexcept;

      ///////////////////
      // clap_host_log //
      ///////////////////
      bool canUseHostLog() const noexcept;
      void log(clap_log_severity severity, const char *msg) const noexcept;
      void hostMisbehaving(const char *msg) const noexcept;
      void hostMisbehaving(const std::string &msg) const noexcept { hostMisbehaving(msg.c_str()); }
      void pluginMisbehaving(const char *msg) const noexcept;
      void pluginMisbehaving(const std::string &msg) const noexcept {
         pluginMisbehaving(msg.c_str());
      }

      ////////////////////////////
      // clap_host_thread_check //
      ////////////////////////////
      bool canUseThreadCheck() const noexcept;
      bool isMainThread() const noexcept;
      bool isAudioThread() const noexcept;

      //////////////////////////////////
      // clap_host_audio_ports_config //
      //////////////////////////////////
      bool canUseAudioPortsConfig() const noexcept;
      void audioPortsConfigRescan() const noexcept;

      ///////////////////////////
      // clap_host_audio_ports //
      ///////////////////////////
      bool canUseAudioPorts() const noexcept;
      void audioPortsRescan(uint32_t flags) const noexcept;

      //////////////////////////
      // clap_host_note_ports //
      //////////////////////////
      bool canUseNotePorts() const noexcept;
      void notePortsRescan(uint32_t flags) const noexcept;

      /////////////////////
      // clap_host_state //
      /////////////////////
      bool canUseState() const noexcept;
      void stateMarkDirty() const noexcept;

      ///////////////////////
      // clap_host_latency //
      ///////////////////////
      bool canUseLatency() const noexcept;
      void latencyChanged() const noexcept;

      ////////////////////
      // clap_host_tail //
      ////////////////////
      bool canUseTail() const noexcept;
      void tailChanged() const noexcept;

      /////////////////////////
      // clap_host_note_name //
      /////////////////////////
      bool canUseNoteName() const noexcept;
      void noteNameChanged() const noexcept;

      //////////////////////
      // clap_host_params //
      //////////////////////
      bool canUseParams() const noexcept;
      void paramsRescan(clap_param_rescan_flags flags) const noexcept;
      void paramsClear(clap_id param_id, clap_param_clear_flags flags) const noexcept;
      void paramsRequestFlush() const noexcept;

      //////////////////////////
      // clap_host_track_info //
      //////////////////////////
      bool canUseTrackInfo() const noexcept;
      bool trackInfoGet(clap_track_info *info) const noexcept;

      ///////////////////
      // clap_host_gui //
      ///////////////////
      bool canUseGui() const noexcept;
      bool guiRequestResize(uint32_t width, uint32_t height) const noexcept;
      bool guiRequestShow() const noexcept;
      bool guiRequestHide() const noexcept;
      void guiClosed(bool wasDestroyed) const noexcept;

      /////////////////////////////
      // clap_host_timer_support //
      /////////////////////////////
      bool canUseTimerSupport() const noexcept;
      bool timerSupportRegister(uint32_t period_ms, clap_id *timer_id) const noexcept;
      bool timerSupportUnregister(clap_id timer_id) const noexcept;

      //////////////////////////
      // clap_host_fd_support //
      //////////////////////////
      bool canUsePosixFdSupport() const noexcept;
      bool posixFdSupportRegister(int fd, int flags) const noexcept;
      bool posixFdSupportModify(int fd, int flags) const noexcept;
      bool posixFdSupportUnregister(int fd) const noexcept;

      //////////////////////////////
      // clap_host_quick_controls //
      //////////////////////////////
      bool canUseQuickControls() const noexcept;
      void quickControlsChanged() const noexcept;
      void quickControlsSuggestPage(clap_id page_id) const noexcept;

      ///////////////////////////
      // clap_host_thread_pool //
      ///////////////////////////
      bool canUseThreadPool() const noexcept;
      bool threadPoolRequestExec(uint32_t numTasks) const noexcept;

      //////////////////////////
      // clap_host_voice_info //
      //////////////////////////
      bool canUseVoiceInfo() const noexcept;
      void voiceInfoChanged() const noexcept;

   protected:
      void ensureMainThread(const char *method) const noexcept;
      void ensureAudioThread(const char *method) const noexcept;

      const clap_host *const _host;

      const clap_host_log *_hostLog = nullptr;
      const clap_host_thread_check *_hostThreadCheck = nullptr;
      const clap_host_thread_pool *_hostThreadPool = nullptr;
      const clap_host_audio_ports *_hostAudioPorts = nullptr;
      const clap_host_audio_ports_config *_hostAudioPortsConfig = nullptr;
      const clap_host_note_ports *_hostNotePorts = nullptr;
      const clap_host_file_reference *_hostFileReference = nullptr;
      const clap_host_latency *_hostLatency = nullptr;
      const clap_host_gui *_hostGui = nullptr;
      const clap_host_timer_support *_hostTimerSupport = nullptr;
      const clap_host_posix_fd_support *_hostPosixFdSupport = nullptr;
      const clap_host_params *_hostParams = nullptr;
      const clap_host_track_info *_hostTrackInfo = nullptr;
      const clap_host_state *_hostState = nullptr;
      const clap_host_note_name *_hostNoteName = nullptr;
      const clap_host_quick_controls *_hostQuickControls = nullptr;
      const clap_host_voice_info *_hostVoiceInfo = nullptr;
      const clap_host_tail *_hostTail = nullptr;
   };
}} // namespace clap::helpers