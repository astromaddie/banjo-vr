#include "patches.h"
#include "functions.h"
#include "core1/core1.h"
#include "vr_state.h"
#include "vr_scene_rules.h"

extern f32 sViewportFOVy;
extern f32 sViewportPosition[3];
extern f32 sViewportRotation[3];
s32 ncCamera_getType(void);
s32 ncFirstPersonCamera_getState(void);
s32 map_get(void);
s32 player_is_present(void);
s32 level_get(void);
s32 getGameMode(void);
void func_802C02D4(f32 target[3]);

static VrGameState vr_state;
static u32 vr_frame;
static s32 vr_logged_scene;
s32 vr_sky_drawn;

// Capture the *actual* main projection, after near/far clamps, before temporary HUD,
// transition and sky viewports replace it. Mirrors inverse(Rz(roll) Rx(-pitch) Ry(-yaw)).
void vr_capture_camera(f32 near, f32 far) {
    f32 p = sViewportRotation[0] * 0.01745329252f;
    f32 y = sViewportRotation[1] * 0.01745329252f;
    f32 r = sViewportRotation[2] * 0.01745329252f;
    f32 sp = sinf(p), cp = cosf(p), sy = sinf(y), cy = cosf(y);
    f32 sr = sinf(r), cr = cosf(r);
    s32 i;
    for (i = 0; i < 3; ++i) {
        vr_state.cam_eye[i] = sViewportPosition[i];
    }
    vr_state.cam_at[0] = vr_state.cam_eye[0] - sy * cp;
    vr_state.cam_at[1] = vr_state.cam_eye[1] + sp;
    vr_state.cam_at[2] = vr_state.cam_eye[2] - cy * cp;
    // The bridge orbits about cam_at. A unit forward vector would orbit about the eye,
    // not Banjo. The free dynamic camera's native target includes its player-height/lag rules.
    // Only read it in live third-person VR; the flat/first-person/scripted basis stays native.
    if (recomp_vr_is_active() && getGameMode() == GAME_MODE_3_NORMAL
        && ncCamera_getType() == CAMERA_TYPE_2_DYNAMIC && player_is_present()
        && !(ncFirstPersonCamera_getState() >= 1 && ncFirstPersonCamera_getState() <= 3)) {
        func_802C02D4(vr_state.cam_at);
    }
    vr_state.cam_up[0] = sr * cy + cr * sp * sy;
    vr_state.cam_up[1] = cr * cp;
    vr_state.cam_up[2] = -sr * sy + cr * sp * cy;
    vr_state.fov_y_deg = sViewportFOVy;
    vr_state.near_z = near;
    vr_state.far_z = far;
}

// baMotor_80250C08 is reached exactly once by mainLoop, including paused/loading frames.
// Publish the completed preceding draw here, before any locks or display-list submission.
// Scene state may lead/lag its workload by a tick, as allowed by the frozen contract.
void vr_frame_hook(void) {
    static const char *names[VR_SCENE_COUNT] = {
        "unknown", "gameplay", "first-person", "cutscene", "transition",
        "menu", "pause", "arcade", "multiplayer"
    };
    s32 present = player_is_present();
    s32 state = present ? bs_getState() : 0;
    s32 scene = vr_scene_classify(getGameMode(), map_get(), level_get(), ncCamera_getType(),
                                 ncFirstPersonCamera_getState(), state, gctransition_active());
    vr_state.frame = ++vr_frame;
    vr_state.scene = scene;
    if (present) {
        player_getPosition(vr_state.player_pos);
        vr_state.player_yaw_rad = yaw_get() * 0.01745329252f;
    } else {
        vr_state.player_pos[0] = vr_state.player_pos[1] = vr_state.player_pos[2] = 0.0f;
        vr_state.player_yaw_rad = 0.0f;
    }
    vr_state.sky_dome = vr_sky_drawn;
    vr_sky_drawn = 0;
    recomp_vr_publish_game_state(&vr_state);
    if (recomp_vr_is_active()) {
        if (scene != vr_logged_scene) {
            recomp_printf("[vr-scene] %s -> %s\n", names[vr_logged_scene], names[scene]);
            vr_logged_scene = scene;
        }
    } else {
        vr_logged_scene = VR_SCENE_UNKNOWN;
    }
}
