// True once the host has read the game state from the stand-in (src/vr/game/fallback_game_state.cpp)
// rather than from the game side: no scene reports, no camera, no pause.
#pragma once

#ifdef __cplusplus
extern "C" {
#else
#include <stdbool.h>
#endif

bool vr_game_state_is_stand_in(void);

#ifdef __cplusplus
}
#endif
