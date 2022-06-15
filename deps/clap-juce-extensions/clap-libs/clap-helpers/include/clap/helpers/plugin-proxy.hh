#pragma once

#include "host-proxy.hh"

namespace clap { namespace helpers {
   template <MisbehaviourHandler h, CheckingLevel l>
   class PluginProxy {
   public:
      PluginProxy(const clap_host *host, const clap_plugin *plugin);

      void init();

      template <typename T>
      void getExtension(const T *&ptr, const char *id) noexcept;

   protected:
      void ensureMainThread(const char *method) const noexcept;
      void ensureAudioThread(const char *method) const noexcept;

      HostProxy<h, l> _host;
      const clap_plugin *const _plugin;

      const clap_plugin_params *_pluginParams = nullptr;
      const clap_plugin_quick_controls *_pluginQuickControls = nullptr;
      const clap_plugin_audio_ports *_pluginAudioPorts = nullptr;
      const clap_plugin_gui *_pluginGui = nullptr;
      const clap_plugin_gui_x11 *_pluginGuiX11 = nullptr;
      const clap_plugin_gui_win32 *_pluginGuiWin32 = nullptr;
      const clap_plugin_gui_cocoa *_pluginGuiCocoa = nullptr;
      const clap_plugin_gui_free_standing *_pluginGuiFreeStanding = nullptr;
      const clap_plugin_timer_support *_pluginTimerSupport = nullptr;
      const clap_plugin_fd_support *_pluginFdSupport = nullptr;
      const clap_plugin_thread_pool *_pluginThreadPool = nullptr;
      const clap_plugin_preset_load *_pluginPresetLoad = nullptr;
      const clap_plugin_state *_pluginState = nullptr;
   };
}} // namespace clap::helpers