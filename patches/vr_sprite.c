#include "patches.h"
#include "vr_state.h"
#include "functions.h"
#include "core1/core1.h"
#include "transform_ids.h"

extern struct {
    void (*unk0)(ActorMarker *);
    void *unk4;
    s32 unk8;
} D_80371EC0;
extern f32 D_80371ECC;
extern u8 D_803858B0;
void func_80344124(void);
void func_80349AD0(void);

u32 vr_model_billboard_flag = 0;

u32 vr_billboard_group_id(u32 id, u32 flag) {
    if ((id == G_EX_ID_IGNORE) || (id == G_EX_ID_AUTO) || !recomp_vr_is_active()) {
        return id;
    }

    return id | flag;
}

// VR: a sprite's own matrix group with a billboard flag on the id of the prop or actor drawing it
// (the prop's or marker's transform id), with the prop sprites' interpolation. FALSE (nothing
// pushed) off VR or without an id.
static bool vr_push_sprite_billboard_group(Gfx **gfx, u32 flag) {
    if ((cur_drawn_model_transform_id == 0) || !recomp_vr_is_active()) {
        return FALSE;
    }

    u32 id = vr_billboard_group_id(cur_drawn_model_transform_id, flag);
    if (perspective_interpolation_skipped()) {
        gEXMatrixGroupSkipAll((*gfx)++, id, G_EX_PUSH, G_MTX_MODELVIEW, G_EX_EDIT_NONE);
    }
    else {
        gEXMatrixGroupDecomposed((*gfx)++, id, G_EX_PUSH, G_MTX_MODELVIEW,
            G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_SKIP, G_EX_COMPONENT_INTERPOLATE,
            G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_SKIP, G_EX_COMPONENT_INTERPOLATE,
            G_EX_ORDER_LINEAR, G_EX_EDIT_NONE, G_EX_COMPONENT_SKIP, G_EX_COMPONENT_AUTO);
    }
    return TRUE;
}

// Copies of the native sprite paths; only the VR distance calculation and the VR billboard tag
// differ.
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
    // VR: prop and actor sprites (notes, flowers, eggs) are upright billboards.
    bool billboard_group = vr_push_sprite_billboard_group(gfx, VR_BILLBOARD_CYLINDRICAL_ID_FLAG);
    gSPMatrix((*gfx)++, (*mtx)++, G_MTX_PUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
    temp_a3 = (BKSpriteFrameDisplayData *)((s32)self + sizeof(BKSpriteDisplayData) + frame*sizeof(BKSpriteFrameDisplayData));
    gSPSegment((*gfx)++, 1, osVirtualToPhysical(temp_a3->vtx));
    gSPDisplayList((*gfx)++, temp_a3->gfx);
    gSPPopMatrix((*gfx)++, G_MTX_MODELVIEW);
    if (billboard_group) {
        gEXPopMatrixGroup((*gfx)++, G_MTX_MODELVIEW);
    }
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

extern f32 D_8036E58C[3];
BKSpriteDisplayData *func_80330F30(ActorMarker *marker);
void func_803257A4(ActorMarker *marker);
void func_80344C38(void (*arg0)(ActorMarker *), ActorMarker *arg1);
s32 func_80344C20(BKSpriteDisplayData *arg0);
void func_80335D30(Gfx **gfx);
void func_8033687C(Gfx **gfx);
void func_80338338(s32 r, s32 g, s32 b);
void func_803382FC(s32 alpha);
void func_803382E4(s32 arg0);

// Copy of the camera-facing actor sprite draw (rotation = the camera's, plus the actor's roll):
// Grunty's spells. VR: a spherical billboard.
RECOMP_PATCH Actor *func_80325CAC(ActorMarker *marker, Gfx **gfx, Mtx **mtx, Vtx **vtx) {
    f32 scale[3];
    f32 rotation[3];
    Actor *this;
    BKSpriteDisplayData *sp40;

    this = marker_getActor(marker);
    sp40 = func_80330F30(marker);
    scale[0] = scale[1] = scale[2] = this->scale;
    if (this->unk104 != NULL) {
        this->position[0] -= D_8036E58C[0];
        this->position[1] -= D_8036E58C[1];
        this->position[2] -= D_8036E58C[2];
    }
    viewport_getRotation_vec3f(rotation);
    rotation[2] += this->roll;
    func_80338338(0xFF, 0xFF, 0xFF);
    if (this->unk124_11 != 0) {
        func_803382FC(this->alpha_124_19);
        func_803382E4(0xC);
    } else if ((func_80344C20(sp40) & 0xB00) != 0) {
        func_803382E4(0xB);
    } else {
        func_803382E4(0xE);
    }
    func_80344C38(&func_803257A4, marker);
    func_80335D30(gfx);
    bool billboard_group = vr_push_sprite_billboard_group(gfx, VR_BILLBOARD_SPHERICAL_ID_FLAG);
    func_80344720(sp40, marker->propPtr->unk8_15, marker->propPtr->unk8_5, this->position, rotation, scale, gfx, mtx);
    if (billboard_group) {
        gEXPopMatrixGroup((*gfx)++, G_MTX_MODELVIEW);
    }

    func_8033687C(gfx);
    if (this->unk104 != NULL) {
        this->position[0] = this->position[0] + D_8036E58C[0];
        this->position[1] = this->position[1] + D_8036E58C[1];
        this->position[2] = this->position[2] + D_8036E58C[2];
    }
    return this;
}
