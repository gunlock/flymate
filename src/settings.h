#pragma once
#include <expected>
#include <filesystem>
#include <fmt/format.h>
#include <glaze/glaze.hpp>
#include <string>
#include <tuple>

// clang-format off
struct WindowPosition {
  int x = 0;
  int y = 0;
  int width = 0;
  int height = 0;

  struct glz {
    using T = WindowPosition;
    static constexpr auto value = ::glz::object(
      "x", &T::x,
      "y", &T::y,
      "width", &T::width,
      "height", &T::height
    );
  };
};

struct AppData {
  std::string version = fmt::format("{}",FLYMATE_VERSION);
  WindowPosition main_window;
  struct glz {
    using T = AppData;
    static constexpr auto value = ::glz::object(
      "version", &T::version,
  "window", &T::main_window
    );
  };
};
// clang-format on

namespace flymate {

class Settings {
public:
  void init();

  std::expected<void, std::string> load();
  std::expected<void, std::string> save() const;
  std::expected<void, std::string> save(const AppData& appData);

  AppData getAppData() const;
  std::filesystem::path getUserSettingsPath() const;
  bool isInitialized() const;
  std::filesystem::path getPluginPath() const;
  std::tuple<int, int> getXplaneVersion() const;
  int getXPLMVersion() const;

private:

  std::filesystem::path userSettingsPath_;
  std::filesystem::path pluginPath_;
  AppData appData_;
  bool isInitialized_ = false;
  int xpMajorVersion_ = 0;
  int xpMinorVersion_ = 0;
  int xplmVersion_ = 0;
};

inline Settings settings;

} // namespace flymate
