#include "patches.h"
#include "functions.h"
#include "vr_state.h"

// Diagnostic (setting diag_save_in_play, env BK_VR_DIAG_SAVE_IN_PLAY=1): once in play for a few
// seconds, save the current game once through the routine the pause menu's SAVE AND QUIT calls
// (gameSelect_saveAndExit: gameFile_save, then gameFile_8033CFD4, which writes the EEPROM image
// and so the host's save file). The saved file is no longer empty, so the next launch's file
// select skips the new-game intro and starts in Spiral Mountain (gameSelect_update, GAME_SELECT_STARTING).
// The host import reports 0 unless the VR host set the setting, so the flat game and an ordinary
// VR run never get past the first test.

extern void gameSelect_saveAndExit(void);
extern s32 gameSelect_getGameNumber(void);
s32 map_get(void);
s32 level_get(void);

// Seconds of uninterrupted play before the save.
#define VR_DIAG_SAVE_DELAY_S 4.0f

void vr_diag_save_tick(s32 scene) {
    static s32 done;
    static f32 play_seconds;
    s32 level;

    if (done || !recomp_vr_diag_save_in_play()) {
        return;
    }
    if (scene != VR_SCENE_GAMEPLAY) {
        play_seconds = 0.0f;
        return;
    }
    play_seconds += time_getDelta();
    if (play_seconds < VR_DIAG_SAVE_DELAY_S) {
        return;
    }
    // The game's own guards in gameSelect_saveAndExit (a real level, a chosen game, not the file
    // select or a demo); waiting here instead of calling it blind keeps the log line truthful.
    level = level_get();
    if (level <= 0 || level >= 0xD || gameSelect_getGameNumber() == -1 || map_get() == 0x91) {
        return;
    }
    done = 1;
    gameSelect_saveAndExit();
    recomp_printf("[vr-diag] game saved (game %d, level %d, map 0x%X)\n", gameSelect_getGameNumber() + 1, level, map_get());
}
