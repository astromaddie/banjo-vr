#include <atomic>
#include <cassert>
#include <cstring>
#include <cstdlib>
#include <thread>
#include <vector>
#include <iostream>
#include "../include/dk64vr_game_state.h"
#include "../include/vr_game_profile.h"
#include "../../../patches/vr_scene_rules.h"

std::atomic_bool exited{false};

int main(int argc, char **) {
    const bool forced = argc > 1;
    assert(dk64vr_is_active() == forced);
    dk64vr_set_active(1);
    assert(dk64vr_is_active() == 1);
    dk64vr_set_active(0);
    assert(dk64vr_is_active() == forced);
    std::vector<uint8_t> rdram(0x2000);
    recomp_context ctx{};
    recomp_vr_is_active(rdram.data(), &ctx);
    assert(ctx.r2 == forced);
    dk64vr_game_state state{};
    state.frame = 41; state.scene = DK64VR_SCENE_FIRST_PERSON;
    state.cam_eye[0] = 12.5f; state.player_yaw_rad = 1.25f; state.sky_dome = 1;
    std::memcpy(rdram.data() + 0x1000, &state, sizeof(state));
    ctx.r4 = static_cast<int32_t>(0x80001000);
    recomp_vr_publish_game_state(rdram.data(), &ctx);
    dk64vr_game_state got{};
    dk64vr_get_game_state(&got);
    assert(std::memcmp(&got, &state, sizeof(state)) == 0);
    dk64vr_publish_game_state(nullptr); dk64vr_get_game_state(nullptr);

    // Concurrent readers must see a complete generation, not a mixture of adjacent writes.
    state = {};
    dk64vr_publish_game_state(&state);
    std::atomic<bool> done{false};
    std::vector<std::thread> readers;
    for (int t = 0; t < 4; ++t) readers.emplace_back([&] {
        do {
            dk64vr_game_state copy;
            uint32_t words[19];
            dk64vr_get_game_state(&copy);
            std::memcpy(words, &copy, sizeof(words));
            for (auto word : words) assert(word == words[0]);
        } while (!done.load());
    });
    for (uint32_t gen = 1; gen <= 100000; ++gen) {
        uint32_t words[19];
        for (auto &word : words) word = gen;
        std::memcpy(&state, words, sizeof(words));
        dk64vr_publish_game_state(&state);
    }
    done.store(true);
    for (auto &reader : readers) reader.join();

    auto scene = [](int mode = GAME_MODE_3_NORMAL, int map = MAP_1_SM_SPIRAL_MOUNTAIN,
                    int level = LEVEL_B_SPIRAL_MOUNTAIN, int camera = CAMERA_TYPE_2_DYNAMIC,
                    int fp = 0, int state = BS_1_IDLE, int transition = 0) {
        return vr_scene_classify(mode, map, level, camera, fp, state, transition);
    };
    assert(scene() == VR_SCENE_GAMEPLAY);
    for (int fp = 1; fp <= 3; ++fp)
        assert(scene(3, 1, 12, 2, fp) == VR_SCENE_FIRST_PERSON);
    assert(scene(3, 1, 12, 2, 4) == VR_SCENE_GAMEPLAY);
    for (int camera : {1, 3, 4}) assert(scene(3, 1, 12, camera) == VR_SCENE_CUTSCENE);
    for (int action : {BS_44_JIG_JIGGY, BS_34_JIG_NOTEDOOR})
        assert(scene(3, 1, 12, 2, 0, action) == VR_SCENE_CUTSCENE);
    for (int action : {BS_9_EGG_HEAD, BS_A_EGG_ASS})
        assert(scene(3, 1, 12, 2, 0, action) == VR_SCENE_GAMEPLAY);
    assert(scene(3, MAP_7B_CS_INTRO_GL_DINGPOT_1, LEVEL_D_CUTSCENE) == VR_SCENE_CUTSCENE);
    for (int map : {MAP_1E_CS_START_NINTENDO, MAP_1F_CS_START_RAREWARE,
                    MAP_91_FILE_SELECT, MAP_83_CS_GAME_OVER_MACHINE_ROOM})
        assert(scene(3, map, LEVEL_D_CUTSCENE, 1) == VR_SCENE_MENU);
    for (int mode : {7, 8, 9, 10}) assert(scene(mode) == VR_SCENE_MENU);
    assert(scene(4, 1, 12, 3) == VR_SCENE_PAUSE);
    assert(scene(4, 1, 12, 3, 2, BS_44_JIG_JIGGY, 1) == VR_SCENE_TRANSITION);
    for (int mode : {1, 2}) assert(scene(mode) == VR_SCENE_TRANSITION);
    assert(scene(0) == VR_SCENE_MENU);
    const auto *profile = vr_game_profile_get();
    assert(profile->camera_matrix_group_count == 2);
    assert(profile->camera_matrix_groups[0] == 0x1000 && profile->camera_matrix_groups[1] == 0x1001);
    assert(profile->player_height_units == 138 && profile->default_world_scale == 100);
    assert(profile->game_near_units == 10 && std::strcmp(profile->player_name, "Banjo") == 0);
    std::cout << "game-side host imports, seqlock, scene rules and profile passed\n";
}
