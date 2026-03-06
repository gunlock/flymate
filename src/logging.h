#pragma once
#include <expected>
#include <string>

namespace flymate::logging {

// Initialize spdlog with a rotating file sink in the plugin directory
// Must be called after AppState::init() so the plugin path is known.
std::expected<void, std::string> init(const std::string& pluginPath);

// Flush and drop all spdlog loggers. Call from XPluginStop.
void shutdown();

} // namespace flymate::logging
