#!/usr/bin/env bash
#
# Build FlyMate for one or more platforms.
#
# Usage (inside Docker):
#   scripts/build.sh <linux|windows|macos|all>
#   scripts/build.sh --deploy all
#
# --deploy: after building, assemble the plugin directory and create FlyMate.zip
#
# Output (build only):
#   build/<platform>/FlyMate/<xp_dir>/FlyMate.xpl
#
# Output (--deploy):
#   build/deploy/FlyMate.zip
#   └── FlyMate/
#       ├── lin_x64/FlyMate.xpl
#       ├── win_x64/FlyMate.xpl
#       └── mac_x64/FlyMate.xpl   (universal binary: x86_64 + arm64)

set -euo pipefail

DEPLOY=false
PLATFORM=""

while [[ $# -gt 0 ]]; do
    case $1 in
        --deploy) DEPLOY=true; shift ;;
        *) PLATFORM="$1"; shift ;;
    esac
done

if [[ -z "$PLATFORM" ]]; then
    echo "Usage: build.sh [--deploy] <linux|windows|macos|all>"
    exit 1
fi

DEPLOY_DIR=build/deploy

# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

build_preset() {
    local preset=$1; shift
    local fc_args
    mapfile -t fc_args < <(find_fc_args)
    if [[ ${#fc_args[@]} -gt 0 ]]; then
        echo "  Reusing cached sources: ${fc_args[*]}"
    fi
    echo "=== Configuring ${preset} ==="
    cmake --preset "$preset" "${fc_args[@]}" "$@"
    echo "=== Building ${preset} ==="
    cmake --build --preset "$preset"
    echo "  ${preset}: OK"
}

# Scan all existing build/*/_deps/*-src directories and return
# FETCHCONTENT_SOURCE_DIR args so that cmake reuses already-downloaded sources
# from any prior build (same session or previous invocation).
find_fc_args() {
    local found=()
    for src_dir in build/*/_deps/*-src; do
        [[ -d "$src_dir" ]] || continue
        local dep_name
        dep_name=$(basename "$src_dir" | sed 's/-src$//' | tr '[:lower:]' '[:upper:]')
        # Only emit each dependency once (first match wins).
        if [[ ! " ${found[*]+"${found[*]}"} " =~ " ${dep_name} " ]]; then
            found+=("$dep_name")
            echo "-DFETCHCONTENT_SOURCE_DIR_${dep_name}=$(realpath "$src_dir")"
        fi
    done
}

# ---------------------------------------------------------------------------
# Platform builders
# ---------------------------------------------------------------------------

build_linux() {
    build_preset linux "$@"
}

build_windows() {
    build_preset windows "$@"
}

build_macos() {
    build_preset mac-x64 "$@"
    build_preset mac-arm64 "$@"
}

build_all() {
    local LOGDIR="/tmp/build-logs"
    mkdir -p "$LOGDIR"

    # Phase 1: Build linux first to populate the FetchContent source cache.
    echo "=== Phase 1: linux (populates shared source cache) ==="
    build_linux

    # Phase 2: Build remaining platforms in parallel.
    # Each build_preset call auto-discovers cached sources via find_fc_args.
    echo "=== Phase 2: building windows, macos in parallel ==="

    PIDS=()

    (build_windows) > "$LOGDIR/windows.log" 2>&1 &
    tail -f "$LOGDIR/windows.log" --pid=$! | sed "s/^/[windows] /" &
    PIDS+=($!)
    echo "  Started windows (PID $!)"

    (build_macos) > "$LOGDIR/macos.log" 2>&1 &
    tail -f "$LOGDIR/macos.log" --pid=$! | sed "s/^/[macos] /" &
    PIDS+=($!)
    echo "  Started macos (PID $!)"

    # Wait for all parallel builds and collect failures.
    FAILED=()
    NAMES=(windows macos)
    for i in "${!NAMES[@]}"; do
        if wait "${PIDS[$i]}"; then
            echo "  ${NAMES[$i]}: OK"
        else
            echo "  ${NAMES[$i]}: FAILED (see $LOGDIR/${NAMES[$i]}.log)"
            FAILED+=("${NAMES[$i]}")
        fi
    done

    if [[ ${#FAILED[@]} -gt 0 ]]; then
        for name in "${FAILED[@]}"; do
            echo "=== LOG: ${name} ==="
            cat "$LOGDIR/${name}.log"
        done
        echo "FATAL: ${#FAILED[@]} build(s) failed: ${FAILED[*]}"
        exit 1
    fi
}

# ---------------------------------------------------------------------------
# Deploy: assemble plugin directory + zip
# ---------------------------------------------------------------------------

deploy() {
    echo "=== Assembling final plugin directory ==="
    rm -rf "$DEPLOY_DIR"
    mkdir -p "$DEPLOY_DIR/FlyMate/lin_x64" \
             "$DEPLOY_DIR/FlyMate/win_x64" \
             "$DEPLOY_DIR/FlyMate/mac_x64"

    cp build/linux/FlyMate/lin_x64/FlyMate.xpl     "$DEPLOY_DIR/FlyMate/lin_x64/"
    cp build/windows/FlyMate/win_x64/FlyMate.xpl   "$DEPLOY_DIR/FlyMate/win_x64/"

    echo "=== Creating macOS universal binary ==="
    llvm-lipo-18 -create \
        build/mac-x64/FlyMate/mac_x64/FlyMate.xpl \
        build/mac-arm64/FlyMate/mac_x64/FlyMate.xpl \
        -output "$DEPLOY_DIR/FlyMate/mac_x64/FlyMate.xpl"

    echo "=== Stripping debug symbols ==="
    llvm-strip-18 --strip-all "$DEPLOY_DIR/FlyMate/lin_x64/FlyMate.xpl"
    llvm-strip-18 --strip-all "$DEPLOY_DIR/FlyMate/mac_x64/FlyMate.xpl"

    echo "=== Creating FlyMate.zip ==="
    (cd "$DEPLOY_DIR" && zip -r FlyMate.zip FlyMate/)

    echo "=== Done ==="
    echo "Plugin directory:"
    find "$DEPLOY_DIR/FlyMate" -type f | sort
    file "$DEPLOY_DIR"/FlyMate/*/FlyMate.xpl 2>/dev/null || true
    echo "Archive: $DEPLOY_DIR/FlyMate.zip ($(du -h "$DEPLOY_DIR/FlyMate.zip" | cut -f1))"
}

# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

case $PLATFORM in
    linux)   build_linux ;;
    windows) build_windows ;;
    macos)   build_macos ;;
    all)     build_all ;;
    *) echo "Unknown platform: $PLATFORM"; exit 1 ;;
esac

if $DEPLOY; then
    deploy
fi
