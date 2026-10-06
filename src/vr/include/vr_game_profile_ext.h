// Renderer-side extension of vr_game_profile.h (that header is frozen; this one is not).
// Defined per game in src/vr/game/<game>_profile_ext.c; a weak default (all off) lives in
// src/vr/game/profile_ext_default.c.
#ifndef VR_GAME_PROFILE_EXT_H
#define VR_GAME_PROFILE_EXT_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct vr_game_profile_ext {
    // Nonzero: perspective projections drawn into the main colour image after the world camera
    // and outside the camera groups are HUD (drawn into the HUD image with their own projection,
    // never given the eye camera). Banjo draws its HUD counters through such a projection
    // (0x1004), and its dialogue portraits (0x1100 + id) and title overlays too. DK64: 0.
    int perspective_hud_after_camera;
    // Nonzero: the game turns its camera heading through dk64vr_take_turn (a camera patch), so
    // the right stick's comfort turn works. Zero: the right stick defaults to the game's own
    // C-left / C-right camera swing (right_stick_x = camera).
    int takes_comfort_turn;
} vr_game_profile_ext;

const vr_game_profile_ext *vr_game_profile_ext_get(void);

#ifdef __cplusplus
}
#endif

#endif
