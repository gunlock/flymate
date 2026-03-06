#include "logging.h"
#include <XPLMUtilities.h>
#include <filesystem>
#include <spdlog/sinks/base_sink.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/spdlog.h>

namespace flymate::logging {

std::expected<void, std::string> init(const std::string& pluginPath) {
  try {
    // Tear down previous logger if reinitializing (e.g. plugin reload with -z nodelete)
    if (spdlog::get("flymate")) {
      spdlog::drop("flymate");
    }

    auto logFile = (std::filesystem::path(pluginPath) / "flymate.log").string();
    constexpr size_t maxSize = 1024 * 1024 * 2; // 2 MB
    constexpr size_t maxFiles = 3;
    auto file_sink =
        std::make_shared<spdlog::sinks::rotating_file_sink_mt>(logFile, maxSize, maxFiles);
    file_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] %v");

    auto logger = std::make_shared<spdlog::logger>("flymate", spdlog::sinks_init_list{file_sink});

    logger->set_level(spdlog::level::debug);
    logger->flush_on(spdlog::level::warn);

    spdlog::set_default_logger(logger);

    spdlog::info("Logging initialized (file: {})", logFile);
    return {};
  } catch (const spdlog::spdlog_ex& ex) {
    return std::unexpected(std::string("spdlog init failed: ") + ex.what());
  }
}

void shutdown() {
  spdlog::info("Logging shutdown");
  spdlog::default_logger()->flush();
  spdlog::drop_all();
}

} // namespace flymate::logging
