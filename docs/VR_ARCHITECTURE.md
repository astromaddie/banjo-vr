# Banjo VR (Steam Frame) architecture

Current state of the VR layer in this fork of Banjo: Recompiled (N64Recomp + RT64 +
N64ModernRuntime). All VR code is behind the CMake option `BK_VR`; with it off the flat build is
unchanged (the game side's `vr_api.cpp` and profile are compiled in both, inert until the
renderer calls `dk64vr_set_active(1)`).

The renderer side is a port of Donkey Kong 64 VR (`~/code/dk64-vr`, `steam-frame` 1fa9ccb);
`dk64-vr/docs/VR_ARCHITECTURE.md` describes the shared mechanics in full (frame protocol, eye
camera, depth/fog remap, camera smoothing, foveation, HUD split, cinema, menus, settings menu).
This document records what is Banjo-specific and the honest status. The game side (scene
reports, culling, profile) is described in [VR_GAME_SIDE.md](VR_GAME_SIDE.md).

**Nothing here has been worn.** Every row in the status table is at most "verified unattended".

## 1. Target and probe facts

- Native Linux ARM64 title (SDL2 + Vulkan through RT64/plume), devkit title `BanjoVR`, compat tool
  `SteamLinuxRuntime_4-arm64`. Persistent data in `$XDG_DATA_HOME/BanjoRecompiled`
  (`~/.local/share/BanjoRecompiled` on the Frame) through `APP_FOLDER_PATH`, set by the launcher.
  The game keeps its ROM there as `bk.n64.us.1.0.z64` (the user's original US 1.0 cartridge
  dump, sha1 1fe16320...; never committed), plus saves, mods, settings and `vr.toml`.
- Display: 2160 x 2160 an eye (`resolution_scale` auto). Without a per-app SteamVR entry the
  title runs at the display's 72 Hz; the recommended per-app setting is in section 7.
- Device: Turnip Adreno 750, Vulkan 1.4 (as DK64).

## 2. Submodules carrying port commits

| Path | Port remote | Branch |
| --- | --- | --- |
| `lib/rt64` | `astromaddie/dk64-vr-rt64` (private, shared with DK64 VR) | `steam-frame` |
| `lib/rt64/src/contrib/plume` | `astromaddie/dk64-vr-plume` (private) | `steam-frame` |

`lib/RecompFrontend` and `lib/N64ModernRuntime` are BanjoRecomp's own pins, unchanged.

## 3. Build

```sh
BK_ROM="/path/to/Banjo-Kazooie (USA).z64" tools/docker/build-linux.sh all   # flat
JOBS=5 BK_VR=1 tools/docker/build-linux.sh build && BK_VR=1 tools/docker/build-linux.sh package \
  && BK_VR=1 tools/docker/build-linux.sh abi                                # VR
BUILD_DIR=build-vrtest tools/docker/build-linux.sh vrtest                   # host tests
FRAME_HOST=steamos@192.168.1.31 tools/steam_frame/deploy.sh --rom "/path/to/rom.z64"
```

The image is `dk64-vr-linux` (built from the same `tools/docker/linux.Dockerfile`). The ROM step
uses `tools/bk_decompress_rom.py`, a Python port of the decomp's `bk_rom_decompress` (output
checked against the decomp's sha1 1fb13cad...). The package is `$ORIGIN/lib` with SDL2 and
libatomic bundled, libstdc++ static; the ABI check passes the glibc 2.35 floor.

## 4. What is Banjo-specific in the renderer

- **Game profile** (`src/vr/game/banjo_profile.c`, game side): camera matrix groups `0x1000`
  (world) and `0x1001` (sky), player height 138 units, default `world_scale` 100 units/m (Banjo
  1.38 m), player name "Banjo". The renderer takes the camera groups, the default scale and the
  menu's "World size" (Banjo's height) from it. The `world_scale` range is DK64's 4-100 around
  its 15 scaled to the profile: 26.7-667 units/m for Banjo (5.2 m to 0.21 m tall).
- **Profile extension** (`src/vr/include/vr_game_profile_ext.h`, renderer side, not frozen;
  Banjo's values in `src/vr/game/banjo_profile_ext.c`, DK64's defaults weak in
  `profile_ext_default.c`):
  - `perspective_hud_after_camera` = 1. Banjo draws its HUD counters through a perspective
    projection (`0x1004`), its dialogue portraits through `0x1100 + id`, and its title/game-over
    overlays through `0x1003`, `0x1005-0x1008`. RT64's HUD classifier
    (`WorkloadQueue::threadVrClassifyCalls`) sends every perspective projection drawn into the
    main image after the world camera and outside the camera groups to the HUD image when the
    host asks (`VrHost::perspectiveHudAfterCamera`, rt64 947a2a0). They are never given the eye
    camera (the eye camera only replaces projections in the camera groups). Unattended dump:
    Grunty's 3D portrait and dialogue box in the HUD image, absent from the eyes.
  - `takes_comfort_turn` = 0. Banjo has no camera-heading patch for `dk64vr_take_turn`, so the
    right stick defaults to the game's own C-left/C-right camera swing (`right_stick_x` =
    camera). Setting it back to "Comfort turn" in the VR menu would show a snap that unwinds
    after 8 s, because the game camera never follows.
- **Camera cut distance**: 4 m in game units from the profile (400 units for Banjo; DK64 60).
- **Controls**: BanjoRecomp's gamepad defaults are the same as DK64 Recompiled's (both come
  from the same `set_control_defaults`), so the Frame map is identical: A jump/fly up, X attack
  (N64 B), LT crouch (Z), RT centre camera (R), Y/B camera left/right (C-left/C-right; with LT:
  Talon Trot/Wonderwing), LB C-down (zoom; LT+LB egg backwards), RB C-up (first person; LT+RB
  egg forwards), Menu Start, R3 N64 L, D-pad N64 D-pad (unused by Banjo), left stick N64 stick.
  System layer as the standard: View short = recomp menu, View hold 1 s = recentre, Menu + View
  0.5 s = VR settings menu. Right stick X: the game camera swing (above); right stick Y: nothing.
- **Pause**: `src/vr/game/vr_control.cpp` implements `dk64vr_control.h` (pause flag, turn queue,
  HUD diagnostic); the game thread waits in `recomp_vr_publish_game_state` while the host pauses
  it (headset off, focus lost, VR settings menu), as in DK64.
- **Unattended start**: `--game bk` (the vr-diag `ARGS` line, default in `unattended.sh`) starts
  the game from the launcher's first update (`src/main/main.cpp`; BanjoRecomp's runtime pin has
  no `--game` option of its own). `dump_in_play=1` makes `dump_frame`/`dump_interval` count only
  frames of live stereo play, so dumps land in gameplay after the long new-game intro (which the
  game lets you skip only when a save file exists, `cutscene_skipIntroCutsceneCheck`).
- **Naming**: `BK_VR_<KEY>` environment overrides, OpenXR application name `BanjoVR`, action set
  "Banjo-Kazooie", logs `[vr]`, `[vr-perf]`, `[openxr]` as in DK64, `[vr-scene]` from the game side.

## 5. Frame protocol, eye camera, HUD, cinema, menus

As DK64 VR (its sections 4, 5, 5c, 9, 9a). Banjo's world projection is `guPerspective` then roll,
pitch and yaw on the projection stack, with the camera translation in the model matrices and a
compensating `gEXSetViewMatrixFloat`; RT64 folds the rotations into its view and the explicit
view matrix supplies the translation, so the eye camera sees an ordinary world-to-view matrix and
the DK64 eye injection applies unchanged.

## 6. Status

"Unattended" means a Steam launch on the Frame (`steam.pipe`) with `force_render`, checked in logs
and dumps; it never pressed a real button and nobody wore it.

| Feature | State |
| --- | --- |
| Linux ARM64 flat build and package, ABI check | builds; container smoke run reaches the launcher UI; flat not run on the Frame |
| VR build, host unit tests (8,930 checks, DK64 profile) | pass |
| OpenXR host, Vulkan binding, session lifecycle, clean exit (`diag_exit_seconds`) | verified unattended |
| Scene routing from the game side (boot/title/file select on the panel, intro in the cinema, transitions freeze, gameplay in stereo) | verified unattended (log: transition, menu, cutscene/transition through the intro, gameplay after about 3.5 min of autostart presses) |
| Stereo gameplay eye images, culling behind the head | verified unattended (Spiral Mountain at Banjo's house, yaw 0/90/180: both eyes consistent, world-locked, scenery present behind the head; section 6a) |
| HUD split incl. Banjo's perspective HUD and portraits | verified unattended (dialogue box and Grunty portrait in the HUD image, not in the eyes) |
| 3D cinema for the intro cutscenes | verified unattended (cinema layer, picture rows 0-1: Banjo's pillarbox is geometry, so the whole frame is the picture) |
| Controller bindings (Frame, Index, Touch suggested and accepted) | verified unattended (all three accepted); no real button press yet |
| Pause (game thread waits in the state publish) | verified unattended through the VR settings menu (game 0.0 Hz while open, resumes on close); standby/focus pause not exercised (runs force rendering) |
| VR settings menu (quick menu, full panel, all tabs) | verified unattended (`diag_vr_menu_at` tour over the frozen cinema; "Banjo 1.4 m", right stick "Game camera"); real input headset-unverified |
| Recomp menus on the panel, pointer | built (shared code); not exercised on Banjo |
| Billboard sprites facing the eye, not the game camera | not started (game side lists it as renderer work) |
| Comfort turn | off by default (no camera patch); game camera swing on the right stick |
| Worn | never |

### 6a. Measured (unattended, off-head, Steam launch, 2026-10-06)

No per-app SteamVR entry yet, so the display ran at 72 Hz; eyes 2160 x 2160, foveation high
(eye-tracked, falling back to the projection centre off-head), `world_scale` 100.

| Scene | fps | Missed / 10 s | GPU a frame (both eyes + copy) |
| --- | --- | --- | --- |
| Spiral Mountain, gameplay (2 windows x 2 runs) | 71.7-71.8 | 1-2 | 5.8-6.9 ms (eyes 2.8-3.5 ms each) |
| Intro cutscenes in the cinema | 71.7-72.0 | 0-2 (22-24 in the window with the first stereo switch) | 5.6-8.0 ms |

At 120 Hz the frame budget is 8.33 ms; the gameplay GPU time fits, but 120 Hz itself is
unmeasured (needs the per-app setting and a Steam launch). Dumps looked at: Spiral Mountain eye
pairs at yaw 0/90/180 (both eyes, consistent stereo, the house, fence, flowers and cliffs in
place, nothing missing behind the head), the HUD image (lives and honeycomb health counters,
solved alpha, absent from the eyes), the intro's dialogue box with Grunty's 3D portrait in the
HUD image, Gruntilda's lair in the cinema, and the VR settings menu panels.

Not judged from dumps: depth comfort, scale (Banjo is small and far in the starting shot),
billboard orientation when the head turns, and anything about input.

## 7. SteamVR per-app setting

Set **BanjoVR** to **120 Hz** and resolution **2160** in SteamVR's per-app settings in the headset
(the same as DK64VR). The app renders 2160 x 2160 an eye whatever SteamVR recommends.

## 8. Wear checklist (about 10 minutes)

See [WEAR_CHECKLIST.md](WEAR_CHECKLIST.md).
