// Stand-in for the game side's half of the VR interface (src/game/vr_api.cpp, written with the
// Banjo patches): every function of dk64vr_game_state.h and dk64vr_control.h, weak, so the real
// ones replace them at link time.
//
// Without scene reports the stand-in calls every frame gameplay (so the renderer goes stereo
// whenever a frame carries the camera projection) and has no camera, player or pause: the
// standby pause cannot hold the game thread, and comfort turns never reach the game camera.
// vr_game_state_is_stand_in() tells the host which it has, for the logs and the autostart.

#include <atomic>
#include <chrono>
#include <cstring>

#include "dk64vr_game_state.h"
#include "dk64vr_control.h"
#include "vr_game_state_stand_in.h"

namespace {
std::atomic<int> g_active{0};
std::atomic<int> g_paused{0};
std::atomic<bool> g_stand_in_used{false};
std::atomic<uint32_t> g_frame{0};
}

extern "C" bool vr_game_state_is_stand_in(void) {
    return g_stand_in_used.load(std::memory_order_relaxed);
}

extern "C" __attribute__((weak)) void dk64vr_get_game_state(dk64vr_game_state *out) {
    g_stand_in_used.store(true, std::memory_order_relaxed);
    std::memset(out, 0, sizeof(*out));
    out->frame = g_frame.fetch_add(1, std::memory_order_relaxed) + 1;
    out->scene = DK64VR_SCENE_GAMEPLAY;
    out->cam_up[1] = 1.0f;
}

extern "C" __attribute__((weak)) void dk64vr_publish_game_state(const dk64vr_game_state *) {}

extern "C" __attribute__((weak)) int dk64vr_is_active(void) {
    return g_active.load(std::memory_order_relaxed);
}

extern "C" __attribute__((weak)) void dk64vr_set_active(int active) {
    g_active.store(active != 0, std::memory_order_relaxed);
}

extern "C" __attribute__((weak)) void dk64vr_set_paused(int paused) {
    g_paused.store(paused != 0, std::memory_order_relaxed);
}

extern "C" __attribute__((weak)) int dk64vr_is_paused(void) {
    return g_paused.load(std::memory_order_relaxed);
}

extern "C" __attribute__((weak)) void dk64vr_request_turn(int) {}

extern "C" __attribute__((weak)) int dk64vr_take_turn(void) {
    return 0;
}

extern "C" __attribute__((weak)) void dk64vr_request_hud_diag(void) {}

extern "C" __attribute__((weak)) int dk64vr_take_hud_diag(void) {
    return 0;
}
