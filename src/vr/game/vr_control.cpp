// The host's requests to the game (dk64vr_control.h), outside the frozen game-state contract:
// the standby/menu pause flag (the game thread waits on it in recomp_vr_publish_game_state,
// src/game/vr_api.cpp), comfort-turn degrees for a camera patch to take, and the HUD diagnostic.
// Banjo has no camera-heading patch yet, so turns are never taken (profile ext:
// takes_comfort_turn 0) and requests expire after 0.3 s.
#include <atomic>
#include <chrono>
#include <cstdio>
#include <mutex>

#include "../include/dk64vr_control.h"

namespace {
std::atomic<int> g_paused{0};
std::mutex g_turn_mutex;
int g_turn_degrees = 0;
std::chrono::steady_clock::time_point g_turn_time;
std::atomic<int> g_hud_diag{0};
}

extern "C" void dk64vr_set_paused(int paused) {
    g_paused.store(paused != 0 ? 1 : 0, std::memory_order_relaxed);
}

extern "C" int dk64vr_is_paused(void) {
    return g_paused.load(std::memory_order_relaxed);
}

extern "C" void dk64vr_request_turn(int degrees) {
    std::lock_guard<std::mutex> lock(g_turn_mutex);
    g_turn_degrees += degrees;
    g_turn_time = std::chrono::steady_clock::now();
}

extern "C" int dk64vr_take_turn(void) {
    std::lock_guard<std::mutex> lock(g_turn_mutex);
    const int degrees = g_turn_degrees;
    g_turn_degrees = 0;
    if (degrees == 0) {
        return 0;
    }
    if ((std::chrono::steady_clock::now() - g_turn_time) > std::chrono::milliseconds(300)) {
        std::printf("[vr] turn: %d deg dropped (not taken within 0.3 s)\n", degrees);
        return 0;
    }
    return degrees;
}

extern "C" void dk64vr_request_hud_diag(void) {
    g_hud_diag.store(1);
}

extern "C" int dk64vr_take_hud_diag(void) {
    return g_hud_diag.exchange(0);
}
