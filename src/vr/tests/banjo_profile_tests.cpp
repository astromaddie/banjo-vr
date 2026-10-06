// The shared VR layer with Banjo's profile (src/vr/game/banjo_profile.c and banjo_profile_ext.c):
// world_scale default and range, the World size text, the right stick default.
#include <cmath>
#include <cstdio>
#include <string>

#include "dk64vr_settings.h"
#include "dk64vr_menu.h"
#include "vr_game_profile_ext.h"

static int failures = 0;
#define CHECK(c) do { if (!(c)) { std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #c); failures++; } } while (0)

int main() {
    using namespace dk64vr;
    dk64vr_settings_t s;
    dk64vr_settings_defaults(&s);
    const int i = dk64vr_settings_index("world_scale");
    CHECK(i >= 0);
    const sfvr_setting_key *k = dk64vr_settings_key(size_t(i));
    CHECK(std::fabs(dk64vr_settings_get(&s, "world_scale") - 100.0) < 1e-9);
    CHECK(std::fabs(k->min - 100.0 * 4.0 / 15.0) < 1e-6 && std::fabs(k->max - 100.0 * 100.0 / 15.0) < 1e-6);
    CHECK(std::fabs(dk64vr_settings_dk_height(&s) - 1.38) < 1e-9);
    CHECK(menu::dkHeightText(100.0) == "Banjo 1.4 m");
    CHECK(dk64vr_settings_get(&s, "right_stick_x") == 0.0);  // native comfort-turn camera patch
    CHECK(dk64vr_settings_get(&s, "snap_angle") == 45.0);
    CHECK(dk64vr_settings_get(&s, "turn_mode") == 0.0);
    // The presets include the default and stay inside the range (0.21 m to 5.2 m).
    const std::vector<double> presets = menu::dkHeightPresets(k->min, k->max);
    bool hasDefault = false;
    for (double h : presets) {
        hasDefault = hasDefault || (std::fabs(h - 1.38) < 1e-9);
        CHECK(h > 0.2 && h < 5.2);
    }
    CHECK(hasDefault);
    CHECK(std::fabs(menu::stepWorldScale(100.0, 1, k->min, k->max) - 138.0 / 1.4) < 1e-6);
    // Billboards: the patches' flags (patches/transform_ids.h) over every existing id.
    const vr_game_profile_ext *ext = vr_game_profile_ext_get();
    CHECK(ext->billboard_group_count == 2);
    // The pause menu goes to the HUD image over the frozen world (pause_menu_patches.c).
    CHECK(ext->pause_menu_hud == 1);
    auto modeOf = [&](uint32_t id) {
        for (int g = 0; g < ext->billboard_group_count; g++) {
            if ((id >= ext->billboard_groups[g].first_id) && (id <= ext->billboard_groups[g].last_id)) {
                return ext->billboard_groups[g].mode;
            }
        }
        return 0;
    };
    CHECK(modeOf(0x00001000u) == 0);                              // camera
    CHECK(modeOf(0x31000010u) == 0);                              // highest plain id
    CHECK(modeOf(0x40000000u | 0x20000100u) == VR_BILLBOARD_CYLINDRICAL);  // prop sprite
    CHECK(modeOf(0x40000000u | 0x10000100u) == VR_BILLBOARD_CYLINDRICAL);  // actor sprite
    CHECK(modeOf(0x80000000u | 0x01000005u) == VR_BILLBOARD_SPHERICAL);    // particle
    CHECK(modeOf(0x80000000u | 0x31000010u) == VR_BILLBOARD_SPHERICAL);
    CHECK(modeOf(0xFFFFFFFFu) == 0);                              // G_EX_ID_AUTO
    std::printf("banjo profile: %d failures\n", failures);
    return failures ? 1 : 0;
}
