#include "../include/vr_game_profile.h"

// Existing projection_transform_tagging / sky_transform_tagging IDs. Both are world
// perspectives; the sky uses a camera-centred model, not DK64's screen-space gradient.
static const uint32_t camera_groups[] = { 0x00001000, 0x00001001 };

const vr_game_profile *vr_game_profile_get(void) {
    static const vr_game_profile profile = {
        camera_groups, 2,
        138.0f, // Both normal Banjo model assets: vertex Y -7..131 at default scale 1.
        100.0f, // 138 units / 100 units per metre = 1.38 m; adjustable by the renderer.
        10.0f,  // Gameplay minimum; actual near is dynamic and in each snapshot.
        "Banjo"
    };
    return &profile;
}
