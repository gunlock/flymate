#include "settings.h"
#include <XPLMDefs.h>
#include <XPLMDisplay.h>
#include <XPLMPlugin.h>
#include <XPLMUtilities.h>
#include <fstream>
#include <iostream>

using std::expected;
using std::ofstream;
using std::string;
using std::unexpected;
namespace fs = std::filesystem;

constexpr char kAppName[] = "flymate";

// Get platform-specific user app settings path
static expected<fs::path, string> get_app_settings_dir() {
  fs::path base_path;

#if defined(_WIN32)
  if (const char* appdata = std::getenv("APPDATA")) {
    base_path = fs::path(appdata) / kAppName;
  } else {
    return unexpected("Environment variable %APPDATA% not found.");
  }
#elif defined(__APPLE__)
  if (const char* home = std::getenv("HOME")) {
    base_path = fs::path(home) / "Library" / "Application Support" / kAppName;
  } else {
    return unexpected("Environment variable $HOME not found.");
  }
#else
  // Linux/Unix fallback
  if (const char* xdg = std::getenv("XDG_CONFIG_HOME")) {
    base_path = fs::path(xdg) / kAppName;
  } else if (const char* home = std::getenv("HOME")) {
    base_path = fs::path(home) / ".config" / kAppName;
  } else {
    return std::unexpected("Could not determine user config directory.");
  }
#endif

  return base_path;
}

namespace flymate {

// Uses XPLMDebugString rather than spdlog because the logger
// is not initialized until after init() completes.
void Settings::init() {
  if (auto path_res = get_app_settings_dir(); path_res) {
    fs::path settings_folder = *path_res;
    fs::path settings_file = settings_folder / "settings.json";

    std::error_code ec;
    // create_directories creates all missing parent folders (mkdir -p)
    if (!fs::create_directories(settings_folder, ec) && ec) {
      XPLMDebugString(
          ("FlyMate Error: Failed to create directory: " + ec.message() + "\n").c_str());
      isInitialized_ = false;
      return;
    }

    userSettingsPath_ = settings_file;

    // Check if file exists; if not, create a default one
    if (!fs::exists(settings_file)) {
      ofstream outfile(settings_file);
      if (!outfile) {
        XPLMDebugString(
            ("FlyMate Error: Failed to create settings file at: " + settings_file.string() + "\n")
                .c_str());
        isInitialized_ = false;
        return;
      }
      // don't write to it, leave the newly created file blank
      outfile.close();
    }
    isInitialized_ = true;

    // X-Plane version
    XPLMHostApplicationID hostId;
    int xpVersion;
    XPLMGetVersions(&xpVersion, &xplmVersion_, &hostId);
    xpMajorVersion_ = xpVersion / 1000;
    xpMinorVersion_ = xpVersion % 1000;

    // Get plugin path
    // SDK: Each parameter should be a pointer to a buffer of at least 256 characters.
    //      Documentation is unclear what happens if path length > 256. Let's be
    //      conservative and reserve more
    char buf[1024];
    XPLMGetPluginInfo(XPLMGetMyID(), nullptr, buf, nullptr, nullptr);
    // buf = ".../FlyMate/lin_x64/FlyMate.xpl"
    // Strip filename, then platform dir (lin_x64, win_x64, mac_x64)
    pluginPath_ = fs::path(buf).parent_path().parent_path();

  } else {
    // Return the error from the path fetching step
    XPLMDebugString(("FlyMate Error: " + path_res.error() + "\n").c_str());
    isInitialized_ = false;
  }
}

expected<void, string> Settings::load() {
  if (!isInitialized_) {
    return unexpected("Settings not initialized.");
  }

  // If file is empty (newly created), keep defaults and write them out
  if (fs::file_size(userSettingsPath_) == 0) {
    return save();
  }

  string buffer;
  auto ec = glz::read_file_json(appData_, userSettingsPath_.string(), buffer);
  if (ec) {
    return unexpected(glz::format_error(ec, buffer));
  }

  return {};
}

expected<void, string> Settings::save() const {
  if (!isInitialized_) {
    return unexpected("Settings not initialized.");
  }

  string buffer;
  auto ec = glz::write_file_json(appData_, userSettingsPath_.string(), buffer);
  if (ec) {
    return unexpected(glz::format_error(ec, buffer));
  }

  return {}; // success;
}

expected<void, string> Settings::save(const AppData& appData) {
  if (!isInitialized_) {
    return unexpected("Settings not initialized.");
  }

  appData_ = appData;
  return save();
  return {}; // success
}

AppData Settings::getAppData() const {
  return appData_;
}

std::filesystem::path Settings::getUserSettingsPath() const {
  return userSettingsPath_;
}

bool Settings::isInitialized() const {
  return isInitialized_;
}

std::filesystem::path Settings::getPluginPath() const {
  return pluginPath_;
}

std::tuple<int, int> Settings::getXplaneVersion() const {
  return {xpMajorVersion_, xpMinorVersion_};
}

int Settings::getXPLMVersion() const {
  return xplmVersion_;
}

} // namespace flymate
