#ifndef BK_VR_TURN_RULES_H
#define BK_VR_TURN_RULES_H

#include "vr_scene_rules.h"

// Only free follow/orbit cameras. Node/locked, C-swing, climbing, spline and authored
// action cameras retain control. Transformations and Talon Trot use the same free states.
static inline int vr_turn_camera_allowed(int scene, int present, int update_enabled,
                                        int dynamic_state, int mode, int movement, int map) {
    if (scene != VR_SCENE_GAMEPLAY || !present || !update_enabled
        || movement == BSGROUP_5_CLIMB || map == MAP_90_GL_BATTLEMENTS) return 0;
    if (mode == 0 || mode == 5 || mode == 6 || mode == 7 || mode == 8 || mode == 9 || mode == 0xA)
        return 0;
    return dynamic_state == 1 || dynamic_state == 3 || dynamic_state == 4
        || dynamic_state == 0xB || dynamic_state == 0x13;
}
#endif
