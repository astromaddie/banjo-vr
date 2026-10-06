// Banjo-Kazooie's renderer-side profile extension (vr_game_profile_ext.h).
#include "vr_game_profile_ext.h"

static const vr_game_profile_ext banjo_ext = {
    // HUD counters (PROJECTION_HUD_TRANSFORM_ID 0x1004), dialogue portraits (0x1100 + id), the
    // press-start/logo/game-over overlays (0x1003, 0x1005..0x1008) and the transition (0x1002)
    // are perspective projections after the camera: all HUD, never the eye camera.
    1,
    // No camera-heading patch yet: the right stick swings the game's own camera (C-left/right).
    0,
};

const vr_game_profile_ext *vr_game_profile_ext_get(void) {
    return &banjo_ext;
}
