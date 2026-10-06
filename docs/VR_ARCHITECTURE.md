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
JOBS=4 BK_VR=1 tools/docker/build-linux.sh build && JOBS=4 BK_VR=1 tools/docker/build-linux.sh package \
  && JOBS=4 BK_VR=1 tools/docker/build-linux.sh abi                                # VR
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
  - `billboard_groups`: camera-facing billboards (section 4a).
  - `takes_comfort_turn` = 1. The free-camera patch consumes `recomp_vr_take_turn(applicable)`
    and changes Banjo's R-look target yaw. Right stick defaults to comfort turn, snap 45 degrees;
    `right_stick_x = camera` keeps the native C-left/C-right swing.
  - `reports_turn_applicability` = 1. The game reports rejection for forced/locked cameras;
    the host suppresses new turns and clears compensation there. `cam_at` publishes the actual
    native focus in active third-person play, so the eye orbit rotates around Banjo. Section 5d.
- **Camera cut distance**: 4 m in game units from the profile (400 units for Banjo; DK64 60).
- **Controls**: BanjoRecomp's gamepad defaults are the same as DK64 Recompiled's (both come
  from the same `set_control_defaults`), so the Frame map is identical: A jump/fly up, X attack
  (N64 B), LT crouch (Z), RT centre camera (R), Y/B camera left/right (C-left/C-right; with LT:
  Talon Trot/Wonderwing), LB C-down (zoom; LT+LB egg backwards), RB C-up (first person; LT+RB
  egg forwards), Menu Start, R3 N64 L, D-pad N64 D-pad (unused by Banjo), left stick N64 stick.
  System layer as the standard: View short = recomp menu, View hold 1 s = recentre, Menu + View
  0.5 s = VR settings menu. Right stick X: comfort turn (above); right stick Y: nothing.
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

### 4a. Billboards face the viewer

Banjo builds its sprites from the flat camera's rotation (`sViewportMatrix`, yaw and pitch), so in
the headset they face the game camera and turn edge-on away from the centre of its view. In VR the
patches add a flag to the matrix group id of each camera-facing draw (`patches/transform_ids.h`;
every plain id is below `0x40000000`, interpolation pairs the flagged id as before):

| Draw | Where | Flag | Facing |
| --- | --- | --- | --- |
| Prop and actor sprites (notes, flowers, eggs, extra lives, the statue on Spiral Mountain's cliff) | `func_80344138` (`vr_sprite.c`), with the prop's or marker's transform id | `0x40000000` | cylindrical: upright, turns about the vertical only |
| Model billboard nodes, yaw only (palm trees, fire sparkles, Mumbo's skull candles, half the Christmas tree's lights) | `func_803384A8` (`transform_tagging.c`) via `set_model_matrix_group` | `0x40000000` | cylindrical |
| Model billboard nodes with pitch (twinklies, the other Christmas tree lights) | same | `0x80000000` | spherical |
| Camera-facing particles (smoke, sparkles; not `PART_EMIT_ROTATABLE`) | `__particleEmitter_drawOnPass` | `0x80000000` | spherical, own roll kept |
| Camera-facing projectiles (smoke puffs) | `func_8033F7F0` when `unk28_12` | `0x80000000` | spherical |
| Grunty's spells | `func_80325CAC` (`vr_sprite.c`) | `0x80000000` | spherical |

Authored-rotation sprites (`func_80344720` from rotatable particles and `func_80325AE0`), ground
decals and the HUD keep their orientation. The profile extension names the two flagged ranges;
RT64 (`rt64_vr_billboard.h`, `TransformProcessor::faceBillboards`) rebuilds those model matrices
in each eye pass after interpolation: it recovers the game's camera basis from the matrix (local
Z, level X), keeps what the game added to it (roll, scale, mirroring), and replaces the basis with
one facing the viewpoint. The viewpoint is the point midway between the eyes from the same view
as the eye cameras (`billboardViewpoint`), so both eyes see one flat card (per-eye facing would
turn it ~2 degrees apart at 1 m and break the stereo of its edges) and a pure head turn does not
move it. A billboard turns about its matrix origin, or about the centre of its vertices when the
origin lies outside them (map billboard nodes pivot in map coordinates). The cinema keeps the
game's facing. Setting `billboard_facing` (default 1) turns it off; `RT64_VR_LOG_BILLBOARDS=300`
logs the turned billboards and any untagged matrix that faces the game camera.

Why upright for sprites: they stand for upright things, and the eye is often above or below them
(the third-person camera, a lean); a fully facing card tilts back and leaves the ground. Particles
and effects are volumetric puffs, so they face the viewer completely.

## 5. Frame protocol, eye camera, HUD, cinema, menus

As DK64 VR (its sections 4, 5, 5c, 9, 9a). Banjo's world projection is `guPerspective` then roll,
pitch and yaw on the projection stack, with the camera translation in the model matrices and a
compensating `gEXSetViewMatrixFloat`; RT64 folds the rotations into its view and the explicit
view matrix supplies the translation, so the eye camera sees an ordinary world-to-view matrix and
the DK64 eye injection applies unchanged.

### 5d. Comfort turn and coordinator device check

Source details and camera exclusions: [VR_GAME_SIDE.md](VR_GAME_SIDE.md#comfort-turning).
The game applies whole-degree requests before its native camera update: R-look target
`D_8037DBA4`, current radial yaw `D_8037DBA8`, with unwrapped debt to guide the wrapped native
follower. A 45-degree change takes **1.333 s** to within 0.01 degrees in the original-ROM
30 Hz follower with controlled collision-free inputs. Camera mode interpolation and presentation
smoothing add their own convergence; device timing remains unmeasured. The host's pure
`ComfortTurn` (`src/vr/bridge/dk64vr_turn.*`) holds the shown unwrapped heading while the game
eases. `orbitViewCorrection` uses the sampled real native focus and the main `0x1000` camera,
after camera smoothing/offsets. Its single camera-space correction is applied to both world
and sky projections; the sky's camera-relative origin must not orbit about an absolute world
focus independently. Movement is projected through the same angle. A smooth hold runs at the
requested rate, stopping on release.
An abandoned multi-revolution turn unwinds by the equivalent shortest angle over 0.4 s,
preserving its physical starting view while avoiding extra spins.

The source milestone passes `vrtest` (8,930 existing checks plus 1,420 new comfort/import checks),
`tools/vr/test_game_side.sh`, and `tools/vr/test_generated.sh` (actual N64Recomp functions,
original-ROM inactive camera dispatch, native R-look follower, water/flight retention and
rejection). Both VR and flat Linux ARM64 configurations compile. The VR package passes the
ABI check. Flat mode reuses the same build tree with `BK_VR=0`, then VR is rebuilt and repackaged;
no duplicate build-tree copies. This is host/build evidence only. No headset was accessed.

After integrating this branch and the billboard branch, the coordinator must build/package
that exact merged source and book the Frame with its `frame-lock.sh` before deploy/run. With
BanjoVR installed and the ROM/save present, use the existing Steam-title unattended helper:

```sh
# Book the Frame first. The helper writes one-shot vr-diag, starts through Steam, and fetches logs.
FRAME_HOST=steamos@192.168.1.31 tools/steam_frame/unattended.sh 300 build/turn-snap-device \
  force_render=1 diag_autostart=240 diag_exit_seconds=290 diag_camera_log=1 \
  right_stick_x=turn turn_mode=snap snap_angle=45 snap_blink=0 \
  diag_turn_stick=1 diag_snap_seconds=0 diag_camera_seconds=0 \
  diag_yaw=0 camera_smoothing=medium camera_lower=0 camera_closer=0
FRAME_HOST=steamos@192.168.1.31 tools/steam_frame/unattended.sh 300 build/turn-smooth-device \
  force_render=1 diag_autostart=240 diag_exit_seconds=290 diag_camera_log=1 \
  right_stick_x=turn turn_mode=smooth smooth_turn_speed=120 snap_blink=0 \
  diag_turn_stick=1 diag_snap_seconds=0 diag_camera_seconds=0 \
  diag_yaw=0 camera_smoothing=medium camera_lower=0 camera_closer=0
```

Exact one-shot `~/.local/share/BanjoRecompiled/vr-diag` lines for the snap run (the helper's
`key=value` arguments become these environment overrides):

```text
ARGS=--game bk
BK_VR_FORCE_RENDER=1
BK_VR_DIAG_AUTOSTART=240
BK_VR_DIAG_EXIT_SECONDS=290
BK_VR_DIAG_CAMERA_LOG=1
BK_VR_RIGHT_STICK_X=turn
BK_VR_TURN_MODE=snap
BK_VR_SNAP_ANGLE=45
BK_VR_SNAP_BLINK=0
BK_VR_DIAG_TURN_STICK=1
BK_VR_DIAG_SNAP_SECONDS=0
BK_VR_DIAG_CAMERA_SECONDS=0
BK_VR_DIAG_YAW=0
BK_VR_CAMERA_SMOOTHING=medium
BK_VR_CAMERA_LOWER=0
BK_VR_CAMERA_CLOSER=0
```

For the smooth run change `BK_VR_TURN_MODE=smooth` and add
`BK_VR_SMOOTH_TURN_SPEED=120`. `diag_turn_stick=1` holds X for 4 s, releases for 4 s, and repeats
only during applicable stereo gameplay. With snap, there is one 45-degree step per hold.
Use a save/autostart that reaches **Spiral Mountain**, and require `[vr-scene] ... -> gameplay`
and `[vr-turn] camera applicable` before measuring. If it never reaches gameplay, the check
has not run; intro/cinema logs are not turn evidence.

Proof in the per-display `[vr-cam] turn f N applicable A game G target T remaining R shown S`
(with post-orbit `pos` and sampled `focus` following): S is measured from the rendered
projection's post-orbit view matrix, unwrapped near G + R, rather than echoing the requested
target. World and sky share the heading correction. Position uses the main camera's world
origin even when the first rendered projection is the rebased sky.

- On snap, consecutive **f N / f N+1** with A=1 must change S by **-45.000 degrees** (right),
  while G changes only by the native camera's small step. The signed relation is S = G + R;
  T changes by -45.000 and stays there during that hold. Head pose `diag_yaw=0` prevents a
  synthetic head turn from explaining it. `[vr-turn] accepted 45 deg ...` proves the native
  heading write. The next several seconds must show R shrinking toward zero and G approaching
  T, followed by `turn: game camera arrived`, without an 8-second timeout/unwind.
- On smooth, S must change by **-120 * (t[N+1]-t[N]) degrees** during the steady hold:
  about **-1.667 degrees/frame at 72 Hz**, **-1.000 at 120 Hz**. Use t from the matching
  `[vr-cam] f N t ... raw yaw ... out yaw ...` line and actual display period. Over a 4-second
  hold T moves -480 degrees, crossing a wrap; game G stays unwrapped too. On
  `[vr-cam] diag: right stick released`, T and S stop immediately while G catches up and R
  falls. The window's `[vr-cam] shown: yaw rate ... turn requested ...` is corroborating
  evidence; a mixed held/released 10-second window will not average -120 deg/s.
- Check native swing separately with `right_stick_x=camera` and a real right stick (the synthetic
  comfort diagnostic intentionally does nothing in that setting). Enter a fixed/node camera,
  first person, pause and the final arena: no accepted heading write; when stereo turn lines
  continue, A=0 / `camera not applicable` and R=0. Panel/cinema/frozen routing can stop those
  lines entirely, so check the scene/routing/pause logs and absence of accepted turns there.
  Real input, collisions during compensated orbit, transformations/flight in
  actual maps and worn comfort remain acceptance work.

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
| Billboard sprites facing the eye, not the game camera | verified unattended (section 4a; host tests; Spiral Mountain: the cliff statue sprite upright and facing the eyes, the only billboard in that view; particles not seen on the device) |
| Comfort turn (snap 45°, smooth orbit, native camera follows) | host/import and generated-MIPS tests pass; ARM64 build/package and flat compilation verified; device/worn acceptance pending |
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
