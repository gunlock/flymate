// #include "appstate.h"
#include "logging.h"
#include "settings.h"
#include <XPLMDefs.h>
#include <XPLMDisplay.h>
#include <XPLMPlugin.h>
#include <XPLMUtilities.h>
#include <cstdio>
#include <fmt/format.h>
#include <spdlog/spdlog.h>

#ifdef WINDOWS
#include <windows.h>
#endif

using std::string;

PLUGIN_API int XPluginStart(char* outName, char* outSig, char* outDesc) {
  // Buffer size documentented in the "Developing Plugins" article constexpr size_t BUF_SIZE = 256;
  constexpr size_t BUF_SIZE = 256;
  std::snprintf(outName, BUF_SIZE, "FlyMate");
  std::snprintf(outSig, BUF_SIZE, "com.flymate.plugin");
  std::snprintf(outDesc, BUF_SIZE, "FlyMate v%s - X-Plane Utility", FLYMATE_VERSION);

  flymate::settings.init();

  // Confirm settings are initialized, if not FATAL error
  if (!flymate::settings.isInitialized()) {
    XPLMDebugString("FlyMate Error: Settings could not be initialized.\n");
    return 0; // error
  }

  // Confirm settings can be loaded, if not FATAL error
  if (auto result = flymate::settings.load(); !result) {
    XPLMDebugString(
        fmt::format("FlyMate Error: Failed to load settings with error: {}", result.error())
            .c_str());
    return 0; // error
  }

  // Initialize logging, if not FATAL error
  if (auto result = flymate::logging::init(flymate::settings.getPluginPath()); !result) {
    XPLMDebugString(fmt::format("FlyMate Error: Logger init failed: {}. Shutting plugin down.\n",
                                result.error())
                        .c_str());
    return 0; // error
  }

  // Print start up log messages
  auto [xpMajorVersion, xpMinorVersion] = flymate::settings.getXplaneVersion();
  string logstr(fmt::format("FlyMate: Flymate v{}  X-Plane v{}.{}\n", FLYMATE_VERSION,
                            xpMajorVersion, xpMinorVersion));
  XPLMDebugString(logstr.c_str());
  spdlog::info("Flymate v{}. X-Plane v{}.{}", FLYMATE_VERSION, xpMajorVersion, xpMinorVersion);

  return 1; // success
}

PLUGIN_API int XPluginEnable() {
  // TODO: add enable code
  spdlog::info("XPluginEnable called");
  return 1;
}

PLUGIN_API void XPluginDisable() {
  // TODO: Add disable code
  spdlog::info("XPluginDisable called");
}

PLUGIN_API void XPluginStop() {
  spdlog::info("FlyMate: plugin stopped");
  // TODO: get window location and call flymate::settings.save()
  flymate::logging::shutdown();
}

PLUGIN_API void XPluginReceiveMessage(XPLMPluginID from, int msg, void* param) {
  (void)from;
  (void)msg;
  (void)param;
}
