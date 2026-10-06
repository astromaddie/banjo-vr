#ifndef BK_VR_SCENE_RULES_H
#define BK_VR_SCENE_RULES_H

#include "enums.h"
#include "core2/nc/camera.h"
#include "vr_state.h"

// Pure rules used by the live hook and host tests. Ordering is deliberate: a transition
// covers its destination; pause covers a scripted view; title/demo/2D content stays on a panel.
static inline int vr_scene_classify(int mode, int map, int level, int camera,
                                    int first_person, int player_state, int transition) {
    if (transition || mode == GAME_MODE_1_UNKNOWN || mode == GAME_MODE_2_UNKNOWN) {
        return VR_SCENE_TRANSITION;
    }
    if (mode == GAME_MODE_4_PAUSED) {
        return VR_SCENE_PAUSE;
    }
    if (map == MAP_1E_CS_START_NINTENDO || map == MAP_1F_CS_START_RAREWARE
        || map == MAP_91_FILE_SELECT || map == MAP_83_CS_GAME_OVER_MACHINE_ROOM
        || mode == GAME_MODE_7_ATTRACT_DEMO || mode == GAME_MODE_9_BANJO_AND_KAZOOIE
        || mode == GAME_MODE_8_BOTTLES_BONUS || mode == GAME_MODE_A_SNS_PICTURE) {
        return VR_SCENE_MENU;
    }
    if (level == LEVEL_D_CUTSCENE || camera == CAMERA_TYPE_1_UNKNOWN
        || camera == CAMERA_TYPE_3_STATIC || camera == CAMERA_TYPE_4_RANDOM
        || player_state == BS_44_JIG_JIGGY || player_state == BS_34_JIG_NOTEDOOR) {
        return VR_SCENE_CUTSCENE;
    }
    if (first_person >= FIRSTPERSON_STATE_1_ENTER && first_person <= FIRSTPERSON_STATE_3_EXIT) {
        return VR_SCENE_FIRST_PERSON;
    }
    if (mode == GAME_MODE_3_NORMAL) {
        return VR_SCENE_GAMEPLAY;
    }
    return VR_SCENE_MENU; // unknown/non-playback modes fail closed to a panel
}
#endif
