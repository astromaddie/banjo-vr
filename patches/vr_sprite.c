#include "patches.h"
#include "vr_state.h"
#include "functions.h"
#include "core1/core1.h"

extern struct {
    void (*unk0)(ActorMarker *);
    void *unk4;
    s32 unk8;
} D_80371EC0;
extern f32 D_80371ECC;
extern u8 D_803858B0;
void func_80344124(void);
void func_80349AD0(void);

// Copies of the native sprite paths; only the VR distance calculation differs.
RECOMP_PATCH void func_80344138(BKSpriteDisplayData *self, s32 frame, s32 mirrored, f32 position[3], f32 scale[3], Gfx **gfx, Mtx **mtx) {
    f32 sp6C[3];
    f32 sp60[3];
    f32 temp_f14;
    f32 sp50[3];
    f32 temp_f0;
    f32 temp_f12;
    f32 var_f2_2;
    BKSpriteFrameDisplayData *temp_a3;
    f32 sp3C;
    f32 sp38;
    f32 sp34;

    viewport_getPosition_vec3f(sp6C);
    viewport_getLookVector(sp60);
    sp50[0] = position[0] - sp6C[0];
    sp50[1] = position[1] - sp6C[1];
    sp50[2] = position[2] - sp6C[2];
    temp_f14 = sp60[0]*sp50[0] + sp60[1]*sp50[1] + sp60[2]*sp50[2];
    // In VR, distance/LOD must be independent of the flat camera heading.
    if (recomp_vr_is_active()) {
        temp_f14 = gu_sqrtf(sp50[0]*sp50[0] + sp50[1]*sp50[1] + sp50[2]*sp50[2]);
    }
    if ((temp_f14 < 0.0f) || (20000.0f < temp_f14)) {
        func_80344124();
        return;
    }
    if (scale != NULL) {
        sp3C = scale[0];
        sp38 = scale[1];
        sp34 = scale[2];
    } else {
        sp3C = 1.0f;
        sp34 = 1.0f;
        sp38 = 1.0f;
    }

    temp_f0 = sp3C * self->sprite->unk8;
    temp_f12 = sp38 * self->sprite->unkA;
    var_f2_2 = (temp_f12 <= temp_f0) ? temp_f0 : temp_f12;
    if ((3000.0f < temp_f14) && ((var_f2_2 / temp_f14) < D_80371ECC) && !D_803858B0) {
        func_80344124();
        return;
    }
    if (D_80371EC0.unk0 != NULL) {
        D_80371EC0.unk0(D_80371EC0.unk4);
    }
    mlMtxSet(viewport_getMatrix());
    func_80252330(sp50[0], sp50[1], sp50[2]);
    if ((scale != NULL) || mirrored) {
        mlMtxScale_xyz((mirrored) ? -scale[0] : scale[0], sp38, sp34);
    }
    mlMtxApply(*mtx);
    gSPMatrix((*gfx)++, (*mtx)++, G_MTX_PUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
    temp_a3 = (BKSpriteFrameDisplayData *)((s32)self + sizeof(BKSpriteDisplayData) + frame*sizeof(BKSpriteFrameDisplayData));
    gSPSegment((*gfx)++, 1, osVirtualToPhysical(temp_a3->vtx));
    gSPDisplayList((*gfx)++, temp_a3->gfx);
    gSPPopMatrix((*gfx)++, G_MTX_MODELVIEW);
    func_80349AD0();
    func_80344124();
}

RECOMP_PATCH void func_80344424(BKSpriteDisplayData *arg0, s32 frame, bool mirrored, f32 position[3], f32 scale[3], f32 rotation, Gfx **gfx, Mtx **mtx) {
    f32 sp6C[3];
    f32 sp60[3];
    f32 sp5C;
    f32 sp50[3];
    f32 temp_f0;
    f32 temp_f12;
    f32 var_f2_2;
    BKSpriteFrameDisplayData *temp_a3;
    f32 sp3C;
    f32 sp38;
    f32 sp34;

    viewport_getPosition_vec3f(sp6C);
    viewport_getLookVector(sp60);
    sp50[0] = position[0] - sp6C[0];
    sp50[1] = position[1] - sp6C[1];
    sp50[2] = position[2] - sp6C[2];
    sp5C = sp60[0]*sp50[0] + sp60[1]*sp50[1] + sp60[2]*sp50[2];
    // In VR, distance/LOD must be independent of the flat camera heading.
    if (recomp_vr_is_active()) {
        sp5C = gu_sqrtf(sp50[0]*sp50[0] + sp50[1]*sp50[1] + sp50[2]*sp50[2]);
    }
    if ((sp5C < 0.0f) || (20000.0f < sp5C)) {
        func_80344124();
        return;
    }
    if (scale != NULL) {
        sp3C = scale[0];
        sp38 = scale[1];
        sp34 = scale[2];
    } else {
        sp3C = 1.0f;
        sp34 = 1.0f;
        sp38 = 1.0f;
    }

    temp_f0 = sp3C * arg0->sprite->unk8;
    temp_f12 = sp38 * arg0->sprite->unkA;
    var_f2_2 = (temp_f12 <= temp_f0) ? temp_f0 : temp_f12;
    if ((3000.0f < sp5C) && ((var_f2_2 / sp5C) < D_80371ECC) && !D_803858B0) {
        func_80344124();
        return;
    }

    if (D_80371EC0.unk0 != NULL) {
        D_80371EC0.unk0(D_80371EC0.unk4);
    }
    mlMtxSet(viewport_getMatrix());
    mlMtxRotatePYR(0.0f, 0.0f, rotation);
    func_80252330(sp50[0], sp50[1], sp50[2]);
    if ((scale != NULL) || mirrored) {
        mlMtxScale_xyz((mirrored) ? -scale[0] : scale[0], sp38, sp34);
    }
    mlMtxApply(*mtx);
    gSPMatrix((*gfx)++, (*mtx)++, G_MTX_PUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
    temp_a3 = (BKSpriteFrameDisplayData *)((s32)arg0 + sizeof(BKSpriteDisplayData) + frame*sizeof(BKSpriteFrameDisplayData));
    gSPSegment((*gfx)++, 1, osVirtualToPhysical(temp_a3->vtx));
    gSPDisplayList((*gfx)++, temp_a3->gfx);
    gSPPopMatrix((*gfx)++, G_MTX_MODELVIEW);
    func_80349AD0();
    func_80344124();
}


RECOMP_PATCH void func_80344720(BKSpriteDisplayData *arg0, s32 frame, bool mirrored, f32 position[3], f32 rotation[3], f32 scale[3], Gfx **gfx, Mtx **mtx) {
    f32 sp5C[3];
    f32 sp50[3];
    f32 sp4C;
    f32 sp40[3];
    f32 temp_f0;
    f32 temp_f12;
    f32 var_f14;
    BKSpriteFrameDisplayData *temp_a3;

    viewport_getPosition_vec3f(sp5C);
    viewport_getLookVector(sp50);
    sp40[0] = position[0] - sp5C[0];
    sp40[1] = position[1] - sp5C[1];
    sp40[2] = position[2] - sp5C[2];
    sp4C = sp50[0]*sp40[0] + sp50[1]*sp40[1] + sp50[2]*sp40[2];
    // In VR, distance/LOD must be independent of the flat camera heading.
    if (recomp_vr_is_active()) {
        sp4C = gu_sqrtf(sp40[0]*sp40[0] + sp40[1]*sp40[1] + sp40[2]*sp40[2]);
    }
    if ((sp4C < 0.0f) || (20000.0f < sp4C)) {
        func_80344124();
        return;
    }

    temp_f0 = scale[0] * arg0->sprite->unk8;
    temp_f12 = scale[1] * arg0->sprite->unkA;
    var_f14 = (temp_f12 <= temp_f0) ? temp_f0 : temp_f12;
    if ((3000.0f < sp4C) && ((var_f14 / sp4C) < D_80371ECC) && !D_803858B0) {
        func_80344124();
        return;
    }

    if (D_80371EC0.unk0 != NULL) {
        D_80371EC0.unk0(D_80371EC0.unk4);
    }
    mlMtxIdent();
    func_80252330(sp40[0], sp40[1], sp40[2]);
    mlMtxRotatePYR(rotation[0], rotation[1], rotation[2]);
    mlMtxScale_xyz((mirrored) ? -scale[0] : scale[0], scale[1], scale[2]);
    mlMtxApply(*mtx);
    gSPMatrix((*gfx)++, (*mtx)++, G_MTX_PUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
    temp_a3 = (BKSpriteFrameDisplayData *)((s32)arg0 + sizeof(BKSpriteDisplayData) + frame*sizeof(BKSpriteFrameDisplayData));
    gSPSegment((*gfx)++, 1, osVirtualToPhysical(temp_a3->vtx));
    gSPDisplayList((*gfx)++, temp_a3->gfx);
    gSPPopMatrix((*gfx)++, G_MTX_MODELVIEW);
    func_80349AD0();
    func_80344124();
}
