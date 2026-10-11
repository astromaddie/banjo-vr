# Banjo-Kazooie VR for Steam Frame

This is my Steam Frame fork of [Banjo: Recompiled](https://github.com/BanjoRecomp/BanjoRecomp), which rebuilds the original Nintendo 64 game as a native application. The fork adds an optional OpenXR stereo mode for Steam Frame, using Vulkan through RT64 and Plume. Flat mode remains available.

**EXPERIMENTAL.** This is a personal-use project and is still a work in progress. You need the supported US 1.0 Banjo-Kazooie ROM from your own copy of the game. The repository contains source and build tools, not the ROM or generated game data.

## What the Frame fork adds

- A tracked stereo world, with the game HUD and menus kept on a readable panel.
- 3D cinema presentation for the opening movie and other cutscenes, plus flat and immersive cutscene choices.
- Snap or smooth comfort turning, adjustable world size and camera position, camera smoothing, and foveated rendering.
- A VR settings menu, recentering, pause handling, and Steam Frame controller bindings. The default right stick turns in third person; hold Menu + View to open VR settings and hold View to recenter.
- A native Linux ARM64 build path for Steam Frame. The existing flat game remains the default unless VR is enabled.

The renderer and game-side changes are described in [VR architecture](docs/VR_ARCHITECTURE.md) and [Banjo's game-specific VR integration](docs/VR_GAME_SIDE.md). Detailed implementation notes and test history stay in those documents.

## Build

Use Docker on ARM64 Linux or an ARM64 Docker environment on Apple Silicon. The build script creates its pinned compiler image and packages the game.

For a complete Linux ARM64 build, provide your own supported ROM and run:

```sh
git clone --recurse-submodules --branch steam-frame https://github.com/astromaddie/banjo-vr.git
cd banjo-vr
export BK_ROM="/path/to/owned-us-rom.z64"
BK_VR=1 JOBS=4 tools/docker/build-linux.sh all
```

The build checks the ROM against the expected image before generating game code. See [BUILDING.md](BUILDING.md) for the upstream build prerequisites and steps. The ROM and generated game data stay outside Git and CI artifacts.

To copy the package to your Frame, register it in the Steam library, and copy your own ROM, set the Frame's SSH address and run:

```sh
FRAME_HOST="steamos@YOUR_FRAME_ADDRESS" tools/steam_frame/deploy.sh \
  --package build-linux-aarch64-vr/package/BanjoRecompiled --rom "$BK_ROM"
```

Launch BanjoVR from the Frame's Steam library so it receives controller input.

## ROM-free VR checks

The [VR validation workflow](.github/workflows/public-vr-validation.yml) runs without a ROM or repository secrets. You can run the same production-source checks locally after cloning the dependencies above, with CMake, Ninja and a C++ compiler:

```sh
bash tools/vr/prepare-public-dependencies.sh
cmake -S src/vr/tests -B build-vrtest -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build-vrtest
ctest --test-dir build-vrtest --output-on-failure
```

## Upstream, contributions, and licenses

This fork has used OpenAI Codex to assist with VR source, tests and documentation. Disclosed AI assistance is allowed here, subject to human review. Banjo: Recompiled's upstream AI restriction applies to submissions to that official project; it does not set this fork's policy. See [this fork's contribution policy](CONTRIBUTING.md) and the [original upstream policy](CONTRIBUTING_UPSTREAM.md).

This fork retains Banjo: Recompiled's upstream notices and license.

The earlier [PR0 readiness audit](docs/PR0_READINESS.md), [source and CI metadata](docs/PR0_METADATA.json), and [README history](docs/README_PRE_PR0.md) remain available as historical records. The host application retains its upstream [GPLv3 notice](COPYING); RT64 and Plume retain their own MIT notices. Code licenses do not grant permission to redistribute the commercial game, ROMs, or extracted media. Bring your own game data.
