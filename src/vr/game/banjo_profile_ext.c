// Banjo-Kazooie's renderer-side profile extension (vr_game_profile_ext.h).
#include "vr_game_profile_ext.h"

// Camera-facing billboards. In VR the patches add a flag above every existing transform id
// (patches/transform_ids.h, all below 0x40000000) to the matrix groups of billboards: sprites of
// props and actors (notes, flowers, eggs, extra lives) and the models' yaw-only billboard nodes
// stay upright; particles, smoke projectiles, spells and the models' full billboard nodes face the
// viewpoint completely.
static const vr_billboard_group banjo_billboards[] = {
    { 0x40000000u, 0x7FFFFFFFu, VR_BILLBOARD_CYLINDRICAL },  // VR_BILLBOARD_CYLINDRICAL_ID_FLAG
    { 0x80000000u, 0xBFFFFFFFu, VR_BILLBOARD_SPHERICAL },    // VR_BILLBOARD_SPHERICAL_ID_FLAG
};

static const vr_game_profile_ext banjo_ext = {
    // HUD counters (PROJECTION_HUD_TRANSFORM_ID 0x1004), dialogue portraits (0x1100 + id), the
    // press-start/logo/game-over overlays (0x1003, 0x1005..0x1008) and the transition (0x1002)
    // are perspective projections after the camera: all HUD, never the eye camera.
    .perspective_hud_after_camera = 1,
    // vr_turn.c turns the free camera's native R-look target yaw and reports rejection.
    .takes_comfort_turn = 1,
    .reports_turn_applicability = 1,
    .billboard_groups = banjo_billboards,
    .billboard_group_count = (int)(sizeof(banjo_billboards) / sizeof(banjo_billboards[0])),
};

const vr_game_profile_ext *vr_game_profile_ext_get(void) {
    return &banjo_ext;
}
