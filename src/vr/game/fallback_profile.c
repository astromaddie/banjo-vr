// Stand-in for the game side's vr_game_profile_get() (src/vr/game/banjo_profile.c, written with
// the Banjo patches). Weak, so the real profile replaces it at link time; until it lands the
// renderer runs with these values. They are DK64's numbers except the camera groups, which are
// Banjo's (patches/transform_ids.h: PROJECTION_GAMEPLAY_TRANSFORM_ID and
// PROJECTION_SKYBOX_TRANSFORM_ID), so stereo works at all; player height and world scale are
// placeholders until the game side measures Banjo.
#include "vr_game_profile.h"

static const uint32_t fallback_camera_groups[] = { 0x00001000u, 0x00001001u };

static const vr_game_profile fallback_profile = {
    fallback_camera_groups,
    2,
    25.2f,  // DK64's model height in game units
    15.0f,  // DK64's default world_scale (units per metre)
    10.0f,  // DK64's camera near
    "DK",
};

__attribute__((weak)) const vr_game_profile *vr_game_profile_get(void) {
    return &fallback_profile;
}
