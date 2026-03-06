# FlyMate

An ImGui overlay plugin for X-Plane 11/12.

## Build

### Setup

```bash
cp CMakeUserPresets.json.example CMakeUserPresets.json
# Edit CMakeUserPresets.json: set XPLANE_PLUGIN_DIR to your X-Plane plugins path

cmake --preset dev
```

`CMakeUserPresets.json` is gitignored and won't affect other developers.

### Local Development

Build presets for the iterative dev loop (Linux development host assumed).

All builds use Docker with Zig for cross-compilation, so Docker must be
installed and running.

```bash
cmake --build --preset build:linux  # compile the Linux .xpl only
cmake --build --preset build:all    # compile all platforms
cmake --build --preset install      # build Linux + copy to X-Plane plugins dir
```

### Release

Cross-compile all platforms via Docker (Zig toolchain) and produce a release zip:

```bash
cmake --build --preset deploy           # cross-compile all platforms + zip
```

The deploy preset runs `scripts/build.sh --deploy all` inside the Docker
container, producing:

```
build/deploy/
├── FlyMate/
│   ├── lin_x64/FlyMate.xpl
│   ├── win_x64/FlyMate.xpl
│   └── mac_x64/FlyMate.xpl   ← universal binary (x86_64 + arm64)
└── FlyMate.zip
```

Copy `FlyMate/` into `X-Plane/Resources/plugins/` to install.

Individual cross-compile configure/build presets (used internally by the Docker build):

| Preset      | Platform       |
| ----------- | -------------- |
| `linux`     | Linux x86_64   |
| `windows`   | Windows x86_64 |
| `mac-x64`   | macOS x86_64   |
| `mac-arm64` | macOS ARM64    |

## Docker Image Management

```bash
cmake --build --preset docker:rebuild   # rebuild the Docker image from scratch
cmake --build --preset docker:clean     # remove the Docker image
```

## License

[MIT-0](LICENSE)
