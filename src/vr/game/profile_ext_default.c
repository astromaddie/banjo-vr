// The extension's defaults (vr_game_profile_ext.h): DK64's behaviour. Weak, so a game's own
// <game>_profile_ext.c replaces it.
#include "vr_game_profile_ext.h"

static const vr_game_profile_ext default_ext = { 0, 1 };

__attribute__((weak)) const vr_game_profile_ext *vr_game_profile_ext_get(void) {
    return &default_ext;
}
