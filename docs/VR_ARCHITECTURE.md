# Banjo VR (Steam Frame) architecture

This fork adds native OpenXR stereo rendering to Banjo: Recompiled. The optional VR renderer is
enabled with `BK_VR`; flat mode remains the default. The game integration, Banjo profile and
shared renderer are described here, with Banjo's scene rules and camera details in
[VR_GAME_SIDE.md](VR_GAME_SIDE.md). The DK64 architecture document covers mechanics shared by
both ports, including the OpenXR frame loop, eye rendering, HUD, cinema, menus and settings.

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
  - `sky_camera_group` = `0x1001`: the sky's eye cameras sit at the head's centre (section 4b).
  - `pause_menu_hud` = 1: the pause menu shows from the HUD image over the frozen world
    (section 4c).
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

### 4b. The sky at infinity

Banjo's sky (`sky_draw`, projection group `0x1001`, identity view) is a small model centred on the
game camera, drawn first without depth. Measured with `RT64_VR_LOG_PROJECTIONS` on Spiral Mountain,
its vertices lie **64-190 units** from the camera: 0.6-1.9 m at `world_scale` 100. Given each eye's
camera like the world, that put the sky at arm's length: in the eye dumps its clouds had 51-87 px
more crossed disparity than Gruntilda's tower, which sat at infinity, and every head movement
moved the eyes inside a dome about a metre across. In the headset it read as a sky that turns with
the head. Now the sky's eye cameras keep each eye's rotation and projection but sit at the head's
centre with no head translation (`xr_host.cpp` `eyeCamera`, through RT64's `eyeCameraGroup`), so
it is at infinity: the same cloud patches now match at exactly the infinity offset (0 px excess)
and the tower is unchanged. The yaw 0/90/180/270 dumps show different parts of the sky (blue above
the house, Gruntilda's green clouds above her lair), level with the world. `sky_at_infinity = 0`
brings the old behaviour back for A/B.

Note on eye dumps: the lens's hidden area (outside the octagon-like outline) isn't drawn by the
depth-tested world. The sky, drawn without depth, fills it, so the right edge of a left-eye dump
shows the sky's lower half as a flat green area. The lenses never show that region.

### 4c. Pause menu over the frozen world

Banjo's pause menu draws its background as a copy of the last world frame: on the first paused
frame `gcpausemenu_draw` copies the colour buffer into the Z-buffer (`func_80315084`), and every
later paused frame blits it back (`func_80315110`) over a black fill; the world itself isn't drawn
while paused. In VR the patched blit `func_80314BB0` (`patches/pause_menu_patches.c`) returns
without drawing, so paused frames hold only the menu: the zoomboxes (3D models in the `0x1004` HUD
perspective), their portraits and text, the item counters and the spinning jiggy.

The host keeps the stereo world frozen and dimmed as before. Once the pause panel latches it asks
RT64 for `VrFrame::Mode::Hud` frames (rt64 0b59675) instead of flat ones: each paused frame is
rendered once at the eye size and its HUD calls are solved into the HUD image. With no camera
group in the frame, every non-full-frame 2D draw and every perspective outside the camera groups
counts as HUD; the black fill covers the frame and stays out. The HUD image takes the panel's place
(world-locked, 2 m, `panel_width` wide, 4:3) over transparent black, like DK64's VR settings menu.
A recomp menu over the pause still uses the panel. `pause menu: on/off the HUD image` in the log.

`diag_pause_at=N` (seconds of live stereo play) dumps the eyes, presses Start, dumps the HUD image
(`vr-hud-101/102`) and the panel 1.5 s and 4.5 s into the pause, then presses Start again.

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
| Sky at infinity (section 4b) | verified unattended (yaw 0/90/180/270 eye dumps; stereo matching of the clouds before/after) |
| Pause menu from the HUD image over the frozen, dimmed world (section 4c) | verified unattended (`diag_pause_at`: menu complete in the HUD image with solved alpha, composite over the frozen eye image, pause and resume logs); real Start press and worn look pending |
| Pause (game thread waits in the state publish) | verified unattended through the VR settings menu (game 0.0 Hz while open, resumes on close); standby/focus pause not exercised (runs force rendering) |
| VR settings menu (quick menu, full panel, all tabs) | verified unattended (`diag_vr_menu_at` tour over the frozen cinema; "Banjo 1.4 m", right stick "Game camera"); real input headset-unverified |
| Recomp menus on the panel, pointer | built (shared code); not exercised on Banjo |
| Billboard sprites facing the eye, not the game camera | verified unattended (section 4a; host tests; Spiral Mountain: the cliff statue sprite upright and facing the eyes, the only billboard in that view; particles not seen on the device) |
| Comfort turn (snap 45°, smooth orbit, native camera follows) | host/import and generated-MIPS tests pass; ARM64 build/package and flat compilation verified; device/worn acceptance pending |
| RT64 per-eye resource slots, removing the wait between eye submissions | host work in this branch; headset-unverified; A/B and resource inventory in [EYE_WAIT_TEST.md](EYE_WAIT_TEST.md) |
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

## Camera standard migration delivery (2026-10-07)

Source `b34d2b380f58e200aa054076b5be98bd45819522` passed the existing host VR checks, native ARM64 build,
ABI/dependency closure and package-content/hash checks. Package metadata correctly records the preserved pre-existing nested dependency dirt.
Successful host/build/ABI command output remained in tool results; persistent
BUILD-METADATA.json and the final package are retained as build evidence.

The adapter now uses shared sfvr adjustment/defaults conventions, apparent size,
height, head gain, seated calibration, smooth/snap/off choices and live retained
world rendering while settings freeze simulation. The existing yaw-only settings
panel defaults to the right, with left/centre alternatives. Physical left grip
plus physical right stick adjusts size horizontally and requests camera offsets
vertically, consumes the stick before game/turn readers, and requires neutral
after release or focus loss. Gameplay controls retain their existing mappings.

**Incomplete:** negative closer/lower values are saved but those offsets are not
rendered. The established nonnegative nearer/lower path remains. A safe native
collision query and guest-state restoration contract for farther/higher movement
is not established. Automatic approval review rejected the proposed Banjo guest
call because collision-state restoration was unproven and could corrupt game
state. No guessed call was substituted or shipped.

Installed `~/devkit-game/BanjoVR/BanjoRecompiled` SHA-256:
`8ca783a762c08569dd12af4b3e822fdc31c38d219f2a544d53b58be887922cdd`. The booked, idle existing Steam-title launch (raw app ID 2393272997)
reached SteamVR/OpenXR, 2160x2160 eye swapchains and READY/SYNCHRONIZED; Frame,
Index and Touch profile suggestions were accepted (23 Frame paths). Only the
task's native process was stopped. The rewritten VR settings were restored;
settings/save hashes and modes match the pre-run values.

| Current increment | Evidence level |
| --- | --- |
| ARM64 package and Steam boot/session/profile suggestions | verified unattended |
| New live gameplay preview, physical modifier, tracked recentring and comfort | not worn; preview not captured |
| Farther/higher camera extension | blocked; negative offsets inactive |

Wear checks: live scale and safe nearer/lower adjustment; neutral release;
right-side panel readability; pause/settings/resume; recentre and turning. Skip
negative offsets until a verified native collision adapter exists. Evidence:
task output `device-camera-migration/BanjoVR/run-stdout.txt` and the cross-port
report. The smoke boot does not certify gameplay rendering or physical input.
