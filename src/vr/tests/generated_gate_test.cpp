// Executes the actual N64Recomp output. Original ROM functions are the flat-path oracle;
// callees supplying map/camera data and consuming display lists are controlled here.
#include <cassert>
#include <cmath>
#include <cstring>
#include <iostream>
#include <fstream>
#include <vector>
#include <array>
#include "selected.h"
#include "test_symbols.h"
#include "../include/dk64vr_game_state.h"

static gpr addr(uint32_t a) { return static_cast<int32_t>(a); }
static float &f(uint8_t *rdram, uint32_t a) {
    return *reinterpret_cast<float *>(rdram + a - 0x80000000u);
}
static int mode = 3, map = 1, level = 11, camera = 2, fp = 0, action = 1, transition = 0;
static bool present = true, demo = false;
static int logs = 0, models = 0;
static std::array<float, 3> eye{0, 0, 0}, rot{0, 0, 0}, look{0, 0, -1};
static std::vector<int32_t> trace;
static void vec(uint8_t *rdram, gpr dest, const std::array<float, 3> &v) {
    std::memcpy(rdram + static_cast<uint32_t>(dest) - 0x80000000u, v.data(), 12);
}
#define STUB(name) extern "C" void name(uint8_t *rdram, recomp_context *ctx)
#define RETINT(name, value) STUB(name) { (void)rdram; ctx->r2 = (value); }
RETINT(getGameMode, mode) RETINT(map_get, map) RETINT(level_get, level)
RETINT(ncCamera_getType, camera) RETINT(ncFirstPersonCamera_getState, fp)
RETINT(bs_getState, action) RETINT(gctransition_active, transition)
RETINT(player_is_present, present) RETINT(recomp_in_demo_playback_game_mode, demo)
RETINT(mapSpecificFlags_validateCRC1, 1) RETINT(viewport_getMatrix, addr(0x80013000))
RETINT(perspective_interpolation_skipped, 0)
STUB(recomp_printf) { (void)rdram; (void)ctx; ++logs; }
STUB(sinf_recomp) { (void)rdram; ctx->f0.fl = std::sin(ctx->f12.fl); }
STUB(__cosf_recomp) { (void)rdram; ctx->f0.fl = std::cos(ctx->f12.fl); }
STUB(gu_sqrtf) { (void)rdram; ctx->f0.fl = std::sqrt(ctx->f12.fl); }
STUB(ml_min_f) { (void)rdram; ctx->f0.fl = std::fmin(ctx->f12.fl, ctx->f14.fl); }
STUB(func_802BB938) { (void)rdram; ctx->f0.fl = 65.0f; }
STUB(yaw_get) { (void)rdram; ctx->f0.fl = 90.0f; }
STUB(player_getPosition) { vec(rdram, ctx->r4, {100, 200, 300}); }
STUB(viewport_getPosition_vec3f) { vec(rdram, ctx->r4, eye); }
STUB(viewport_getRotation_vec3f) { vec(rdram, ctx->r4, rot); }
STUB(viewport_getLookVector) { vec(rdram, ctx->r4, look); }
STUB(mapModel_getBounds) {
    for (int i = 0; i < 3; ++i) { MEM_W(i*4, ctx->r4) = -5000; MEM_W(i*4, ctx->r5) = 5000; }
}
STUB(cube_positionToIndices) {
    for (int i = 0; i < 3; ++i) MEM_W(i*4, ctx->r4) = 10;
}
STUB(func_80256664) { (void)rdram; (void)ctx; } // tested angles are already normalized
#define TRACE0(name) STUB(name) { (void)rdram; (void)ctx; trace.push_back(__LINE__); }
TRACE0(func_8032D3A8) TRACE0(func_80308EC8) TRACE0(func_80308D2C)
STUB(func_8032D510) { (void)rdram; trace.push_back(static_cast<int32_t>(ctx->r4)); }
static void bounds(uint8_t *rdram, recomp_context *ctx) {
    for (int off : {0x10, 0x14}) {
        gpr p = addr(MEM_W(off, ctx->r29));
        for (int i = 0; i < 3; ++i) trace.push_back(MEM_W(i*4, p));
    }
}
STUB(func_80301F50) { trace.push_back(1); bounds(rdram, ctx); }
STUB(func_80302634) { trace.push_back(2); bounds(rdram, ctx); }
TRACE0(mlMtxIdent) TRACE0(mlMtxSet) TRACE0(func_80349AD0)
STUB(mlMtxApply) { std::memset(rdram + static_cast<uint32_t>(ctx->r4) - 0x80000000u, 0xAB, 64); }
STUB(func_80252330) { (void)rdram; trace.push_back(ctx->f12.u32l); trace.push_back(ctx->f14.u32l); trace.push_back(ctx->r6); }
STUB(mlMtxScale_xyz) { (void)rdram; trace.push_back(ctx->f12.u32l); trace.push_back(ctx->f14.u32l); trace.push_back(ctx->r6); }
STUB(mlMtxRotatePYR) { (void)rdram; trace.push_back(ctx->f12.u32l); trace.push_back(ctx->f14.u32l); trace.push_back(ctx->r6); }
STUB(func_80344124) { (void)ctx; MEM_W(0, addr(K_D_80371EC0)) = 0; MEM_B(0, addr(K_D_803858B0)) = 0; }
STUB(osVirtualToPhysical_recomp) { (void)rdram; ctx->r2 = ctx->r4 & 0x1FFFFFFF; }
TRACE0(ml_vec3f_pitch_rotate_copy) TRACE0(ml_vec3f_yaw_rotate_copy)
TRACE0(modelRender_setPrimAndEnvColors)
STUB(modelRender_draw) { (void)rdram; (void)ctx; ++models; }
extern "C" recomp_func_t *get_function(int32_t) { assert(false && "unexpected indirect call"); return nullptr; }
extern "C" void switch_error(const char *, uint32_t, uint32_t) { assert(false); }
extern "C" void do_break(uint32_t) { assert(false); }

static recomp_context context() {
    recomp_context c{};
    c.r29 = addr(0x80FFF000);
    return c;
}
static void close(float a, float b) { assert(std::fabs(a - b) < 0.0002f); }

int main() {
    std::vector<uint8_t> memory(32*1024*1024);
    auto *rdram = memory.data();
    auto load = [&](const char *file, uint32_t rom, uint32_t vram, uint32_t size) {
        std::ifstream in(file, std::ios::binary); assert(in);
        in.seekg(rom);
        std::vector<uint8_t> data(size);
        in.read(reinterpret_cast<char *>(data.data()), size); assert(in);
        for (uint32_t i = 0; i < size; ++i) memory[(vram - 0x80000000u + i) ^ 3] = data[i];
    };
    load("banjo.us.v10.decompressed.z64", K_core1_rom, K_core1_vram, K_core1_size);
    load("banjo.us.v10.decompressed.z64", K_core2_rom, K_core2_vram, K_core2_size);
    std::ifstream patch("patches/patches.bin", std::ios::binary | std::ios::ate); assert(patch);
    load("patches/patches.bin", 0, 0x80801000u, static_cast<uint32_t>(patch.tellg()));
    auto invoke = [&](recomp_func_t *fn) { auto c = context(); c.f_odd = &c.f0.u32h; fn(rdram, &c); return c; };
    // Existing flat culling semantics: sphere checks honor the transient switch;
    // AABB checks retain demo fidelity, while ordinary recomp play already passes them.
    for (int i = 0; i < 4; ++i) {
        f(rdram, K_sViewportFrustumPlanes + i*16) = 1;
        f(rdram, K_sViewportFrustumPlanes + i*16 + 12) = 0;
    }
    vec(rdram, addr(0x80010000), {50, 0, 0});
    vec(rdram, addr(0x80010010), {100, 0, 0});
    auto sphere = [&](recomp_func_t *fn) {
        auto c = context(); c.r4 = addr(0x80010000); c.f14.fl = 10; c.f_odd = &c.f0.u32h; fn(rdram, &c); return c.r2;
    };
    auto box = [&](recomp_func_t *fn) {
        auto c = context(); c.r4 = addr(0x80010000); c.r5 = addr(0x80010010); c.f_odd = &c.f0.u32h; fn(rdram, &c); return c.r2;
    };
    MEM_W(0, addr(K_frustum_checks_enabled)) = 1;
    dk64vr_set_active(0); demo = true;
    assert(sphere(viewport_func_8024DB50) == sphere(baseline_viewport_func_8024DB50));
    assert(box(viewport_isBoundingBoxInFrustum) == box(baseline_viewport_isBoundingBoxInFrustum));
    assert(sphere(viewport_func_8024DB50) == 0);
    demo = false; assert(box(viewport_isBoundingBoxInFrustum) == 1);
    MEM_W(0, addr(K_frustum_checks_enabled)) = 0;
    assert(sphere(viewport_func_8024DB50) == 1);
    MEM_W(0, addr(K_frustum_checks_enabled)) = 1;
    dk64vr_set_active(1); demo = true;
    assert(sphere(viewport_func_8024DB50) == 1 && box(viewport_isBoundingBoxInFrustum) == 1);
    demo = false;

    std::cerr << "culling passed\n";
    // Original vs patched map traversal call/bounds sequence in flat mode at all quadrants.
    for (int i = 0; i < 3; ++i) MEM_W(0x30 + i*4, addr(K_sCubeList)) = 21;
    for (float pitch : {0.0f, 90.0f, 270.0f}) for (float yaw : {0.0f, 90.0f, 180.0f, 270.0f}) {
        rot = {pitch, yaw, 0}; dk64vr_set_active(0);
        trace.clear(); invoke(baseline_func_80302C94); auto original = trace;
        trace.clear(); invoke(func_80302C94);
        if (trace != original) {
            std::cerr << "traversal mismatch " << pitch << ", " << yaw << "\noriginal:";
            for (auto v : original) std::cerr << " " << v;
            std::cerr << "\npatched:"; for (auto v : trace) std::cerr << " " << v;
            std::cerr << "\n"; std::exit(1);
        }
        dk64vr_set_active(1); trace.clear(); invoke(func_80302C94);
        // One sort discriminator precedes six bounds; native radius four is retained.
        const auto n = trace.size();
        assert(n >= 8);
        assert(trace[n-7] == 6 && trace[n-6] == 6 && trace[n-5] == 6);
        assert(trace[n-4] == 14 && trace[n-3] == 14 && trace[n-2] == 14);
    }
    std::cerr << "traversal passed\n";
    // Far plane off path must match the actual ROM. In VR it encloses every map direction.
    rot = {0, 0, 0};
    for (auto direction : {std::array<float,3>{0,0,-1}, {1,0,0}, {0,0,1}}) {
        look = direction; dk64vr_set_active(0);
        invoke(baseline_func_802BBA84);
        float old_far = f(rdram, K_D_8037D8CC), old_near = f(rdram, K_D_8037D8C8);
        invoke(func_802BBA84);
        assert(f(rdram, K_D_8037D8CC) == old_far && f(rdram, K_D_8037D8C8) == old_near);
        dk64vr_set_active(1); invoke(func_802BBA84);
        close(f(rdram, K_D_8037D8CC), std::sqrt(75000000.0f) + 100);
    }

    std::cerr << "far passed\n";
    // All three real sprite entry points retain flat commands/side effects and admit
    // behind-camera geometry in VR. Same authored radial range/size LOD still applies.
    MEM_W(0, addr(0x80011000)) = 0x80011100;
    MEM_W(4, addr(0x80011000)) = 0x80011200;
    MEM_W(8, addr(0x80011000)) = 0x80011300;
    MEM_H(8, addr(0x80011100)) = 100; MEM_H(10, addr(0x80011100)) = 100;
    vec(rdram, addr(0x80010100), {1,1,1});
    vec(rdram, addr(0x80010200), {0,0,0});
    look = {0,0,-1}; f(rdram, K_D_80371ECC) = 0.1f;
    std::array<recomp_func_t *,3> patched{func_80344138, func_80344424, func_80344720};
    std::array<recomp_func_t *,3> originals{baseline_func_80344138, baseline_func_80344424, baseline_func_80344720};
    auto sprite = [&](recomp_func_t *fn, int kind, float z) {
        vec(rdram, addr(0x80010000), {0,0,z});
        MEM_W(0, addr(0x80012000)) = 0x80012100;
        MEM_W(0, addr(0x80012004)) = 0x80012300;
        std::memset(rdram + 0x12100, 0, 0x300);
        auto c = context(); c.r4 = addr(0x80011000); c.r5 = c.r6 = 0; c.r7 = addr(0x80010000);
        auto put = [&](int offset, uint32_t v) { MEM_W(offset, c.r29) = v; };
        if (kind == 0) { put(0x10,0x80010100); put(0x14,0x80012000); put(0x18,0x80012004); }
        if (kind == 1) { put(0x10,0x80010100); put(0x14,0); put(0x18,0x80012000); put(0x1C,0x80012004); }
        if (kind == 2) { put(0x10,0x80010200); put(0x14,0x80010100); put(0x18,0x80012000); put(0x1C,0x80012004); }
        trace.clear(); c.f_odd = &c.f0.u32h; fn(rdram, &c);
        std::vector<uint8_t> result(rdram + 0x12000, rdram + 0x12400);
        return std::make_pair(result, trace);
    };
    for (int kind = 0; kind < 3; ++kind) for (float z : {-100.0f, 100.0f, -5000.0f, 21000.0f}) {
        dk64vr_set_active(0);
        auto old = sprite(originals[kind],kind,z);
        assert(sprite(patched[kind],kind,z) == old);
        dk64vr_set_active(1); sprite(patched[kind],kind,z);
        assert((MEM_W(0, addr(0x80012000)) != static_cast<int32_t>(0x80012100)) == (std::fabs(z) == 100));
    }
    std::cerr << "sprites passed\n";
    // Snapshot built by generated MIPS: camera basis including roll, scene, player,
    // sky latch and one publish/log per scene change. Host imports are production code.
    vec(rdram, addr(K_sViewportPosition), {10,20,30});
    vec(rdram, addr(K_sViewportRotation), {0,90,90}); f(rdram, K_sViewportFOVy) = 40;
    auto c = context(); c.f12.fl = 23; c.f14.fl = 10000; c.f_odd = &c.f0.u32h; vr_capture_camera(rdram,&c);
    dk64vr_set_active(0); invoke(vr_frame_hook); assert(logs == 0);
    dk64vr_game_state snapshot{}; dk64vr_get_game_state(&snapshot);
    close(snapshot.cam_eye[0],10); close(snapshot.cam_at[0],9);
    close(snapshot.cam_at[1],20); close(snapshot.cam_at[2],30);
    close(snapshot.cam_up[0],0); close(snapshot.cam_up[1],0); close(snapshot.cam_up[2],-1);
    assert(snapshot.fov_y_deg == 40 && snapshot.near_z == 23 && snapshot.far_z == 10000);
    close(snapshot.player_yaw_rad,1.5707963f); assert(snapshot.player_pos[2] == 300);
    dk64vr_set_active(1); MEM_W(0,addr(K_vr_sky_drawn)) = 1;
    invoke(vr_frame_hook); dk64vr_get_game_state(&snapshot);
    assert(snapshot.scene == DK64VR_SCENE_GAMEPLAY && snapshot.sky_dome == 1 && logs == 1);
    auto frame = snapshot.frame; invoke(vr_frame_hook); dk64vr_get_game_state(&snapshot);
    assert(snapshot.frame == frame+1 && snapshot.sky_dome == 0 && logs == 1);
    mode = 4; invoke(vr_frame_hook); dk64vr_get_game_state(&snapshot);
    assert(snapshot.scene == DK64VR_SCENE_PAUSE && logs == 2);
    present = false; invoke(vr_frame_hook); dk64vr_get_game_state(&snapshot);
    assert(snapshot.player_pos[0] == 0 && snapshot.player_yaw_rad == 0);
    // Camera basis stays orthonormal across pitch, yaw and roll, not just the level case.
    for (float pitch : {-60.0f, 0.0f, 60.0f}) for (float yaw : {0.0f, 90.0f, 180.0f})
        for (float roll : {0.0f, 45.0f, 90.0f}) {
            vec(rdram, addr(K_sViewportRotation), {pitch,yaw,roll});
            auto basis_ctx = context(); basis_ctx.f_odd = &basis_ctx.f0.u32h;
            basis_ctx.f12.fl = 23; basis_ctx.f14.fl = 10000;
            vr_capture_camera(rdram,&basis_ctx); invoke(vr_frame_hook);
            dk64vr_get_game_state(&snapshot);
            float dot = 0, forward2 = 0, up2 = 0;
            for (int i = 0; i < 3; ++i) {
                float forward = snapshot.cam_at[i] - snapshot.cam_eye[i];
                dot += forward * snapshot.cam_up[i];
                forward2 += forward*forward; up2 += snapshot.cam_up[i]*snapshot.cam_up[i];
            }
            close(dot,0); close(forward2,1); close(up2,1);
        }
    // Flare gate: real populated flare state must be untouched and no models emitted.
    MEM_W(4,addr(K_D_80386170)) = 0x80014000; MEM_B(0,addr(K_D_8038617C)) = 1;
    models = 0; invoke(func_80350818); assert(models == 0);
    std::cout << "generated gates, original-ROM flat parity and camera/frame snapshots passed\n";
}
