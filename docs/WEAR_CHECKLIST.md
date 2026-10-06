# BanjoVR wear checklist (about 10 minutes)

First wear of Banjo VR. Everything below is unworn; unattended runs only proved the pictures and
the logs. No real controller press has reached the game yet.

**Before you put it on**

- Build: `steam-frame` b4f1468 (docs ec352b8), installed as the `BanjoVR` devkit title (shortcut
  appid 2393272997, binary md5 `1d448b6e00c6d25066e7b7197b7482f2`), nothing armed, no `vr.toml`
  yet (defaults: Banjo 1.38 m, foveation high, camera smoothing medium).

- SteamVR per-app settings for **BanjoVR** (in the headset): refresh **120 Hz**, resolution
  **2160** (the same as DK64VR). Without them it runs at 72 Hz.
- Launch **BanjoVR** from the Steam library in the headset (not over SSH: no controllers then).
- No save exists yet: a new file plays the long story intro in the cinema first (the game
  lets you skip it with Start only once a save exists).

**1. Launcher and boot (1 min).** The Banjo: Recompiled launcher sits on a panel 2 m ahead, no
black box in front of it. Point with the right hand: beam and dot, never hidden behind the panel.
Click Start Game with the trigger (or press A). The Rareware/Nintendo logos, the title and the
file select stay on the flat panel.

**2. Intro or file (1 min).** A new file's intro plays on the 3D cinema screen 3 m ahead: does the
picture have depth behind the screen, and does the dialogue box (with Grunty's portrait) sit on
the HUD in front, legible? The screen stays put when you turn your head. Advance the dialogue with A
or B; it takes a few minutes.

**3. Spiral Mountain (4 min).** Stereo third person behind Banjo.
- Size: does Banjo look about 1.4 m tall, the house and the cliffs a believable size? (VR menu >
  World size changes it.)
- Left stick: Banjo walks where you look. A jumps, X attacks, LT crouches, RT centres the camera,
  Y and B swing the game camera left and right, RB first person (look around; RB again to leave),
  LB zooms. Right stick left/right also swings the game camera (there is no comfort snap turn
  yet). Note any button that does not do what the game says.
- Turn your head fully around: any missing scenery, holes or popping behind you?
- The honeycomb/lives HUD: 1.5 m ahead, follows your head without rolling, readable, in front of
  everything, and no copy of it left in the world?
- Sprites (flowers, grass tufts, sparkles): do any show their edge when you look from the side?
  (Expected: they still face the game camera, not you.)

**4. Pause and menus (2 min).** Menu pauses: the world freezes, dimmed, with the pause menu on the
panel. View (short) opens the recomp menu the same way; close with View. Hold Menu and View
together for half a second: the VR settings quick menu; change World size and close with B.
Hold View for 1 s: buzz and recentre behind Banjo.

**5. Focus (30 s).** Lift the headset off for a few seconds: on return the game should have
waited (not run on without you).

**6. Answers.** Banjo's size, HUD distance and size, cinema depth, camera motion comfort (the
game camera's swings and its auto-follow), any wrong button, any discomfort. Quit from
SteamVR's own menu. The log: `tools/steam_frame/deploy.sh --logs`
(`FRAME_HOST=steamos@192.168.1.31`).
