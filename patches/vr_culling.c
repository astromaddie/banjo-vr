#include "patches.h"
#include "vr_state.h"
#include "functions.h"
#include "core1/core1.h"
#include "core1/ml.h"

extern struct {
    Cube *cubes;
    f32 margin;
    s32 min[3];
    s32 max[3];
    s32 stride[2];
    s32 cubeCnt;
    s32 unk2C;
    s32 width[3];
    Cube *unk3C;
    Cube *unk40;
    s32 unk44;
} sCubeList;
s32 mapSpecificFlags_validateCRC1(void);
void func_8032D3A8(void);
void cube_positionToIndices(s32 dst[3], f32 pos[3]);
void func_80308EC8(void);
void func_8032D510(Cube *, Gfx **, Mtx **, Vtx **);
void func_80301F50(Gfx **, Mtx **, Vtx **, s32[3], s32[3], s32[3]);
void func_80302634(Gfx **, Mtx **, Vtx **, s32[3], s32[3], s32[3]);
void func_80308D2C(Gfx **, Mtx **, Vtx **);

RECOMP_PATCH void func_80302C94(Gfx **gfx, Mtx **mtx, Vtx **vtx) {
    f32 vp_position[3];
    s32 vp_cube_indices[3];
    f32 vp_rotation[3];
    s32 i;
    s32 sp44[3];
    s32 sp38[3];
    f32 temp_f18;

    if (!mapSpecificFlags_validateCRC1())
        return;

    func_8032D3A8();
    viewport_getPosition_vec3f(vp_position);
    viewport_getRotation_vec3f(vp_rotation);
    func_80256664(vp_rotation);
    cube_positionToIndices(vp_cube_indices, vp_position);
    vp_cube_indices[0] -= sCubeList.min[0];
    vp_cube_indices[1] -= sCubeList.min[1];
    vp_cube_indices[2] -= sCubeList.min[2];
    func_80308EC8();
    sp44[0] = sp44[1] = sp44[2] = 0;
    sp38[0] = sCubeList.width[0] - 1;
    sp38[1] = sCubeList.width[1] - 1;
    sp38[2] = sCubeList.width[2] - 1;
    // The flat traversal excludes entire half-spaces behind its camera. In VR
    // visit both sides, retaining the native four-cell radius and sort order.
    if (!recomp_vr_is_active()) {
        if ((vp_rotation[0]> 250.0f) && (vp_rotation[0]< 290.0f)) {
            if ((vp_rotation[1] >= 225.0f) && (vp_rotation[1] <= 315.0f)) {
                sp44[0] = (vp_cube_indices[0] > sp44[0]) ? vp_cube_indices[0] - 1 : sp44[0];
            } else {
                if ((vp_rotation[1] >= 45.0f) && (vp_rotation[1] <= 135.0f)) {
                    sp38[0] = vp_cube_indices[0];
                }
            }

            if ((vp_rotation[0]>= 45.0f) && (vp_rotation[0]<= 135.0f)) {
                sp44[1] = vp_cube_indices[1];
            } else if ((vp_rotation[0]>= 225.0f) && (vp_rotation[0]<= 315.0f)) {
                sp38[1] = vp_cube_indices[1];
            }
            if ((vp_rotation[1] >= 135.0f) && (vp_rotation[1] <= 225.0f)) {
                sp44[2] = (vp_cube_indices[2] > sp44[2]) ? vp_cube_indices[2] - 1 : sp44[2];
            } else if ((315.0f <= vp_rotation[1]) || (vp_rotation[1] <= 45.0f)) {
                sp38[2] = vp_cube_indices[2];
            }
        } else {
            if ((vp_rotation[1] >= 225.0f) && (vp_rotation[1] <= 315.0f)) {
                sp44[0] = vp_cube_indices[0];
            } else {
                if ((vp_rotation[1] >= 45.0f) && (vp_rotation[1] <= 135.0f)) {
                    sp38[0] = vp_cube_indices[0];
                }
            }

            if ((vp_rotation[0]>= 45.0f) && (vp_rotation[0]<= 135.0f)) {
                sp44[1] = vp_cube_indices[1];
            } else if ((vp_rotation[0]>= 225.0f) && (vp_rotation[0]<= 315.0f)) {
                sp38[1] = vp_cube_indices[1];
            }

            if ((vp_rotation[1] >= 135.0f) && (vp_rotation[1] <= 225.0f)) {
                sp44[2] = vp_cube_indices[2];
            } else if ((315.0f <= vp_rotation[1]) || (vp_rotation[1] <= 45.0f)) {
                sp38[2] = vp_cube_indices[2];
            }
        }

    }

    for(i = 0; i < 3; i++){
        if(vp_cube_indices[i] - sp44[i] >= 5){
            sp44[i] = vp_cube_indices[i] - 4;
        }
        if(sp38[i] - vp_cube_indices[i] >= 5){
            sp38[i] = vp_cube_indices[i] + 4;
        }
    }
    if (sCubeList.unk3C != NULL) {
        func_8032D510(sCubeList.unk3C, gfx, mtx, vtx);
    }

    if (sCubeList.unk40 != NULL) {
        func_8032D510(sCubeList.unk40, gfx, mtx, vtx);
    }
    if (((45.0f <= vp_rotation[1]) && (vp_rotation[1] <= 135.0f)) || ((225.0f <= vp_rotation[1]) && (vp_rotation[1] <= 315.0f))) {
        func_80301F50(gfx, mtx, vtx, vp_cube_indices, sp44, sp38);
    } else {
        func_80302634(gfx, mtx, vtx, vp_cube_indices, sp44, sp38);
    }
    func_80308D2C(gfx, mtx, vtx);
}

// Camera projection range globals (code_34790.c).
extern f32 D_8037D8CC;
extern f32 D_8037D8C8;
void mapModel_getBounds(s32 min[3], s32 max[3]);
s32 coords_func_8033EAF8(s32 min[3], s32 max[3], f32 eye[3], f32 look[3]);
f32 func_802BB938(f32 eye[3], f32 rotation[3]);
s32 level_get(void);
s32 map_get(void);

RECOMP_PATCH void func_802BBA84(void) {
    f32 vp_position[3];
    f32 sp48[3];
    f32 vp_rotation[3];
    s32 min[3];
    s32 max[3];

    mapModel_getBounds(min, max);
    viewport_getPosition_vec3f(vp_position);
    viewport_getRotation_vec3f(vp_rotation);
    viewport_getLookVector(sp48);
    if (recomp_vr_is_active()) {
        // Native far is the ray through the map along the *flat* look vector.
        // Use the furthest map-bound corner, so a head turn cannot clip the world.
        f32 radius2 = 0.0f;
        s32 i;
        for (i = 0; i < 3; ++i) {
            f32 lo = mlAbsF(min[i] - vp_position[i]);
            f32 hi = mlAbsF(max[i] - vp_position[i]);
            f32 extent = MAX(lo, hi);
            radius2 += extent * extent;
        }
        D_8037D8CC = gu_sqrtf(radius2);
    } else {
        D_8037D8CC = (f32) coords_func_8033EAF8(min, max, vp_position, sp48);
    }
    D_8037D8CC += 100.0f;
    if (D_8037D8CC < 1000.0f) {
        D_8037D8CC = 1000.0f;
    }
    if (20000.0 < D_8037D8CC) {
        D_8037D8CC = 20000.0f;
    }
    if (level_get() == LEVEL_D_CUTSCENE) {
        D_8037D8C8 = D_8037D8CC * 0.0078125;
        return;
    }
    if (map_get() == MAP_91_FILE_SELECT) {
        D_8037D8C8 = D_8037D8CC * 0.0078125;
        return;
    }
    D_8037D8C8 = D_8037D8CC * 0.0078125;
    if (D_8037D8C8 < 10.0) {
        D_8037D8C8 = 10.0f;
        return;
    }
    D_8037D8C8 = ml_min_f(D_8037D8C8, func_802BB938(vp_position, vp_rotation));
}
