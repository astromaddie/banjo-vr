# Changelog

## 2026-10-14 — Phosphor mask

- Added `phosphor` and `phosphor_size` (VR settings, Display: "CRT phosphors", "Phosphor size"): red, green and blue stripes and scanlines over the world, the same for both eyes. On at 0.5 by default. Not yet run on the Frame in this port.
- The RT64 VR overlay patch and its pinned hash changed for the new eye shader.

## 2026-10-13 — Manual camera setting

- Added `camera_auto_rotate` (VR settings, Comfort: "Automatic camera turning"). Off keeps Banjo's free orbit camera, so the view turns only from the right stick, C buttons or R; camera collision is the game's own. Not worn.

## 2026-10-11 — PR0 readiness preparation

- Added experimental Steam Frame status, readiness evidence and a public ROM-free VR host-test workflow.
- Replaced the private RT64 pin with its public upstream base plus SHA-256-pinned RT64/Plume VR source overlays.
- Scoped the upstream AI policy to upstream submissions and disclosed Codex-assisted fork work.
- No public build or release was made. Full native builds still require a locally supplied game ROM.

## 2026-10-11 — Current-source PR0 verification

- Updated the tested source to `2e3c67cd23502bb06943a8ddd8dc199faa7645a5`; ROM-free production-source CMake/CTest passed 4/4 locally with public pinned dependencies.
- The latest exact-source GitHub job did not start because of the account billing/spending-limit block; the earlier successful run is retained in PR0_METADATA.json.
- No full ARM64 application build was run because the scoped project/build inventory contained no supported game ROM.

## 2026-10-11 — Public experimental source

- Made the repository public and confirmed the ROM-free GitHub validation passes without private secrets or game data.
