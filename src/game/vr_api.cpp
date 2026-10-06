#include <atomic>
#include <cstdint>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <chrono>
#include <mutex>
#include <thread>

#include "recomp.h"
#include "librecomp/helpers.hpp"
#include "../vr/include/dk64vr_game_state.h"
#include "../vr/include/dk64vr_control.h"
#include "../../patches/vr_state.h"

#define CHECK_SCENE(name) static_assert(VR_SCENE_##name == DK64VR_SCENE_##name, "scene ABI drift")
CHECK_SCENE(UNKNOWN); CHECK_SCENE(GAMEPLAY); CHECK_SCENE(FIRST_PERSON); CHECK_SCENE(CUTSCENE);
CHECK_SCENE(TRANSITION); CHECK_SCENE(MENU); CHECK_SCENE(PAUSE); CHECK_SCENE(ARCADE);
CHECK_SCENE(MULTIPLAYER); CHECK_SCENE(COUNT);
#undef CHECK_SCENE
static_assert(sizeof(dk64vr_game_state) == 76 && offsetof(dk64vr_game_state, sky_dome) == 72,
              "frozen snapshot ABI");

namespace {
constexpr size_t kWords = sizeof(dk64vr_game_state) / sizeof(uint32_t);
std::atomic<uint32_t> sequence{0};
std::atomic<uint32_t> snapshot[kWords]{};
std::mutex writer_mutex;
std::atomic<int> active{0};

bool force_patches() {
    static const bool forced = [] {
        const char *value = std::getenv("BK_VR_FORCE_PATCHES");
        return value && value[0] && value[0] != '0';
    }();
    return forced;
}
}

extern "C" void dk64vr_publish_game_state(const dk64vr_game_state *state) {
    if (!state) return;
    uint32_t words[kWords];
    std::memcpy(words, state, sizeof(words));
    std::lock_guard<std::mutex> lock(writer_mutex);
    // Sequentially consistent atomic words make the seqlock well-defined in C++ even
    // during concurrent copies. A reader accepts only one complete, even generation.
    sequence.fetch_add(1);
    for (size_t i = 0; i < kWords; ++i) snapshot[i].store(words[i]);
    sequence.fetch_add(1);
}

extern "C" void dk64vr_get_game_state(dk64vr_game_state *out) {
    if (!out) return;
    uint32_t words[kWords];
    for (;;) {
        const uint32_t before = sequence.load();
        if (before & 1) continue;
        for (size_t i = 0; i < kWords; ++i) words[i] = snapshot[i].load();
        if (sequence.load() == before) break;
    }
    std::memcpy(out, words, sizeof(words));
}

extern "C" int dk64vr_is_active(void) {
    return active.load(std::memory_order_relaxed) || force_patches();
}
extern "C" void dk64vr_set_active(int value) {
    active.store(value != 0, std::memory_order_relaxed);
}
// The runtime's quit flag (librecomp/src/recomp.cpp), so a paused game never holds up a quit.
extern std::atomic_bool exited;

extern "C" void recomp_vr_is_active(uint8_t *rdram, recomp_context *ctx) {
    (void)rdram;
    _return(ctx, dk64vr_is_active());
}
extern "C" void recomp_vr_publish_game_state(uint8_t *rdram, recomp_context *ctx) {
    // RDRAM is word-swapped: aligned 32-bit ints/floats are already host-endian.
    dk64vr_game_state state;
    std::memcpy(&state, _arg<0, const dk64vr_game_state *>(rdram, ctx), sizeof(state));
    dk64vr_publish_game_state(&state);

    // While the VR host pauses the game (headset off, focus lost, the VR settings menu:
    // dk64vr_control.h), the game thread waits here, once a frame, outside any game lock. Never
    // set in a flat build. A quit still goes through.
    while (dk64vr_is_paused() && !exited.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
}
