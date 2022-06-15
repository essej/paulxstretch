#pragma once

#include <cassert>
#include <iostream>
#include <sstream>

#include "host-proxy.hh"

namespace clap { namespace helpers {
   template <MisbehaviourHandler h, CheckingLevel l>
   HostProxy<h, l>::HostProxy(const ::clap_host *host) : _host(host) {}

   template <MisbehaviourHandler h, CheckingLevel l>
   void HostProxy<h, l>::init() {
      getExtension(_hostLog, CLAP_EXT_LOG);
      getExtension(_hostThreadCheck, CLAP_EXT_THREAD_CHECK);
      getExtension(_hostThreadPool, CLAP_EXT_THREAD_POOL);
      getExtension(_hostAudioPorts, CLAP_EXT_AUDIO_PORTS);
      getExtension(_hostAudioPortsConfig, CLAP_EXT_AUDIO_PORTS_CONFIG);
      getExtension(_hostNotePorts, CLAP_EXT_NOTE_PORTS);
      getExtension(_hostTimerSupport, CLAP_EXT_TIMER_SUPPORT);
      getExtension(_hostPosixFdSupport, CLAP_EXT_POSIX_FD_SUPPORT);
      getExtension(_hostFileReference, CLAP_EXT_FILE_REFERENCE);
      getExtension(_hostLatency, CLAP_EXT_LATENCY);
      getExtension(_hostGui, CLAP_EXT_GUI);
      getExtension(_hostParams, CLAP_EXT_PARAMS);
      getExtension(_hostTrackInfo, CLAP_EXT_TRACK_INFO);
      getExtension(_hostState, CLAP_EXT_STATE);
      getExtension(_hostNoteName, CLAP_EXT_NOTE_NAME);
      getExtension(_hostQuickControls, CLAP_EXT_QUICK_CONTROLS);
      getExtension(_hostVoiceInfo, CLAP_EXT_VOICE_INFO);
   }

   template <MisbehaviourHandler h, CheckingLevel l>
   template <typename T>
   void HostProxy<h, l>::getExtension(const T *&ptr, const char *id) noexcept {
      assert(!ptr);
      assert(id);

      if (_host->get_extension)
         ptr = static_cast<const T *>(_host->get_extension(_host, id));
   }

   ///////////////
   // clap_host //
   ///////////////
   template <MisbehaviourHandler h, CheckingLevel l>
   void HostProxy<h, l>::requestCallback() noexcept {
      _host->request_callback(_host);
   }

   template <MisbehaviourHandler h, CheckingLevel l>
   void HostProxy<h, l>::requestRestart() noexcept {
      _host->request_restart(_host);
   }

   template <MisbehaviourHandler h, CheckingLevel l>
   void HostProxy<h, l>::requestProcess() noexcept {
      _host->request_process(_host);
   }

   /////////////
   // Logging //
   /////////////
   template <MisbehaviourHandler h, CheckingLevel l>
   bool HostProxy<h, l>::canUseHostLog() const noexcept {
      return _hostLog && _hostLog->log;
   }

   template <MisbehaviourHandler h, CheckingLevel l>
   void HostProxy<h, l>::log(clap_log_severity severity, const char *msg) const noexcept {
      if (canUseHostLog()) {
         _hostLog->log(_host, severity, msg);
         return;
      }

      switch (severity) {
      case CLAP_LOG_ERROR:
      case CLAP_LOG_HOST_MISBEHAVING:
         std::cerr << msg << std::endl;
         break;

      default:
         std::clog << msg << std::endl;
         break;
      }
   }

   template <MisbehaviourHandler h, CheckingLevel l>
   void HostProxy<h, l>::hostMisbehaving(const char *msg) const noexcept {
      log(CLAP_LOG_HOST_MISBEHAVING, msg);

      if (h == MisbehaviourHandler::Terminate)
         std::terminate();
   }

   template <MisbehaviourHandler h, CheckingLevel l>
   void HostProxy<h, l>::pluginMisbehaving(const char *msg) const noexcept {
      log(CLAP_LOG_PLUGIN_MISBEHAVING, msg);

      if (h == MisbehaviourHandler::Terminate)
         std::terminate();
   }

   //////////////////
   // Thread Check //
   //////////////////
   template <MisbehaviourHandler h, CheckingLevel l>
   bool HostProxy<h, l>::canUseThreadCheck() const noexcept {
      return _hostThreadCheck && _hostThreadCheck->is_audio_thread &&
             _hostThreadCheck->is_main_thread;
   }

   template <MisbehaviourHandler h, CheckingLevel l>
   bool HostProxy<h, l>::isMainThread() const noexcept {
      assert(canUseThreadCheck());
      return _hostThreadCheck->is_main_thread(_host);
   }

   template <MisbehaviourHandler h, CheckingLevel l>
   bool HostProxy<h, l>::isAudioThread() const noexcept {
      assert(canUseThreadCheck());
      return _hostThreadCheck->is_audio_thread(_host);
   }

   template <MisbehaviourHandler h, CheckingLevel l>
   void HostProxy<h, l>::ensureMainThread(const char *method) const noexcept {
      if (l == CheckingLevel::None)
         return;

      if (!canUseThreadCheck() || isMainThread())
         return;

      std::ostringstream msg;
      msg << "Plugin called the method clap_host_" << method
          << "() on wrong thread! It must be called on main thread!";
      pluginMisbehaving(msg.str());
   }

   template <MisbehaviourHandler h, CheckingLevel l>
   void HostProxy<h, l>::ensureAudioThread(const char *method) const noexcept {
      if (l == CheckingLevel::None)
         return;

      if (!canUseThreadCheck() || isAudioThread())
         return;

      std::ostringstream msg;
      msg << "Plugin called the method clap_host_" << method
          << "() on wrong thread! It must be called on audio thread!";
      pluginMisbehaving(msg.str());
   }

   //////////////////////////////////
   // clap_host_audio_ports_config //
   //////////////////////////////////
   template <MisbehaviourHandler h, CheckingLevel l>
   bool HostProxy<h, l>::canUseAudioPortsConfig() const noexcept {
      if (!_hostAudioPortsConfig)
         return false;

      if (_hostAudioPortsConfig->rescan)
         return true;

      hostMisbehaving("clap_host_audio_ports_config is partially implemented");
      return false;
   }

   template <MisbehaviourHandler h, CheckingLevel l>
   void HostProxy<h, l>::audioPortsConfigRescan() const noexcept {
      assert(canUseAudioPortsConfig());
      ensureMainThread("audio_ports_config.rescan");
      _hostAudioPortsConfig->rescan(_host);
   }

   ///////////////////////////
   // clap_host_audio_ports //
   ///////////////////////////
   template <MisbehaviourHandler h, CheckingLevel l>
   bool HostProxy<h, l>::canUseAudioPorts() const noexcept {
      if (!_hostAudioPorts)
         return false;

      if (_hostAudioPorts->rescan)
         return true;

      hostMisbehaving("clap_host_audio_ports is partially implemented");
      return false;
   }

   template <MisbehaviourHandler h, CheckingLevel l>
   void HostProxy<h, l>::audioPortsRescan(uint32_t flags) const noexcept {
      assert(canUseAudioPorts());
      ensureMainThread("audio_ports.rescan");
      _hostAudioPorts->rescan(_host, flags);
   }

   //////////////////////////
   // clap_host_note_ports //
   //////////////////////////
   template <MisbehaviourHandler h, CheckingLevel l>
   bool HostProxy<h, l>::canUseNotePorts() const noexcept {
      if (!_hostNotePorts)
         return false;

      if (_hostNotePorts->rescan)
         return true;

      hostMisbehaving("clap_host_note_ports is partially implemented");
      return false;
   }

   template <MisbehaviourHandler h, CheckingLevel l>
   void HostProxy<h, l>::notePortsRescan(uint32_t flags) const noexcept {
      assert(canUseNotePorts());
      ensureMainThread("note_ports.rescan");
      _hostNotePorts->rescan(_host, flags);
   }

   /////////////////////
   // clap_host_state //
   /////////////////////
   template <MisbehaviourHandler h, CheckingLevel l>
   bool HostProxy<h, l>::canUseState() const noexcept {
      if (!_hostState)
         return false;

      if (_hostState->mark_dirty)
         return true;

      hostMisbehaving("clap_host_state is partially implemented");
      return false;
   }

   template <MisbehaviourHandler h, CheckingLevel l>
   void HostProxy<h, l>::stateMarkDirty() const noexcept {
      assert(canUseState());
      ensureMainThread("state.mark_dirty");
      _hostState->mark_dirty(_host);
   }

   ///////////////////////
   // clap_host_latency //
   ///////////////////////
   template <MisbehaviourHandler h, CheckingLevel l>
   bool HostProxy<h, l>::canUseLatency() const noexcept {
      if (!_hostLatency)
         return false;

      if (_hostLatency->changed)
         return true;

      hostMisbehaving("clap_host_latency is partially implemented");
      return false;
   }

   template <MisbehaviourHandler h, CheckingLevel l>
   void HostProxy<h, l>::latencyChanged() const noexcept {
      assert(canUseLatency());
      ensureMainThread("latency.changed");
      _hostLatency->changed(_host);
   }

   ////////////////////
   // clap_host_tail //
   ////////////////////
   template <MisbehaviourHandler h, CheckingLevel l>
   bool HostProxy<h, l>::canUseTail() const noexcept {
      if (!_hostTail)
         return false;

      if (_hostLatency->changed)
         return true;

      hostMisbehaving("clap_host_tail is partially implemented");
      return false;
   }

   template <MisbehaviourHandler h, CheckingLevel l>
   void HostProxy<h, l>::tailChanged() const noexcept {
      assert(canUseTail());
      ensureAudioThread("tail.changed");
      _hostTail->changed(_host);
   }

   /////////////////////////
   // clap_host_note_name //
   /////////////////////////
   template <MisbehaviourHandler h, CheckingLevel l>
   bool HostProxy<h, l>::canUseNoteName() const noexcept {
      if (!_hostNoteName)
         return false;

      if (_hostNoteName->changed)
         return true;

      hostMisbehaving("clap_host_note_name is partially implemented");
      return false;
   }

   template <MisbehaviourHandler h, CheckingLevel l>
   void HostProxy<h, l>::noteNameChanged() const noexcept {
      assert(canUseNoteName());
      ensureMainThread("note_name.changed");
      _hostNoteName->changed(_host);
   }

   //////////////////////
   // clap_host_params //
   //////////////////////
   template <MisbehaviourHandler h, CheckingLevel l>
   bool HostProxy<h, l>::canUseParams() const noexcept {
      if (!_hostParams)
         return false;

      if (_hostParams->rescan && _hostParams->clear && _hostParams->request_flush)
         return true;

      hostMisbehaving("clap_host_params is partially implemented");
      return false;
   }

   template <MisbehaviourHandler h, CheckingLevel l>
   void HostProxy<h, l>::paramsRescan(clap_param_rescan_flags flags) const noexcept {
      assert(canUseParams());
      ensureMainThread("params.rescan");
      _hostParams->rescan(_host, flags);
   }

   template <MisbehaviourHandler h, CheckingLevel l>
   void HostProxy<h, l>::paramsClear(clap_id param_id,
                                     clap_param_clear_flags flags) const noexcept {
      assert(canUseParams());
      ensureMainThread("params.clear");
      _hostParams->clear(_host, param_id, flags);
   }

   template <MisbehaviourHandler h, CheckingLevel l>
   void HostProxy<h, l>::paramsRequestFlush() const noexcept {
      assert(canUseParams());
      _hostParams->request_flush(_host);
   }

   //////////////////////////
   // clap_host_track_info //
   //////////////////////////
   template <MisbehaviourHandler h, CheckingLevel l>
   bool HostProxy<h, l>::canUseTrackInfo() const noexcept {
      if (!_hostTrackInfo)
         return false;

      if (_hostTrackInfo->get)
         return true;

      hostMisbehaving("clap_host_track_info is partially implemented");
      return false;
   }

   template <MisbehaviourHandler h, CheckingLevel l>
   bool HostProxy<h, l>::trackInfoGet(clap_track_info *info) const noexcept {
      assert(canUseTrackInfo());
      ensureMainThread("track_info.get");
      return _hostTrackInfo->get(_host, info);
   }

   ///////////////////
   // clap_host_gui //
   ///////////////////
   template <MisbehaviourHandler h, CheckingLevel l>
   bool HostProxy<h, l>::canUseGui() const noexcept {
      if (!_hostGui)
         return false;

      if (_hostGui->resize_hints_changed && _hostGui->request_resize && _hostGui->request_hide &&
          _hostGui->request_show && _hostGui->closed)
         return true;

      hostMisbehaving("clap_host_gui is partially implemented");
      return false;
   }

   template <MisbehaviourHandler h, CheckingLevel l>
   bool HostProxy<h, l>::guiRequestResize(uint32_t width, uint32_t height) const noexcept {
      assert(canUseGui());
      return _hostGui->request_resize(_host, width, height);
   }

   template <MisbehaviourHandler h, CheckingLevel l>
   bool HostProxy<h, l>::guiRequestShow() const noexcept {
      assert(canUseGui());
      return _hostGui->request_show(_host);
   }

   template <MisbehaviourHandler h, CheckingLevel l>
   bool HostProxy<h, l>::guiRequestHide() const noexcept {
      assert(canUseGui());
      return _hostGui->request_hide(_host);
   }

   template <MisbehaviourHandler h, CheckingLevel l>
   void HostProxy<h, l>::guiClosed(bool wasDestroyed) const noexcept {
      assert(canUseGui());
      _hostGui->closed(_host, wasDestroyed);
   }

   ///////////////////
   // Timer Support //
   ///////////////////
   template <MisbehaviourHandler h, CheckingLevel l>
   bool HostProxy<h, l>::canUseTimerSupport() const noexcept {
      if (!_hostTimerSupport)
         return false;

      auto &x = *_hostTimerSupport;
      if (x.register_timer && x.unregister_timer)
         return true;

      hostMisbehaving("clap_timer_support is partially implemented");
      return false;
   }

   template <MisbehaviourHandler h, CheckingLevel l>
   bool HostProxy<h, l>::timerSupportRegister(uint32_t period_ms,
                                              clap_id *timer_id) const noexcept {
      assert(canUseTimerSupport());
      ensureMainThread("timer_support.register_timer");
      return _hostTimerSupport->register_timer(_host, period_ms, timer_id);
   }

   template <MisbehaviourHandler h, CheckingLevel l>
   bool HostProxy<h, l>::timerSupportUnregister(clap_id timer_id) const noexcept {
      assert(canUseTimerSupport());
      ensureMainThread("timer_support.unregister_timer");
      return _hostTimerSupport->unregister_timer(_host, timer_id);
   }

   //////////////////////////
   // clap_host_fd_support //
   //////////////////////////
   template <MisbehaviourHandler h, CheckingLevel l>
   bool HostProxy<h, l>::canUsePosixFdSupport() const noexcept {
      if (!_hostPosixFdSupport)
         return false;

      auto &x = *_hostPosixFdSupport;
      if (x.modify_fd && x.register_fd && x.unregister_fd)
         return true;

      hostMisbehaving("clap_posix_fd_support is partially implemented");
      return false;
   }

   template <MisbehaviourHandler h, CheckingLevel l>
   bool HostProxy<h, l>::posixFdSupportRegister(int fd, int flags) const noexcept {
      assert(canUsePosixFdSupport());
      ensureMainThread("posix_fd_support.register");
      return _hostPosixFdSupport->register_fd(_host, fd, flags);
   }

   template <MisbehaviourHandler h, CheckingLevel l>
   bool HostProxy<h, l>::posixFdSupportModify(int fd, int flags) const noexcept {
      assert(canUsePosixFdSupport());
      ensureMainThread("posix_fd_support.modify");
      return _hostPosixFdSupport->modify_fd(_host, fd, flags);
   }

   template <MisbehaviourHandler h, CheckingLevel l>
   bool HostProxy<h, l>::posixFdSupportUnregister(int fd) const noexcept {
      assert(canUsePosixFdSupport());
      ensureMainThread("posix_fd_support.unregister");
      return _hostPosixFdSupport->unregister_fd(_host, fd);
   }

   ///////////////////////////
   // clap_host_thread_pool //
   ///////////////////////////
   template <MisbehaviourHandler h, CheckingLevel l>
   bool HostProxy<h, l>::canUseThreadPool() const noexcept {
      if (!_hostThreadPool)
         return false;

      if (_hostThreadPool->request_exec)
         return true;

      hostMisbehaving("clap_host_thread_pool is partially implemented");
      return false;
   }

   template <MisbehaviourHandler h, CheckingLevel l>
   bool HostProxy<h, l>::threadPoolRequestExec(uint32_t numTasks) const noexcept {
      assert(canUseThreadPool());
      ensureAudioThread("thread_pool.request_exec");
      return _hostThreadPool->request_exec(_host, numTasks);
   }

   //////////////////////////
   // clap_host_voice_info //
   //////////////////////////
   template <MisbehaviourHandler h, CheckingLevel l>
   bool HostProxy<h, l>::canUseVoiceInfo() const noexcept {
      if (!_hostVoiceInfo)
         return false;

      if (_hostVoiceInfo->changed)
         return true;

      hostMisbehaving("clap_host_voice_info is partially implemented");
      return false;
   }

   template <MisbehaviourHandler h, CheckingLevel l>
   void HostProxy<h, l>::voiceInfoChanged() const noexcept {
      assert(canUseVoiceInfo());
      ensureMainThread("voice_info.changed");
      return _hostVoiceInfo->changed(_host);
   }

   //////////////////////////////
   // clap_host_quick_controls //
   //////////////////////////////
   template <MisbehaviourHandler h, CheckingLevel l>
   bool HostProxy<h, l>::canUseQuickControls() const noexcept {
      if (!_hostQuickControls)
         return false;

      if (_hostQuickControls->changed && _hostQuickControls->suggest_page)
         return true;

      hostMisbehaving("clap_host_quick_controls is partially implemented");
      return false;
   }

   template <MisbehaviourHandler h, CheckingLevel l>
   void HostProxy<h, l>::quickControlsChanged() const noexcept {
      assert(canUseQuickControls());
      ensureMainThread("quick_controls.changed");
      _hostQuickControls->changed(_host);
   }

   template <MisbehaviourHandler h, CheckingLevel l>
   void HostProxy<h, l>::quickControlsSuggestPage(clap_id page_id) const noexcept {
      assert(canUseQuickControls());
      ensureMainThread("quick_controls.suggest_page");
      _hostQuickControls->suggest_page(_host, page_id);
   }
}} // namespace clap::helpers