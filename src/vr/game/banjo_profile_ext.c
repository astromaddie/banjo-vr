// Banjo-Kazooie's renderer-side profile extension (vr_game_profile_ext.h).
#include "vr_game_profile_ext.h"

static const vr_game_profile_ext banjo_ext = {
    // HUD counters (PROJECTION_HUD_TRANSFORM_ID 0x1004), dialogue portraits (0x1100 + id), the
    // press-start/logo/game-over overlays (0x1003, 0x1005..0x1008) and the transition (0x1002)
    // are perspective projections after the camera: all HUD, never the eye camera.
    1,
    // vr_turn.c turns the free camera's native R-look target yaw and reports rejection.
    1,
    1,
};

const vr_game_profile_ext *vr_game_profile_ext_get(void) {
    return &banjo_ext;
}
