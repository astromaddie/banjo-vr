# PR0 release-readiness record — Banjo VR

**State: EXPERIMENTAL; no public Steam Frame build or release.** Source, device and wearer evidence are distinguished below.

The immutable test source commit and final CI result are recorded in [PR0_METADATA.json](PR0_METADATA.json). Full application builds and focused wearer acceptance remain separate open gates.

## Identity and target

- Upstream: [BanjoRecomp/BanjoRecomp](https://github.com/BanjoRecomp/BanjoRecomp).
- Target: standalone Steam Frame, Linux ARM64, SteamVR/OpenXR, Vulkan via SDL2, RT64 and Plume.
- Source baseline before PR0 edits: `b426f05f3722a4e947a5c224601e906eff158c65`.
- Tested source: `89a22f1545e5f445d6c94e8ddf53dafae262c012`; this immutable source also passed the ROM-free GitHub run.
- Intended runtime target: Steam Frame standalone, Linux ARM64, SteamVR/OpenXR and Vulkan; experimental, with no general platform-support claim.
- Host validation: macOS ARM64; the public workflow is configured for Ubuntu 22.04 and its ROM-free GitHub run passed; see PR0_METADATA.json.
- RT64 public base: `43373749dac9bbc1b653e6a02aed40a9e1783bed`; VR overlay SHA-256: `e1aa7dd33453ec9e61a4e023fa81d455bf7e4b106911423da10b48e1b4db5eaa`.
- Plume public base: `d890ac899e505fb30040e037a4037cdeca68f033`; VR overlay SHA-256: `99fbdb9731416e35cbc5854bf977cf8fc3679655246462f6e6fed145d18294db`.

The bootstrap checks exact base commits and patch hashes, then applies tracked VR source overlays to clean public checkouts. It keeps RT64 and Plume license/notice files intact and removes the private submodule URLs.

## Evidence

- **Host/source:** ROM-free production VR bridge, Banjo profile/comfort-turn imports and RT64 eye-slot CMake/CTest: 4/4 passed on 2026-10-11 in a fresh macOS ARM64 checkout with an empty HOME and no Git credentials.
  ```sh
  git submodule update --init --depth=1 -- lib/rt64 lib/N64ModernRuntime lib/bk-decomp
  bash tools/vr/prepare-public-dependencies.sh
  cmake -S src/vr/tests -B build-vrtest -G Ninja
  cmake --build build-vrtest --parallel 2
  ctest --test-dir build-vrtest --output-on-failure
  ```
- **Public CI:** Configured for GitHub push/PR with no repository secrets or ROM; its exact-source Ubuntu 22.04 GitHub run passed; see PR0_METADATA.json. The [workflow](../.github/workflows/public-vr-validation.yml) fetches public sources and runs CMake/CTest.
- **Unattended Frame:** Prior off-head runs verified OpenXR lifecycle, scene routing, stereo views, pause/menu paths and diagnostics at 2160×2160; device profiles were accepted, but real button input reaching the game was never confirmed.
- **Worn:** No Banjo build has been worn; focused wearer acceptance remains pending.
- **Performance:** previous off-head 2160×2160 measurements exist at 72 Hz. 120 Hz and current-source wearer performance are unmeasured; see [VR architecture](VR_ARCHITECTURE.md).

## Clean-checkout boundary

The clean host run covered the PR0 source changes. The immutable integrated source commit listed above then passed the GitHub host job and anonymous recursive dependency checkout. Each checkout used a separate empty HOME, disabled system/global Git configuration, no SSH agent or GitHub token, and fetched only public sources.

The ROM-free test path initialized public test dependencies, initialized the public N64Recomp header source required by the production host-import test, verified RT64/Plume bases and overlay hashes, compiled production VR sources, and passed CTest 4/4. This is source/test evidence on macOS ARM64, not a full runtime support claim. The GitHub workflow is configured for Ubuntu 22.04 and its exact-source ROM-free GitHub run passed.

The tests do not build the complete game executable or native package.

A complete build and playable run require the locally held North American 1.0 Banjo-Kazooie ROM and existing hash validation. No ROM was copied here or used by CI; no complete application build is claimed.

## External data and full build

The original ROM and generated recompiler/game data are local external inputs, excluded from source and CI artifacts. Use only data you own and keep it outside the public tree.

```sh
BK_ROM="/path/to/owned-us-rom.z64" BK_VR=1 JOBS=4 tools/docker/build-linux.sh all
```

See [VR architecture](VR_ARCHITECTURE.md) for implementation, prior device evidence, limitations and the wearer checklist.

## Legacy upstream CI

The inherited full-build workflows fetch private data and are disabled by default in this fork. To opt in deliberately, set the repository Actions variable `RUN_LEGACY_VALIDATION` to the exact string `true` and configure the required repository secrets: `SECRET_NAME` (private extra-data repository) and `SECRET_TOKEN`. The automatic `public-vr-validation` workflow is independent and requires no secrets or ROM.

At the immutable tested source, all 5 direct pins fetched anonymously and all 45 recursive submodules checked out with zero missing, mismatched or conflicted entries. This verifies dependency availability, not a complete application build or rights clearance.
