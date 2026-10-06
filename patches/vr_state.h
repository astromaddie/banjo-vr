#ifndef BK_VR_STATE_H
#define BK_VR_STATE_H

#include "patch_helpers.h"

// Frozen dk64vr_game_state ABI, shared with the renderer. N64 headers have no stdint.h.
#define VR_SCENE_UNKNOWN 0
#define VR_SCENE_GAMEPLAY 1
#define VR_SCENE_FIRST_PERSON 2
#define VR_SCENE_CUTSCENE 3
#define VR_SCENE_TRANSITION 4
#define VR_SCENE_MENU 5
#define VR_SCENE_PAUSE 6
#define VR_SCENE_ARCADE 7
#define VR_SCENE_MULTIPLAYER 8
#define VR_SCENE_COUNT 9

#ifdef MIPS
typedef struct VrGameState {
    u32 frame;
    s32 scene;
    f32 cam_eye[3];
    f32 cam_at[3];
    f32 cam_up[3];
    f32 fov_y_deg;
    f32 near_z;
    f32 far_z;
    f32 player_pos[3];
    f32 player_yaw_rad;
    s32 sky_dome;
} VrGameState;
_Static_assert(sizeof(VrGameState) == 76, "frozen snapshot ABI");
#endif

DECLARE_FUNC(s32, recomp_vr_is_active);
DECLARE_FUNC(void, recomp_vr_publish_game_state, const VrGameState*);
DECLARE_FUNC(s32, recomp_vr_diag_save_in_play);
// Positive degrees turn right. A rejected poll drops pending input and clears host orbit.
DECLARE_FUNC(s32, recomp_vr_take_turn, s32 applicable);

#ifdef MIPS
void vr_frame_hook(void);
void vr_diag_save_tick(s32 scene);
void vr_capture_camera(f32 near, f32 far);
extern s32 vr_sky_drawn;
void vr_apply_comfort_turn(s32 camera_type);
s32 vr_comfort_camera_active(void);
s32 vr_comfort_camera_held(void);
#endif
#endif
