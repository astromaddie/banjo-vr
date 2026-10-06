# RT64 eye submissions: resource ownership and device A/B

This change is **untested on the headset**. The Frame was not contacted. Host bookkeeping
tests and ARM64 builds do not prove stereo correctness or the expected CPU-time reduction.
Run the following procedure after the coordinator has installed and hash-checked the package.

The current base is Banjo `c65a8f1` and RT64 `87afea9`, including comfort turning, settings
schema 6, the save diagnostic and billboard facing. Compare both slot settings on this same
base; the original 7.3 ms wait measurement is a reference, not a measurement of this build.

## Recording and fences

`RT64_VR_EYE_SLOTS=3` is the default for a host that opts in (Banjo). Values 2–4 select that
many in-flight **display-frame** slots, each containing two separate eye recordings. `1`
selects the original implementation: wait after the first eye, defer the second eye until
the next frame (`RT64_VR_DEFER_WAIT=1`). Invalid values select 3. The switch is read at startup.
An older host, including the current DK64 host, defaults to the original implementation;
it needs the host changes described below before it can opt in safely. Raytracing also uses
the original implementation. With `RT64_VR` off or no VR host, the original flat path remains.
Slot mode also requires the existing workload queue to use native Vulkan queue 0, the
queue in Banjo's OpenXR graphics binding; otherwise it logs a fallback to 1. The Frame's
single-queue graphics family is the target. Check `native_queue=0` in the device A/B log.

For slots on, the workload thread:

1. Takes the predicted pose/weights once, as before. Checks older fences without blocking,
   retires completed timestamps/readbacks, and acquires the current recording slot.
2. Records/submits L using its slot's resources, restores the workload's original buffers,
   then records/submits R using a different slot. There is no completion wait between eyes.
3. Ends the XR frame with both submissions queued. At the next display frame, uses the next
   pair of slots. It waits only if an earlier frame's slot is still occupied when reused.

A single manager lock covers both recordings in slot mode, so a present operation cannot
insert a wait on the current left eye between L and R. The original path retains its original
per-pass locking. Pair-lock acquisition is outside eye timings and included in the main CPU frame.

The existing graphics queue owns every command list: creating additional RenderWorkers
would let plume distribute them among different native VkQueues. Shared render targets
remain ordered by that queue and their existing image barriers. The query pools, command
buffers, and fences belong to the recordings; timestamps are read only after their fence
signals, normally one or two display frames late, with no query-result wait flag. Optional
pass-marker labels are saved with their recording rather than overwritten by the next eye.

Drains remain for shared-image growth, resolution/mode/configuration changes, idle callers, shutdown,
and recycling a workload whose framebuffer-change textures are still sampled. A present
queue operation that writes/resizes/reads a shared game image also waits for submitted eye
fences before using another native queue. These are correctness gates, not steady per-eye
waits. Flat panel and probe submissions retain their existing synchronization.

## Resource inventory

| Resource written/consumed by an eye | Dependency | Ownership |
| --- | --- | --- |
| View/projection matrices and fog remaps | Eye | Slot staging + destination buffers |
| Screen positions, RSP fog/shaded colours, position-modify output | Eye | Slot computed outputs |
| Generated texcoords, world positions/normals/velocities | Model/look-at dependent; billboards share one display-frame orientation | Duplicated computed outputs in each recording slot |
| Current/previous/inverse-transpose model matrices | Interpolation and billboard facing toward the shared eye midpoint | Slot staging + destination buffers |
| Interpolated RDP tiles and look-at vectors | Display-frame weight | Slot staging + destination buffers |
| GPU tile mappings/texture indices | Renderer descriptor ordering, framebuffer operations | Slot staging + destination buffers |
| Positions/velocities/UVs/colours, all vertex indices, triangle/index data, viewport/fog/lights/look-at, RDP/render/extra parameters | Workload | Full slot copies; producer can recycle its original geometry buffers |
| Instance/render indices, frame uniforms, each framebuffer's mapped parameter buffer | Recording | Slot FramebufferRenderer buffers |
| Common/texture/framebuffer/RSP process/modify/world/normal-generation/test-Z descriptors; test-Z index output | Recording | Separate renderer/processors/output buffers per slot |
| Native framebuffer upload/history buffers and change counters/descriptors | CPU framebuffer reload | Slot NativeTarget map; storage reloads invalidate the input and do not synchronously read a modified-pixel counter |
| Scratch framebuffer changes and reinterpret-operation descriptors | Recording | Two slot change pools and a slot descriptor vector |
| Hidden-area mesh upload buffers and eye-copy descriptors | Eye/recording | Slot hidden-mask/copier helpers |
| 4×4 dummy colour/depth attachments and cached framebuffers | Recording | Slot renderer/manager; these small images are duplicated |
| Timestamp pool, optional pass timestamp pool, command list and fence | Submission | Separate per recording |
| Host foveation rate image/upload and companion framebuffer | Eye/recording | Rate maps keyed by command list; framebuffer cache belongs to the slot |
| Eye/HUD diagnostic readbacks | Submission | Allocated only for requested dumps; slot buffers mapped after eye completion |
| Texture-cache images, replacement textures, dummy TLUT, immutable shaders/pipelines/samplers | Eye-independent | Shared; existing upload synchronization and a texture-cache lock for each pending submission |
| Render-target copy/resolve/write descriptors | Stable source image | Shared; bound once per source texture, destroyed only after a growth/configuration drain |
| Main/offscreen colour, depth, tile-copy and HUD black/white images; right-eye colour target | GPU-ordered writes | Shared; drain before replacing attachments; cached framebuffers and source bindings invalidated |
| Workload framebuffer-change input images | Workload | Shared until every slot using that workload has retired, before releasing its producer barrier |
| HUD solve descriptor | Stable black/white inputs | Shared; input binding invalidated on growth after draining |

Uploader **threads** and CPU projection/transform/tile/look-at processors are shared. CPU
upload work finishes before the buffer swap is restored; recorded commands refer directly
to the slot's staging/destination resources. The slot patches do not change processor
implementations or plume source. The RT64 base adds billboard processing: projection stores
the viewpoint after `eyeCamera`, then `TransformProcessor::faceBillboards` rewrites the
interpolated current/previous matrices and inverse-transposes before uploading them. The
draw/output buffer swap is already active at that point, so those uploads go to the acquired
slot's `worldTransformsBuffer`, `prevWorldTransformsBuffer` and `invTWorldTransformsBuffer`;
the slot's RSP/vertex descriptors consume the same buffers. CPU upload readiness completes
before restoring the swap. Banjo's host supplies the same midpoint viewpoint for both eyes,
so the model orientation is per display frame although each eye recording owns a copy.
The billboard transform-processing condition is preserved even without a matched frame.

## Memory and timing interpretation

Six recordings are retained with the default of three frame slots. Geometry inputs are
duplicated even where eye-neutral to avoid coupling their lifetime to the producer's four
workloads. This trades more upload traffic and memory for simpler ownership. The large
2160² colour/depth/HUD render targets are shared rather than copied six times.

`slot_buf_mib` reports the ring's allocated staging + destination + computed + renderer
uniform/index + native-history buffers, including RT64's usual 1.5× allocation growth and
256-byte rounding. It is additional to the original workload buffers. It excludes descriptor
pool/command/pipeline driver allocations, hidden-mask buffers, framebuffer-change images,
foveation images, and optional diagnostic readbacks. An indicative vertex-only cost is about
300 bytes per vertex per recording, or about 34 MiB for six recordings at 20,000 vertices,
before indices, matrices, tiles and other data. This is an estimate, not a Banjo measurement.
At 2160², each requested eye readback adds 17.8 MiB; six live dump slots can add 106.8 MiB.
HUD dumps use the host's half-size 1080² image: another 4.45 MiB per used left-eye slot,
or 13.35 MiB across three left-eye slots. Dump buffers persist until session teardown.
Foveation's R8 maps/staging are small (about 36 KiB per recording at a 16×16 rate texel).
Record the measured `slot_buf_mib` and process RSS with the device results.

The `[vr-perf] eyes` line keeps `gpu`, `copy`, `rec`, `submit`, and `wait` and adds:

- `slots`: configured frame-slot count (1 means the original path).
- `reuse`: milliseconds spent waiting for an occupied older recording slot, per eye.
- `drain`: fence wait during an exceptional shared-image growth in that eye's recording.
- `gpu_age`: slot-mode display frames since submission when this GPU sample was collected
  (the original path reports 0).
- `slot_buf_mib`: the buffer allocation count described above.

Each completed eye contributes once to the eye averages. CPU and GPU data in those averages
belong to the same recording. `rec` includes CPU upload readiness and any growth drain inside
recording, but excludes the earlier slot acquisition wait. `wait` is zero for both slot eyes;
moving a wait into `reuse`, or shifting it into frame/mode/present work, is not a performance
win. The main `[vr-perf]` CPU `frame` stage is the authoritative total. `drain` does not count
out-of-eye mode/configuration/workload/present synchronization. GPU figures in the main
frame summary are the latest completed samples rather than blocking queries of the current
frame. Exclude dump windows when comparing performance.

## Unattended A/B: three runs per setting

Prerequisites: BanjoVR's per-app SteamVR setting is **120 Hz** and **2160** resolution; a valid
US ROM and the same Spiral Mountain save are in the title's persistent data folder. Check
`rate=120.0Hz`, `eye=2160x2160`, `mode=stereo`, `foveation=high`, and the scene log's gameplay
state in every accepted window. Do not accept cinema/menus/probes or a different save/scene.
The refresh-rate environment value cannot substitute for the per-app Steam setting.

The one-shot `vr-diag` lines for the baseline are:

```text
ARGS=--game bk
RT64_VR_EYE_SLOTS=1
RT64_VR_DEFER_WAIT=1
BK_VR_FORCE_RENDER=1
BK_VR_TIMING=1
BK_VR_RESOLUTION_SCALE=auto
BK_VR_WORLD_SCALE=100
BK_VR_FOVEATION=high
BK_VR_FOVEATION_EYE_TRACKED=1
BK_VR_BILLBOARD_FACING=1
BK_VR_DIAG_SAVE_IN_PLAY=0
BK_VR_DIAG_TURN_STICK=0
BK_VR_DIAG_AUTOSTART=360
BK_VR_DIAG_EXIT_SECONDS=410
BK_VR_DIAG_YAW=0
BK_VR_DUMP_FRAME=0
BK_VR_DUMP_PANEL_FRAME=0
BK_VR_DUMP_FOVEATION=0
BK_VR_DIAG_VR_MENU_AT=0
BK_VR_DIAG_MENU_AT=0
BK_VR_DIAG_START_AT=0
```

The slots-on run changes **only** `RT64_VR_EYE_SLOTS=3`. No TU/GPU-pass tracing or eye dumps in
these six timing runs. The helper below writes those lines, launches through Steam, and
fetches logs. Book the Frame using the coordinator's normal `frame-lock.sh` before executing
anything here; these are instructions for the coordinator, not device actions taken by this
worker. Alternate configurations to limit warm-up/thermal bias:

```sh
for run in 1 2 3; do
  for slots in 1 3; do
    tools/steam_frame/unattended.sh 420 "evidence/eye-wait/slots${slots}-run${run}" \
      "RT64_VR_EYE_SLOTS=$slots" RT64_VR_DEFER_WAIT=1 \
      force_render=1 timing=1 resolution_scale=auto world_scale=100 \
      foveation=high foveation_eye_tracked=1 billboard_facing=1 \
      diag_save_in_play=0 diag_turn_stick=0 diag_autostart=360 \
      diag_exit_seconds=410 diag_yaw=0 dump_frame=0 dump_panel_frame=0 \
      dump_foveation=0 diag_vr_menu_at=0 diag_menu_at=0 diag_start_at=0
  done
done
```

For each run retain the diag echo, `[vr-slots]`, scene logs, and all `[vr-perf]` lines.
Compare at least the final six consecutive 10-second **gameplay** windows after the first
two gameplay windows have warmed up. If this save takes too long to reach gameplay, repeat
with both configurations using a longer identical duration; do not compare differing scenes.

The expected win is `wait=L/R` falling from the supplied baseline's roughly 7.3/0 ms to
0/0, `reuse` and steady `drain` staying near zero, and total CPU frame dropping below the
8.33 ms budget (target roughly 1.5–3 ms of recording). FPS should approach 120 rather than
60, missed frames should fall, and the summed GPU + copy times should remain comparable.
The duplicated uploads can increase `rec` and GPU setup time: report them rather than
assuming the target was reached. Preserve the per-run means/ranges and outliers; report the
three runs separately before combining them. Do not infer hardware acceptance from these
instructions or from a source/build pass.

## Visual corruption checks (separate from timing)

After performance runs, capture both settings with the same additional diagnostic options.
Use one run per setting with `dump_in_play=1 dump_frame=600 dump_count=6 dump_interval=1
diag_yaw=0 diag_yaw_step=0`, leaving the other A/B options the same. Six consecutive eye
pairs exercise all default recording slots and their reuse. Run another pair with
`dump_count=3 dump_interval=120 diag_yaw_step=90` for yaw 0/90/180. An additional slots-on
run with `RT64_VR_EYE_ORDER=swap` checks pass-order independence.

Compare the left/right pair at each capture and consecutive frames: consistent parallax
direction, no eye duplication/reversal, no vertices from the other eye, no flicker or
alternating stale positions/colours/UVs/fog, and stable house/fence/cliff/sky/HUD geometry.
Head-turn views must retain geometry behind the game camera. Eye differences should come
from parallax, occlusion and the expected per-eye foveation/mask; animated content and timing
prevent requiring identical hashes between separate runs. Verify that the HUD solve/dumps
are correct and absent from the eye images, and test gameplay ↔ cinema/panel transitions,
pause/menu, return to play, and clean exit. These diagnostics must not deadlock and must
not lose or overwrite a requested PNG when multiple recordings are pending.

## Host evidence

Verified on 2026-10-06 with image `dk64-vr-linux`, ARM64 Release, clang 15.0.7, four jobs.
Rebased executable source: superproject `8c05d9ad918d1f88bc254e51b299944117c06b49`, RT64
`8ad0ea0526b4454b98ba095e33e1d3b6fc3b7f77`. Subsequent test-harness/documentation commits
do not change executable inputs; final package metadata records the packaging HEAD.

RT64 rebased without conflicts onto `87afea9`: `279eab3` became `dcce60a`, and `a1335d3`
became `8ad0ea0`; `git range-diff` reports both patches unchanged. Superproject conflicts
were in the RT64 gitlink (selected the corresponding rebased test/core commits),
`src/vr/tests/CMakeLists.txt` and `tools/docker/container-steps.sh` (retained both comfort-turn
and slot suites and the four-job limit), and `docs/VR_ARCHITECTURE.md` (retained the new
billboard/comfort status and the slot row). The XR host merged automatically, retaining
comfort-turn and billboard callbacks together with slot foveation/readback/telemetry ownership.

- `BUILD_DIR=build-linux-aarch64-vr JOBS=4 tools/docker/build-linux.sh vrtest`: all suites
  passed, including the production slot-ring header (24,011 checks), bridge/billboard tests
  (9,250 checks, zero failures), Banjo profile tests (zero failures), comfort-turn tests
  (1,420 checks, zero failures), and the one-shot diagnostic launcher suite.
- `tools/vr/test_game_side.sh` and `tools/vr/test_generated.sh` in the build image: passed
  game-side imports/profile checks, actual generated-MIPS gates, original-ROM flat parity,
  camera/frame snapshots, native turn/follower, water/flight retention and rejection checks.
  The new save diagnostic initially exposed a missing generated-harness callee. The extractor
  now includes the actual `vr_diag_save_tick` output, with controlled save-game callees;
  assertions cover default-off, interrupted delay and exactly one save. Existing assertions
  were retained. The temporary decompressed ROM was hash-checked and removed after testing.
- `BUILD_DIR=build-linux-aarch64-vr BK_VR=0 JOBS=4 tools/docker/build-linux.sh build`:
  passed on the previous base, flat ELF build ID `3d8121293e03aa2b813a14bb4c610669dca77b88`.
  The rebased VR-off build was not repeated; this previous build is not evidence for the
  combined new base. No second build tree was created.
- `BK_ROM="/Users/mads/Downloads/Banjo-Kazooie (USA).z64" BK_VR=1 JOBS=4
  tools/docker/build-linux.sh build`: passed. Final VR ELF build ID
  `ab7cdf2ea9b2cbe3300b9492492a0f694fcb8537`.
- `BK_VR=1 JOBS=4 tools/docker/build-linux.sh package` and `abi`: passed. Package is
  `build-linux-aarch64-vr/package/BanjoRecompiled/` (24 MiB); packaged executable SHA-256
  `e2d88f64a6c71d9d65b5f7967afb40f68574f96b8670dbfc459789397bd864ba`.
  ABI limits passed: executable GLIBC 2.35, bundled SDL 2.34, atomic 2.17, no GLIBCXX.
- Packaged VR executable with `--game bk`, Xvfb, lavapipe and dummy audio: remained running
  for 45 seconds (expected timeout status 124). The US ROM was SHA-1 checked, mounted read-only,
  copied only into a temporary container data directory, and removed with that directory.
  Log confirms `no OpenXR runtime; VR mode off`, llvmpipe initialization, and Banjo startup.
  The software driver warns about unsupported `OpGroupNonUniformAllEqual`; no assertion or
  crash occurred before timeout. This covers game startup and the VR-inactive flat renderer,
  not headset stereo or player-visible acceptance.

Rebase logs are retained in the existing `build-linux-aarch64-vr/` tree:
`eye-slots-rebase-tests.log`, `eye-slots-rebase-vr-build.log`, `eye-slots-rebase-package.log`,
`eye-slots-rebase-abi.log`, `eye-slots-rebase-smoke.log`, `eye-slots-rebase-game-tests.log`,
`eye-slots-rebase-generated-tests.log`, and `eye-slots-rebase-rom-test.log`. Earlier logs,
including `eye-slots-vulkan-info.log`, are retained too. Test binaries and ROM scratch were
removed after preserving the logs. Package metadata reports a dirty tree because the
pre-existing nativefiledialog pointer and bk-decomp line-ending change were preserved,
not committed.

`vulkaninfo` lists only `VK_LAYER_MESA_device_select` and `VK_LAYER_MESA_overlay`; Khronos
validation is unavailable. No DK64 full build was run; its host remains on the legacy path
through the default opt-in implementation. Device A/B, dynamic foveation and stereo GPU
corruption checks remain coordinator work. No push or device action was performed.

RT64 paths changed by the slot commits relative to `87afea9` (relative to `lib/rt64`):

```text
CMakeLists.txt
src/hle/rt64_framebuffer.cpp
src/hle/rt64_framebuffer_manager.cpp
src/hle/rt64_present_queue.cpp
src/hle/rt64_vr.h
src/hle/rt64_vr_hud.h
src/hle/rt64_vr_eye_resources.cpp
src/hle/rt64_vr_eye_slots.h
src/hle/rt64_workload_queue.cpp
src/hle/rt64_workload_queue.h
src/render/rt64_render_target.cpp
src/render/rt64_render_worker.h
src/tests/rt64_vr_eye_slots_test.cpp
```
