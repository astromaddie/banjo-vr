#include "patches.h"
#include "functions.h"
#include "core1/core1.h"
#include "vr_turn_rules.h"

extern u8 D_8037C062, cameraUpdateEnabled;
extern f32 D_8037DBA4, D_8037DBA8;
s32 ncCamera_getType(void);
s32 ncDynamicCamera_getState(void);
s32 ncFirstPersonCamera_getState(void);
s32 player_is_present(void);
s32 getGameMode(void);
s32 map_get(void);
s32 level_get(void);
s32 player_movementGroup(void);
s32 bainput_should_rotate_camera_left(void);
s32 bainput_should_rotate_camera_right(void);
void ncDynamicCamera_setState(s32 state);
void ncDynamicCamera_getPosition(f32 position[3]);
void func_802C069C(void);
void func_80291488(s32 mode);

static s32 vr_turn_owned;
static f32 vr_turn_age;
static f32 vr_turn_remaining, vr_turn_last_yaw;
static f32 vr_turn_last_eye[3];

static s32 vr_turn_allowed(s32 camera_type) {
    s32 present = player_is_present();
    s32 scene = vr_scene_classify(getGameMode(), map_get(), level_get(), camera_type,
        ncFirstPersonCamera_getState(), present ? bs_getState() : 0, gctransition_active());
    return camera_type == CAMERA_TYPE_2_DYNAMIC && !bakey_held(BUTTON_R)
        && !bainput_should_rotate_camera_left() && !bainput_should_rotate_camera_right()
        && vr_turn_camera_allowed(scene, present,
        cameraUpdateEnabled, ncDynamicCamera_getState(), D_8037C062,
        present ? player_movementGroup() : 0, map_get());
}

s32 vr_comfort_camera_active(void) {
    return recomp_vr_is_active() && vr_turn_owned;
}

// Reuse the existing analog camera's retention of R-look, including underwater and flight.
// The native selector still checks camera nodes first, so this cannot override a locked room.
s32 vr_comfort_camera_held(void) {
    if (!recomp_vr_is_active() || !vr_turn_owned) return 0;
    if (!vr_turn_allowed(ncCamera_getType()) || ncDynamicCamera_getState() != 0x13 || vr_turn_age > 8.0f
        || bakey_held(BUTTON_R) || bainput_should_rotate_camera_left()
        || bainput_should_rotate_camera_right()
        || mlAbsF(vr_turn_remaining) < 0.01f) {
        vr_turn_owned = 0;
        return 0;
    }
    return 1;
}

// Called by the existing tagged camera dispatch before the selected camera advances.
// Poll even when disabled/static: rejection drains requests and tells the host to stop orbiting.
void vr_apply_comfort_turn(s32 camera_type) {
    if (recomp_vr_is_active()) {
        f32 eye[3];
        ncDynamicCamera_getPosition(eye);
        if (vr_turn_owned) {
            f32 dx = eye[0] - vr_turn_last_eye[0], dy = eye[1] - vr_turn_last_eye[1];
            f32 dz = eye[2] - vr_turn_last_eye[2];
            if (dx*dx + dy*dy + dz*dz > 400.0f*400.0f) {
                if (ncDynamicCamera_getState() == 0x13) {
                    func_802C069C();
                    D_8037DBA4 = D_8037DBA8;
                }
                vr_turn_owned = 0;
                vr_turn_remaining = 0.0f;
            }
            else if (ncDynamicCamera_getState() != 0x13) {
                vr_turn_owned = 0;
                vr_turn_remaining = 0.0f;
            }
        }
        ml_vec3f_copy(vr_turn_last_eye, eye);
        s32 allowed = vr_turn_allowed(camera_type);
        s32 degrees = recomp_vr_take_turn(allowed);
        if (!allowed) vr_turn_owned = 0;
        vr_turn_age += time_getDelta();
        if (degrees != 0) {
            s32 previous = ncDynamicCamera_getState();
            s32 starting = !vr_turn_owned;
            // Ownership before init preserves target/radius even with desktop analog off.
            if (starting) vr_turn_remaining = 0.0f;
            vr_turn_owned = 1;
            vr_turn_age = 0.0f;
            ncDynamicCamera_setState(0x13);
            func_80291488(4);
            if (starting || previous != 0x13) vr_turn_last_yaw = D_8037DBA8;
            vr_turn_remaining -= degrees;
        }
        if (allowed && vr_turn_owned) {
            // Retain an unwrapped debt. The native follower takes shortest wrapped angles;
            // a nearby proxy target keeps it moving in the requested direction even if the
            // host gets a whole revolution ahead (smooth speed may exceed native 160 deg/s).
            vr_turn_remaining -= mlDiffDegF(D_8037DBA8, vr_turn_last_yaw);
            vr_turn_last_yaw = D_8037DBA8;
            D_8037DBA4 = mlNormalizeAngle(D_8037DBA8
                + ml_clamp_f(vr_turn_remaining, -179.0f, 179.0f));
        }
        if (degrees != 0) {
            recomp_printf("[vr-turn] accepted %d deg target %.3f current %.3f debt %.3f\n",
                degrees, D_8037DBA4, D_8037DBA8, vr_turn_remaining);
        }
    }
    else vr_turn_owned = 0;
}
