# Banjo-Kazooie VR for Steam Frame

This is my Steam Frame fork of [Banjo: Recompiled](https://github.com/BanjoRecomp/BanjoRecomp), which rebuilds the original Nintendo 64 game as a native application. The fork adds an optional OpenXR stereo mode for Steam Frame, using Vulkan through RT64 and Plume. Flat mode remains available.

**EXPERIMENTAL.** This is a personal-use project and is still a work in progress. You need the supported US 1.0 Banjo-Kazooie ROM from your own copy of the game. The repository contains source and build tools, not the ROM or generated game data.

**Want to play on Steam Frame?** Download the prebuilt standalone package from the [v0.1.0 prerelease](https://github.com/astromaddie/banjo-vr/releases/tag/v0.1.0); no compiler is needed. You still need the supported US 1.0 Banjo-Kazooie ROM from your own copy.

## What the Frame fork adds

- A tracked stereo world, with the game HUD and menus kept on a readable panel.
- 3D cinema presentation for the opening movie and other cutscenes, plus flat and immersive cutscene choices.
- Snap or smooth comfort turning, adjustable world size and camera position, camera smoothing, and foveated rendering.
- A VR settings menu, recentering, pause handling, and Steam Frame controller bindings. The default right stick turns in third person; hold Menu + View to open VR settings and hold View to recenter.
- A native Linux ARM64 build path for Steam Frame. The existing flat game remains the default unless VR is enabled.

The renderer and game-side changes are described in [VR architecture](docs/VR_ARCHITECTURE.md) and [Banjo's game-specific VR integration](docs/VR_GAME_SIDE.md). Detailed implementation notes and test history stay in those documents.

## Build and install

The [Steam Frame build and install guide](docs/STEAM-FRAME-BUILD-INSTALL.md) covers ARM64 Docker builds from Linux, macOS, or Windows through WSL2, the supported ROM, package output, first installation and Steam launch. It distinguishes the standalone Linux ARM64 Frame package from Windows or desktop builds. The upstream [BUILDING.md](BUILDING.md) is a desktop recompiler guide, not the Steam Frame packaging route. The ROM and generated game data stay outside Git and CI artifacts.

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
