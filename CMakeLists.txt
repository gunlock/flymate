cmake_minimum_required(VERSION 3.25)

set(FLYMATE_VERSION "0.1.0")
project(FlyMate VERSION ${FLYMATE_VERSION} LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Export compile_commands.json for editor tooling.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(FetchContent)

# ---------------------------------------------------------------------------
# X-Plane SDK 4.3.0
# ---------------------------------------------------------------------------
FetchContent_Declare(
    xpsdk
    URL      https://developer.x-plane.com/wp-content/plugins/code-sample-generation/sdk_zip_files/XPSDK430.zip
    URL_HASH SHA256=b9875ab27b593927b4f9b3e0ddfffe7401ee5dce6d86b50aea0da65f70ff7816
)
FetchContent_MakeAvailable(xpsdk)

set(XPSDK_ROOT "${xpsdk_SOURCE_DIR}")

# ---------------------------------------------------------------------------
# Dear ImGui v1.92.6
# ---------------------------------------------------------------------------
FetchContent_Declare(
    imgui
    URL      https://github.com/ocornut/imgui/archive/refs/tags/v1.92.6.tar.gz
    URL_HASH SHA256=5b17c01f69545bde732b14936d89ce0f508adb83e8b56fa82448371845172bc3
)
FetchContent_MakeAvailable(imgui)

set(IMGUI_DIR "${imgui_SOURCE_DIR}")

# ---------------------------------------------------------------------------
# fmtlib 12.1.0
# ---------------------------------------------------------------------------
FetchContent_Declare(
    fmt
    URL      https://github.com/fmtlib/fmt/archive/refs/tags/12.1.0.tar.gz
    URL_HASH SHA256=ea7de4299689e12b6dddd392f9896f08fb0777ac7168897a244a6d6085043fea
)
FetchContent_MakeAvailable(fmt)


# ---------------------------------------------------------------------------
# glaze 6.2.0 
# ---------------------------------------------------------------------------
FetchContent_Declare(
    glaze
    URL      https://github.com/stephenberry/glaze/archive/refs/tags/v6.2.0.tar.gz
    URL_HASH SHA256=0826d9e50f22a1d8b8e28a29aad8af2781f81bf85efa14d34b5709792e50b2bf
)
set(glaze_ENABLE_AVX2 OFF CACHE BOOL "AVX2 disabled" FORCE) 
FetchContent_MakeAvailable(glaze)

# ---------------------------------------------------------------------------
# spdlog 1.17.0 
# ---------------------------------------------------------------------------
include(FetchContent)                                                                                                
   
FetchContent_Declare(                                                                                                
  spdlog                                                                                                           
  URL      https://github.com/gabime/spdlog/archive/refs/tags/v1.17.0.tar.gz
  URL_HASH SHA256=d8862955c6d74e5846b3f580b1605d2428b11d97a410d86e2fb13e857cd3a744
)
set(SPDLOG_BUILD_SHARED OFF CACHE BOOL "" FORCE)
set(SPDLOG_BUILD_EXAMPLE OFF CACHE BOOL "" FORCE)
set(SPDLOG_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(SPDLOG_FMT_EXTERNAL ON CACHE BOOL "" FORCE)
set(SPDLOG_INSTALL OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(spdlog)


# ---------------------------------------------------------------------------
# Plugin target
# ---------------------------------------------------------------------------
add_library(FlyMate SHARED
    src/main.cpp
    src/settings.h
    src/settings.cpp
    src/logging.h
    src/logging.cpp
)

target_include_directories(FlyMate PRIVATE
    src
    ${XPSDK_ROOT}/CHeaders/XPLM
    ${XPSDK_ROOT}/CHeaders/Widgets
)

# X-Plane SDK version defines (XP11+ compatibility).
target_compile_definitions(FlyMate PRIVATE
    XPLM303
    FLYMATE_VERSION="${FLYMATE_VERSION}"
)

target_link_libraries(FlyMate PRIVATE 
    fmt::fmt 
    glaze::glaze
    spdlog::spdlog
)

# ---------------------------------------------------------------------------
# Platform-specific configuration
# ---------------------------------------------------------------------------
if(WIN32)
  target_compile_definitions(FlyMate PRIVATE IBM=1 WINDOWS=1)
  target_link_libraries(FlyMate PRIVATE
        ${XPSDK_ROOT}/Libraries/Win/XPLM_64.lib
        ${XPSDK_ROOT}/Libraries/Win/XPWidgets_64.lib
        opengl32
    )
  set(PLUGIN_PLATFORM_DIR "win_x64")
elseif(APPLE)
  target_compile_definitions(FlyMate PRIVATE APL=1 MACOS=1)
  target_link_libraries(FlyMate PRIVATE
        "-F${XPSDK_ROOT}/Libraries/Mac"
        "-framework XPLM"
        "-framework XPWidgets"
    )
  set(PLUGIN_PLATFORM_DIR "mac_x64")
else()
    # Linux: XPLM/XPWidgets symbols are resolved at runtime when X-Plane
    # dlopen()s the plugin, so no link-time stubs are needed (unlike Windows
    # .lib or macOS frameworks).
  target_compile_definitions(FlyMate PRIVATE LIN=1 LINUX=1)
  set(PLUGIN_PLATFORM_DIR "lin_x64")
endif()

# Cross-compiling with zig c++ statically links its own libc++ into the
# plugin. Hidden visibility prevents those symbols from clashing with the
# host's C++ runtime (libstdc++ on Linux, libc++ on macOS).
# PLUGIN_API already marks the 5 XPLM entry points as visibility("default").
target_compile_options(FlyMate PRIVATE -fvisibility=hidden -fvisibility-inlines-hidden)

# -z nodelete prevents dlclose()/FreeLibrary() from unmapping the plugin.
# Without this, static destructors registered by C++ template libraries
# (glaze, fmt) point to unmapped memory after unload → crash on shutdown.
target_link_options(FlyMate PRIVATE -Wl,-z,nodelete)

set_target_properties(FlyMate PROPERTIES
    PREFIX ""
    SUFFIX ".xpl"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/FlyMate/${PLUGIN_PLATFORM_DIR}"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/FlyMate/${PLUGIN_PLATFORM_DIR}"
)

# ---------------------------------------------------------------------------
# Docker-based build workflow
#
# All builds use Docker with zig c++ for cross-compilation.
# Assumes a Linux development host for the install target.
# ---------------------------------------------------------------------------
set(DOCKER_IMAGE "flymate-build")
set(XPLANE_PLUGIN_DIR "" CACHE PATH "Path to X-Plane plugins directory (e.g. .../X-Plane 12/Resources/plugins)")

# Helper macro: build docker image (cached) then run a command in the container.
macro(flymate_docker_target target_name comment)
    add_custom_target(${target_name}
        COMMAND docker build -q -t ${DOCKER_IMAGE} "${CMAKE_SOURCE_DIR}"
        COMMAND docker run --rm -v "${CMAKE_SOURCE_DIR}:/src" ${DOCKER_IMAGE} ${ARGN}
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        COMMENT "${comment}"
    )
endmacro()

flymate_docker_target(build-linux   "Building FlyMate for Linux x64"              scripts/build.sh linux)
flymate_docker_target(build-windows "Building FlyMate for Windows x64"            scripts/build.sh windows)
flymate_docker_target(build-macos   "Building FlyMate for macOS (x64 + arm64)"    scripts/build.sh macos)
flymate_docker_target(build-all     "Building FlyMate for all platforms"           scripts/build.sh all)
flymate_docker_target(deploy        "Building and deploying FlyMate (all platforms)" scripts/build.sh --deploy all)

# install-plugin: build for linux and copy to X-Plane plugins directory.
# Assumes a Linux development host.
if(XPLANE_PLUGIN_DIR)
    add_custom_target(install-plugin
        COMMAND docker build -q -t ${DOCKER_IMAGE} "${CMAKE_SOURCE_DIR}"
        COMMAND docker run --rm -v "${CMAKE_SOURCE_DIR}:/src" ${DOCKER_IMAGE} scripts/build.sh linux
        COMMAND ${CMAKE_COMMAND} -E remove_directory "${XPLANE_PLUGIN_DIR}/FlyMate"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${XPLANE_PLUGIN_DIR}/FlyMate/lin_x64"
        COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_SOURCE_DIR}/build/linux/FlyMate/lin_x64/FlyMate.xpl" "${XPLANE_PLUGIN_DIR}/FlyMate/lin_x64/"
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        COMMENT "Building and installing FlyMate to ${XPLANE_PLUGIN_DIR}"
    )
else()
    add_custom_target(install-plugin
        COMMAND ${CMAKE_COMMAND} -E echo "ERROR: XPLANE_PLUGIN_DIR is not set. Set it in CMakeUserPresets.json or pass -DXPLANE_PLUGIN_DIR=..."
        COMMAND ${CMAKE_COMMAND} -E false
    )
endif()

# Docker image management
add_custom_target(docker-rebuild
    COMMAND docker build --no-cache -t ${DOCKER_IMAGE} "${CMAKE_SOURCE_DIR}"
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    COMMENT "Rebuilding Docker image ${DOCKER_IMAGE} (no cache)"
)

add_custom_target(docker-clean-image
    COMMAND docker rmi ${DOCKER_IMAGE}
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    COMMENT "Removing Docker image ${DOCKER_IMAGE}"
)
