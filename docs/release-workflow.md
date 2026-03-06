# GitHub Release Workflow

## How Releases Work

### Trigger
Push a git tag matching `v*`:
```bash
git tag v0.1.0
git push origin v0.1.0
```

### What Happens (`.github/workflows/release.yml`)
1. GitHub Actions detects the tag push
2. Builds the Docker image: `docker build -t flymate-build .`
3. Runs cross-compilation inside Docker: `scripts/build.sh --deploy all`
4. Uploads the resulting zip to a **GitHub Release** via `softprops/action-gh-release`

### What `build.sh --deploy all` Produces
It cross-compiles for 4 targets (linux, windows, mac-x64, mac-arm64), creates a macOS universal binary, then packages everything into:

```
build/deploy/FlyMate.zip
└── FlyMate/
    ├── lin_x64/FlyMate.xpl
    ├── win_x64/FlyMate.xpl
    └── mac_x64/FlyMate.xpl  (universal: x86_64 + arm64)
```

### Where Release Files Live
- **Locally**: `build/deploy/FlyMate.zip` (after running `cmake --build --preset deploy`)
- **On GitHub**: Attached as a download on the Releases page (e.g., `github.com/you/repo/releases/tag/v0.1.0`)
- Files are **not** stored in the repo itself — they're build artifacts uploaded to GitHub Releases

### Local Workflow vs GitHub
| Step | Local (`cmake --build --preset deploy`) | GitHub (push tag) |
|------|---|---|
| Docker build | Same | Same |
| Cross-compile | Same (`build.sh --deploy all`) | Same (`build.sh --deploy all`) |
| Output zip | `build/deploy/FlyMate.zip` on your disk | Uploaded to GitHub Releases page |
| Manual step | You distribute the zip yourself | Users download from GitHub Releases |

### End-to-End Release Process
```
1. Develop & test locally
2. git tag v0.1.0 && git push origin v0.1.0
3. GitHub Actions automatically:
   a. Builds Docker image
   b. Cross-compiles all platforms
   c. Creates FlyMate.zip
   d. Publishes GitHub Release with zip attached
4. Users download zip from GitHub Releases
5. Extract to X-Plane 12/Resources/plugins/
```

## Build Presets

All presets assume a Linux development host and use Docker for compilation.

| Preset | What it does |
|--------|-------------|
| `build:linux` | Compile linux .xpl |
| `build:windows` | Compile windows .xpl |
| `build:macos` | Compile macOS .xpl (x64 + arm64) |
| `build:all` | Compile all platforms (parallel, shared deps) |
| `deploy` | Build all platforms + create universal macOS binary + zip |
| `install` | Build linux + copy to `XPLANE_PLUGIN_DIR` |
| `docker:rebuild` | Rebuild Docker build image (no cache) |
| `docker:clean` | Remove Docker build image |

Usage:
```bash
cmake --preset dev                    # configure (once)
cmake --build --preset build:linux    # build linux only
cmake --build --preset deploy         # build all + create zip
cmake --build --preset install        # build linux + install to X-Plane
```

### Key Files
| File | Role |
|------|------|
| `.github/workflows/release.yml` | GitHub Actions workflow triggered by `v*` tags |
| `scripts/build.sh` | Cross-compiles platforms, optionally creates zip |
| `Dockerfile` | Build environment (Zig + CMake + LLVM) |
| `CMakePresets.json` | Defines the 4 cross-compilation presets (used inside Docker) |
| `CMakeLists.txt` | Custom targets orchestrating Docker builds |
