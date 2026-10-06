// Renderer-side extension of vr_game_profile.h (that header is frozen; this one is not).
// Defined per game in src/vr/game/<game>_profile_ext.c; a weak default (all off) lives in
// src/vr/game/profile_ext_default.c.
#ifndef VR_GAME_PROFILE_EXT_H
#define VR_GAME_PROFILE_EXT_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Billboard facing modes (RT64's VrBillboardMode).
#define VR_BILLBOARD_CYLINDRICAL 1  // turns about world +Y only, stays upright
#define VR_BILLBOARD_SPHERICAL 2    // faces the viewpoint completely
#define VR_BILLBOARD_MATCHING 3     // upright when the game built it upright, else spherical

// Model matrices in matrix groups first_id..last_id (inclusive) are camera-facing billboards.
typedef struct vr_billboard_group {
    uint32_t first_id;
    uint32_t last_id;
    int mode;
} vr_billboard_group;

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
    // Camera-facing billboards the game tags by matrix group: in the eye passes RT64 turns them to
    // face the point between the eyes instead of the flat game camera (setting billboard_facing).
    // NULL/0: none (DK64).
    const vr_billboard_group *billboard_groups;
    int billboard_group_count;
} vr_game_profile_ext;

const vr_game_profile_ext *vr_game_profile_ext_get(void);

#ifdef __cplusplus
}
#endif

#endif
