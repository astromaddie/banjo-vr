# Banjo-Kazooie VR game side

Game-side integration for BanjoRecomp US 1.0, based on `steam-frame` 009795a.
The frozen `src/vr/include/dk64vr_game_state.h` and `vr_game_profile.h` are unchanged.
No renderer source, submodule, headset, deployment, or remote branch was changed.

## Profile and camera evidence

`src/vr/game/banjo_profile.c` supplies:

| Field | Value | Evidence |
| --- | --- | --- |
| World camera groups | `0x1000`, then `0x1001` | `patches/transform_ids.h:16`, `:17`; main draw sets `PROJECTION_GAMEPLAY_TRANSFORM_ID` in `projection_transform_tagging.c:93`; sky temporarily sets `PROJECTION_SKYBOX_TRANSFORM_ID` in `sky_transform_tagging.c:71`. `camera_transform_tagging.c:103` emits the projection matrix group. Existing tags are consistent; interpolation flags and IDs were retained. |
| Player model height | 138 game units | ROM model assets `0x34D` (low poly, 2,970 vertices) and `0x34E` (high poly, 5,184 vertices) both have header and measured vertex Y bounds `-7..131`. This is the authored reference-pose model envelope, not a sampled maximum during jumping or Kazooie animations. |
| Default world scale | 100 units/metre | 138 / 100 = **1.38 m**, a starting size for the short, broad anthropomorphic bear. This is a presentation choice, not a canonical physical height; retain the renderer's scale setting. |
| Game near | 10 units | Normal gameplay floor in `lib/bk-decomp/src/core2/code_34790.c:143`; actual near varies with map bounds, camera collision, and cutscene/file-select rules. Each snapshot carries the effective value after perspective clamps. |
| Player name | `Banjo` | Normal character; transformation heights do not redefine this profile. |

`baModel_reset` uses scale 1 (`lib/bk-decomp/src/core2/ba/model.c:140`), and the player draw
passes that scale to `modelRender_draw` (`:105`, `:108`). `include/model.h:36` describes the
vertex-list bounds and `:190` gives its offset in the model binary. Reproduce the measurement:

```sh
python3 tools/vr/prepare_rom.py '/path/to/Banjo-Kazooie (USA).z64'
```

The helper checks SHA1 `1fe1632098865f639e22c11b9a81ee8f29c75d7a`, inflates the two assets
in memory, and compares header bounds with every vertex. It writes no extracted asset. With
`--decompress banjo.us.v10.decompressed.z64`, it also reproduces the decomp's US overlay
placement and beach/desert swap (`bk_rom_compressor/src/decomp/main.rs`), validating every
RareZip output length. It refuses to overwrite an existing output. ROMs remain untracked.

Banjo uses `guPerspective` with FOV normally 40 degrees and projection scale 0.5, followed by
roll, pitch, and yaw on the projection stack (`camera_transform_tagging.c:60`). The ordinary
world model matrices already subtract the game camera position. The existing positive
camera-translation `gEXSetViewMatrixFloat` compensates for that in RT64; **retain that split**.
Sky models are centred on the game camera, consequently camera-relative after model subtraction,
and use an identity explicit view matrix instead. Do not transplant DK64's gradient-sky drawing.

## Per-frame protocol and scene rules

`baMotor_80250C08` (`patches/recompui_patches.c:10`) is reached exactly once by `mainLoop`,
including paused/loading iterations (`lib/bk-decomp/src/core1/code_0.c:151`). It calls
`vr_frame_hook` after the existing UI callbacks, before frame update/draw/submission locks.
This publishes the preceding completed draw's captured camera and sky latch. Current game
mode may lead or lag that workload by a tick, as permitted by the frozen contract. Consumers
must latch scene routing, not interpret snapshot.frame as a display/XR frame identifier.
The game simulation and display-list building still run once, never once per eye.

The world projection hook captures eye, at (eye plus unit forward), up including roll, FOV,
and **effective** near/far (`camera_transform_tagging.c:56`, `vr_scene.c:24`). Sky/HUD/temporary
projections are excluded. `vr_frame_hook` adds player position and degree-to-radian yaw,
zeroes those player fields when no player is present, increments the snapshot frame counter,
and consumes the sky latch. Before the first world projection the camera fields are zero;
stay on the panel until a valid world perspective is available. With no new draw during a
load, the last camera is retained; the scene tells the renderer to freeze/show its panel.

`patches/vr_scene_rules.h:11` implements these priority-ordered rules:

| Scene | Rule | Renderer treatment |
| --- | --- | --- |
| Transition | `gctransition_active()`, or game mode 1/2 (load/change mode) | Freeze/fade the last stereo world; render native transition content flat. Covers fade-to/from-black/white, jigsaw/zoom/falling-piece effects and destination loads. |
| Pause | Game mode 4 | Freeze/dim world and show pause/totals/menu panel. |
| Menu / 2D | Nintendo/concert boot map 0x1E, Rareware boot map 0x1F, file select 0x91, game-over map 0x83; attract demo mode 7, title mode 9, Bottles puzzle mode 8, SNS picture mode A | Panel. The complete Nintendo/concert boot map deliberately remains panel content; story intro/Grunty maps below use cinema. Bottles and photos use framebuffer imagery/2D interaction despite containing models. Unknown non-play modes also fail closed to menu. |
| Cutscene | `LEVEL_D_CUTSCENE`, camera types 1 (script-controlled), 3 (static), 4 (random), or player states `BS_44_JIG_JIGGY` / `BS_34_JIG_NOTEDOOR` | Cinema by default. Covers story intro, Grunty shots, endings, static camera reveals, jiggy dance and note-door fanfare. |
| First person | `ncFirstPersonCamera_getState()` 1..3 (enter, look idle, exit) | Stereo with first-person camera anchor. State 4 means done. |
| Gameplay | Normal mode 3 after the above rules | Third-person stereo. |

Evidence: mode enum `include/enums.h:2585`; transition predicates
`src/core2/gc/transition.c:385`; camera dispatch `src/core2/code_34790.c:277` and
`src/core2/nc/staticCamera.c:48`; first-person state machine
`src/core2/nc/firstpersoncamera.c:94`; fanfares `src/core2/bs/jig.c:21`, `:163`.
Egg-head/egg-rear actions (`bs/bEggHead.c:18`, `bs/bEggAss.c:15`) do **not** enable a
first-person aim mode in this game. They retain third-person gameplay unless an independent
first-person camera is active. A normal note pickup/fanfare with a free camera remains gameplay;
only the note-door dance or a forced camera triggers cinema. There is no BK multiplayer or
arcade scene to publish. Dialogue alone does not force cinema; its actual camera type does.

While active, changes log `[vr-scene] old -> new` (`vr_scene.c:71`). Inactive reporting is
silent and resets the logged scene to unknown for the next activation.

## Host bridge

`src/game/vr_api.cpp` implements the frozen `dk64vr_*` API. The historical DK64 prefix is
intentional. Snapshots are 19 aligned words / 76 bytes, with scene-number and size assertions
on both sides. Atomic words plus an even/odd sequence protect concurrent readers; writers are
serialized, and sequentially consistent operations avoid a C++ data race/torn generation.
The import copies word-swapped RDRAM as native aligned words (ints/floats are host endian).

Imports (`patches/syms.ld:58`):

- `recomp_vr_is_active` = `0x8F0000D0`, returns `dk64vr_is_active()`.
- `recomp_vr_publish_game_state` = `0x8F0000D4`, takes the MIPS pointer in a0.

The host and profile are in the normal CMake source list. Active defaults to zero. The renderer
calls `dk64vr_set_active(1)` only when its stereo path is active and clears it on shutdown/loss.
`BK_VR_FORCE_PATCHES=1` forces gates/logging for a flat diagnostic process; unset, empty, or a
value starting with `0` does nothing. It is sampled once per process and is **not** a replacement
for a VR runtime. This bridge supplies the two frozen interfaces only; pause/comfort-turn/input
APIs from DK64's separate `dk64vr_control.h` are outside this game-side contract. Standby
simulation blocking and controller integration need the renderer/coordinator's separate wiring.

## Every new VR gate

Every effect below is conditional on `recomp_vr_is_active()`; reporting itself only reads game
state and writes private VR state. With active zero, existing game branches, interpolation tags,
callbacks, RNG calls and display-list commands are retained. New copies have explicit native
branches. This is behavioral parity, not identical executable bytes or zero added CPU overhead.

| Function and gate location | Condition / VR effect | Off path |
| --- | --- | --- |
| `viewport_isBoundingBoxInFrustum`, `patches/culling_patches.c:101` | Active returns true. Cube and remaining AABB tests include geometry outside the flat camera, including demos. | Existing original copied plane test in demos; existing unconditional pass during normal recomp play. |
| `viewport_func_8024DB50`, `patches/culling_patches.c:114` | Active returns true. Model sphere nodes, actors/props, snow and particle/emission callers no longer reject by game-camera planes. | Existing transient `frustum_checks_enabled` gate and exact plane-dot test. |
| `func_80302C94`, `patches/vr_culling.c:56` | Active skips camera pitch/yaw half-space restrictions on cube traversal. Visits both sides while retaining four-cell radius, fallback cubes, and native sorting/final draw calls. | Original half-space selection and traversal sequence. |
| `func_802BBA84`, `patches/vr_culling.c:143` | Active calculates far from the furthest map-bound corner's radial distance, plus the native 100-unit margin and 1,000..20,000 limits. Head turns cannot reveal geometry clipped by a far plane sized only along the flat camera direction. | Original integer `coords_func_8033EAF8` projection result, near/collision rules and limits. Map bounds are signed integer words despite misleading local float declarations in the decomp; the patch declares the ABI correctly. |
| `func_80344138`, `patches/vr_sprite.c:37` | Active uses radial distance for billboard prop sprite rejection/size LOD instead of forward dot; geometry behind the game camera can reach RT64. | Original forward test, size LOD, callbacks, matrices, segment/display-list commands and reset calls. |
| `func_80344424`, `patches/vr_sprite.c:99` | Same for camera-facing particle sprites with roll. | Original path. |
| `func_80344720`, `patches/vr_sprite.c:161` | Same for authored-rotation sprites (e.g. ground-oriented particle quads). | Original path; authored orientation retained. |
| `func_80350818`, `patches/sky_transform_tagging.c:116` | Active returns before lens-flare model quads. These lie along the flat camera's projected sun axis and would have incorrect stereo placement. | Existing tagged flare draw. This function contains no timers/updates to preserve on its early return. |

The shared sphere gate also stops camera-frustum-driven particle deletion (`particle.c:654`)
and permits off-camera smoke/spark emissions (`ch/firefx.c:77`, `code_55180.c:58`,
`code_DB010.c:491`, `GV/code_3BB0.c:55`, `FP/ch/xmastreeeggtoll.c:74`). This deliberately
changes visual particle/emitter state and potentially random consumption **when active**.
No flat-mode RNG call was added or removed. VR attract/demo presentation can consequently diverge
from recorded demo fidelity; attract scenes stay on the panel.

## Culling coverage and retained/uncertain sites

Search coverage: all `lib/bk-decomp/src` and `patches` for viewport/frustum APIs, look-vector
and yaw-based rejection, model geometry traversal, sprites, particles/weather, sky/flare and
screen overlays. This is a source inventory, not proof that every model asset lacks an embedded
GBI cull command or that every gameplay visibility check behaves well with head tracking.

| Site | Result / rule |
| --- | --- |
| Model-bin AABB nodes, `culling_patches.c:78` (`func_80338DCC`) | Already always traverse children before this task; retained. |
| Outer model visibility, `transform_tagging.c:682` | Existing recomp forces drawing, retains original visibility flags/callback behavior in demos. VR sphere pass makes the recorded demo cull flag false; normal behavior is unchanged off. |
| Model sphere nodes, `modelRender.c:946`; transient gate `transform_tagging.c:917` | Covered by the new shared sphere gate; existing skinning traversal switch remains intact. |
| Cube tests `viewport_cube_isInFrustum` / `...2`, `viewport.c:313`, `:328` | AABB covered. `...2` retains its orientation-independent 4,000-unit cell-centre distance cutoff. Four-cell traversal radius also retained. These are authored distance bounds, not flat-camera frustum bounds; lean near the limit is unverified. |
| Opaque/translucent map geometry | Existing model-node bypass plus new cube traversal. Texture/fog/material state remains native. |
| Direct point test `viewport_isPointOutsideFrustum_3f`, `viewport.c:374` | Retained. Used in an actor state/collision transition (`code_DA3A0.c:97`), sound gating (`code_DAAA0.c:85`) and CCW spawn selection (`CCW/code_160.c:178`). Globally returning true would alter gameplay. Head direction still does not count as the game's look direction at these sites. |
| `viewport_isPointPlane_3f`, `viewport.c:389` | Retained weather spawn-height adjustment, not draw culling. Native weather spawn distribution can remain camera biased, particularly demos. |
| `viewport_func_8024E030`, `viewport.c:462` | Retained screen projection for Bottles cursor; puzzle is panel content. |
| Model draw-distance/size limits, `transform_tagging.c:673`; sprite radial limits | Retained. No unbounded actor/prop draw-distance expansion. |
| Asset GBI / face culling | Current RT64 F3D/F3DEX `cullDl` handlers are no-ops (`lib/rt64/src/gbi/rt64_gbi_f3d.cpp:106`, `rt64_gbi_f3dex.cpp:41`), so embedded `G_CULLDL` does not reject here. `branchZ` and ordinary face culling remain; asset-specific usage and camera-facing surfaces still need 0/90/180-degree head-yaw validation. |

## Banjo draw inventory and renderer obligations

| Draw | Existing representation | VR rule / remaining renderer work |
| --- | --- | --- |
| World camera / map / Banjo / actors | Perspective `0x1000`; map IDs `0x00400000` / `0x00800000`, Banjo `0x10000000` | Eye-camera replacement only on world perspectives. Preserve explicit view compensation and interpolation/cut handling. |
| Sky and cloud layers | Camera-centred 3D models, perspective `0x1001`, model IDs starting `0x00F20000`; sky near/far 5/15,000 | Keep native world sky and rotation. Include this perspective in eye replay, not HUD extraction. `sky_dome=1` means native sky models were drawn, not a synthetic DK64 dome. In maps with no sky model, native black background remains. Sky origin differs from the absolute world-camera origin. |
| Sun | Sky model textures / flare system | Native sky model is retained; flare quads are suppressed. No separate DK64-style screen sun sprite was found in the sky/flare paths. |
| Lens flare | Camera-relative model quads (`func_80350818`) | Suppressed by gate above. |
| Billboard props/particles | World position; model orientation faces the **game** camera (`vr_sprite.c:65`, `:129`) | Now submitted from all headings. Renderer still needs to face these billboards to the eye/head basis; rotating 90/180 degrees can expose an edge/back without that. Authored-rotation `func_80344720` quads must retain orientation. **Headset-unverified/open renderer work.** |
| Snow/rain/weather | World-space geometry, existing interpolation IDs and denser 360-degree normal-play spawn patches | Keep world perspective. Sphere rejection covered; demo distributions and camera-based spawn-height rules remain. |
| HUD counters, pause/totals, dialog/zoomboxes | Dedicated zero-position/zero-rotation perspective `0x1004` in `projection_transform_tagging.c:125`; orthographic text/portraits | Extract to HUD/panel. This HUD is **perspective**, so DK64's orthographic-only heuristics are insufficient. Never inject eye camera into `0x1004`. Preserve left/right extended-GBI alignment and clipping. |
| Press Start / title / copyright / Game Over / The End | Separate perspectives `0x1003`, `0x1005..0x1008` | Panel or cinema overlay; never world camera. |
| Framebuffer jigsaw/zoom/falling-piece transitions | Dedicated `jiggy_transition_framebuffer`, perspective `0x1002`, model `0x00F00000` (`transition_patches.c:230`, `:251`) | Render once as native panel/freeze content. Do not stereo-inject the transition projection or replay its framebuffer read/update per eye. Scene transition has priority. |
| Full-screen black/white fades, bounds, scissor, black background fills | Native rectangles/transition drawing | Keep native panel output; XR host must fade/dim the whole frozen stereo layer, not only a HUD quad. Source transition timing remains intact. |
| Pillarbox / letterbox and cutscene clipping | Orthographic `0x1009`, model IDs `0x31000000`; `pillarbox_patches.c:52`; extended GBI scissor | Retain for panel/cinema content. Banjo's pillarbox is geometry, not DK64's reduced camera viewport. Cinema cropping must use Banjo's actual scissor/bars, not fixed DK64 row assumptions. |
| Bottles puzzle / SNS photograph | Framebuffer imagery and screen-space interaction, modes 8/A | Panel. Includes 3D models but isn't immersive gameplay. |
| Fog, near clipping, aspect ratio | Native fog; projection scale 0.5; recomp already disables near clipping/depth-clamps in `graphics_patches.c:120` | Renderer owns physical eye near/fog remap and runtime asymmetric FOV. Do not assume DK64 near 10 or FOV 45 applies to every frame. Retain normal aspect behavior off; panel/cinema uses native presentation. |
| Audio / collision / aiming / room-scale / comfort turn | Game camera/input remain native | This task adds no input/room-scale implementation. Use native game/controller paths; room-scale cannot just move the render eye. |

## Verification and reproduction

Verified levels: MIPS patch compile/link, N64Recomp code generation, native host tests and generated
MIPS execution tests. **Full game build, GPU eye/HUD capture, device install/run and worn acceptance
were not performed.** They belong to the renderer/device workstream. Scale, billboard orientation,
HUD extraction, cinema cropping, behind-head completeness and load/pause routing need that evidence.

Using the existing `dk64-vr-linux` image (clang/ld.lld 15, Linux ARM64):

```sh
# Build N64Recomp/RSPRecomp from this clone's pinned runtime if tools are absent.
cmake -S lib/N64ModernRuntime/N64Recomp -B build/vr-check/n64recomp -GNinja \
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=clang-15 -DCMAKE_CXX_COMPILER=clang++-15
cmake --build build/vr-check/n64recomp --target N64RecompCLI RSPRecomp -j6
cp build/vr-check/n64recomp/N64Recomp build/vr-check/n64recomp/RSPRecomp .

make -C patches CC=clang-15 LD=ld.lld-15 -j6
./N64Recomp patches.toml
./N64Recomp banjo.us.rev0.toml
CC=clang-15 CXX=clang++-15 bash tools/vr/test_game_side.sh
CC=clang-15 CXX=clang++-15 bash tools/vr/test_generated.sh
```

Run inside a container with only this clone mounted writable at `/src`. The minimum delivered
patch check regenerates `RecompiledPatches/`; the ordinary game generation is also used to obtain
original-ROM oracle functions for tests. RSPRecomp was built, but RSP generation/full game linking
was not needed for this game-side check.

Tests compile the actual host API/profile and execute actual selected N64Recomp functions:

- 100,000 writes with four concurrent readers; complete snapshot generations, host pointer import,
  active/force flag, profile and scene precedence (force flag tested in separate processes).
- Original-ROM vs patched VR-off traversal calls/bounds at 12 pitch/yaw combinations.
- Original-ROM vs patched VR-off far/near values at three look directions; VR far contains all corners.
- All three sprite entry points: original-ROM vs VR-off display-list bytes, output pointers and callee
  traces; behind-camera acceptance and retained radial distance/size limits when active.
- Existing demo AABB and sphere behavior off; both pass outside game frustum when active.
- Generated camera/frame reporting, 27 pitch/yaw/roll basis combinations, real publish import,
  scene-change logs, player absence and sky latch consumption; lens-flare early return.

The harness loads the original core overlays and patch binary, word-swaps their data like the
runtime, then executes generated instructions. Map/camera suppliers and downstream graphics
consumers are controlled callees; the plane/dot/coordinate math is generated from the ROM.
This checks patch branches and byte outputs, **not a complete gameplay/audio/RNG parity run**.
The generated harness passed AddressSanitizer (`VR_TEST_FLAGS='-g -fsanitize=address'`).
Neither tests nor docs embed ROM bytes or generated source. Tools/generated outputs and temporary
build products are not committed; scratch is removed at task end.

Final local check on 2026-10-06: 1,544 patch functions and 8,500 original game functions
regenerated; host/force tests and generated/ASan tests passed. Frozen-header diff is empty.
Artifact SHA256 (generated locally, uncommitted):

```text
patches.elf  58ba389f22c8672146542d31b00830a79477f7c02221612d85a1bcadddf7c8ce
patches.bin  1534fd973c72c31f980f5a29b8d2a459fc73387576e00ca6c9d1bdc39007c9c6
patches.c    f80830df3f45f5d3209562ddd05bcf657e763ffdb795243226d4a5fda154113d
```
