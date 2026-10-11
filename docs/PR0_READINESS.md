# PR0 — banjo-vr

**EXPERIMENTAL — ready for public source technical preview.** Based on [BanjoRecomp/BanjoRecomp](https://github.com/BanjoRecomp/BanjoRecomp), targeting native Linux ARM64 Steam Frame, SteamVR/OpenXR and Vulkan. No binary release was uploaded.

At `2e3c67cd23502bb06943a8ddd8dc199faa7645a5`, the ROM-free production VR CMake/CTest suite passed 4/4 from a clean checkout with an empty home and no personal Git credentials. Public RT64/Plume bases and the hash-pinned VR overlays resolved correctly. The complete game build was not run here: it needs the supported user-owned ROM. This publication is a source technical preview, with the current capabilities described in the README.

The automatic [public validation workflow](../.github/workflows/public-vr-validation.yml) uses no ROMs or repository secrets. The latest private-repo job ran no steps because GitHub blocked it on account billing; this is recorded separately from the successful local checks. Earlier successful CI runs remain in [PR0_METADATA.json](PR0_METADATA.json).

The history audit found no ROMs, extracted game-data payloads or personal credentials. Project branding and gameplay captures remain separate from the source licenses. Upstream GPLv3 host notices in `COPYING` and renderer/component notices are preserved; they do not give permission to distribute Nintendo's game or data. The upstream contribution policy is retained in CONTRIBUTING_UPSTREAM.md and applies to their project, not the development practices of this fork.

For the ROM-free checks, run:

```sh
bash tools/vr/prepare-public-dependencies.sh
cmake -S src/vr/tests -B build-vrtest -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build-vrtest --parallel 2
ctest --test-dir build-vrtest --output-on-failure
```

The full build takes your own game input: `BK_ROM=/path/to/owned-rom.z64 BK_VR=1 JOBS=4 tools/docker/build-linux.sh all`. The README and architecture documentation list the supported revision and current features. Inherited workflows that need private data remain disabled unless `RUN_LEGACY_VALIDATION=true` and their required secrets are configured.
