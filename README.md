# Banjo: Recompiled — Steam Frame VR fork

**Status: EXPERIMENTAL. No Steam Frame build or release has been published.** Upstream is [BanjoRecomp/BanjoRecomp](https://github.com/BanjoRecomp/BanjoRecomp). This fork adds an optional native OpenXR stereo path targeting standalone Steam Frame on Linux ARM64 with SteamVR/OpenXR and Vulkan through RT64/Plume; flat mode remains the default.

| Evidence | Verified status |
| --- | --- |
| Host/source | ROM-free production VR bridge, Banjo profile/comfort-turn imports and RT64 eye-slot tests compiled on macOS ARM64; CTest 4/4 passed on 2026-10-11 from base `b426f05f3722a4e947a5c224601e906eff158c65` plus the PR0 changes. The public [ROM-free workflow](.github/workflows/public-vr-validation.yml) is configured without secrets; no remote GitHub run has been observed. |
| Unattended Frame | Prior off-head runs verified OpenXR lifecycle, scene routing, stereo views, pause/menu paths and diagnostics at 2160×2160. Device profiles were accepted; real button input reaching the game was not confirmed. |
| Worn | No Banjo build has been worn; focused wearer acceptance remains pending. |

**Known failures and limits:** physical button input and wearer comfort are unverified; 120 Hz and current-source worn performance are unmeasured. No complete application or package build is claimed. Tested source is the base above plus the PR0 change set; the coordinator will record the integrated source commit.

**External game data:** a personally owned North American 1.0 Banjo-Kazooie ROM is required for a complete build and gameplay. ROM and generated game data stay outside Git and CI artifacts.

Run the public, ROM-free production-source checks from a clean checkout:

```sh
bash tools/vr/prepare-public-dependencies.sh
cmake -S src/vr/tests -B build-vrtest -G Ninja
cmake --build build-vrtest --parallel 2
ctest --test-dir build-vrtest --output-on-failure
```

A complete local build requires your owned ROM and its existing hash validation:

```sh
BK_ROM="/path/to/owned-us-rom.z64" BK_VR=1 JOBS=4 tools/docker/build-linux.sh all
```

This fork used OpenAI Codex to assist with VR source, tests and readiness documentation; disclosed AI assistance is allowed here subject to review. The upstream AI restriction applies to submissions to the official project. See [the scoped contribution policy](CONTRIBUTING.md) and [the original upstream policy retained verbatim](CONTRIBUTING_UPSTREAM.md).

See [PR0 readiness and evidence](docs/PR0_READINESS.md), [VR architecture and wearer checklist](docs/VR_ARCHITECTURE.md), and the [README retained for history](docs/README_PRE_PR0.md). Upstream licenses and notices remain in their original files.
