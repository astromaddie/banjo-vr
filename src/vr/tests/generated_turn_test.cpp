// Actual generated Banjo patch + original ROM R-look follower. Only external scene,
// scenery/collision and controller samples are controlled; yaw easing is game code.
#include <atomic>
#include <array>
#include <cassert>
#include <cmath>
#include <cstring>
#include <fstream>
#include <iostream>
#include <vector>
#include "selected_turn.h"
#include "test_symbols.h"
#include "../include/dk64vr_game_state.h"
#include "../include/dk64vr_control.h"

std::atomic_bool exited{false};
static gpr addr(uint32_t a) { return static_cast<int32_t>(a); }
static float &f(uint8_t *m, uint32_t a) { return *reinterpret_cast<float *>(m+a-0x80000000u); }
static std::array<float,3> position{100,180,850}, rotation{}, target{100,80,300};
static int mode=3, camera=2, dynamic=0xB, fp=0, movement=0, map=1, action=1, water=0;
static bool present=true, analog=false, demo=false, node=false, cLeft=false, cRight=false, rButton=false;
static int logs=0, nativeCalls=0;
static std::vector<int> trace;
static float dt=1.0f/30;
static recomp_context context() { recomp_context c{}; c.r29=addr(0x80FFF000); return c; }
static recomp_context call(recomp_func_t *fn, uint8_t *m, gpr a=0) {
    auto c=context(); c.r4=a; c.f_odd=&c.f0.u32h; fn(m,&c); return c;
}
static void put(uint8_t *m, gpr a, const std::array<float,3> &v) {
    std::memcpy(m+static_cast<uint32_t>(a)-0x80000000u,v.data(),12);
}
static std::array<float,3> get(uint8_t *m, gpr a) {
    std::array<float,3> v; std::memcpy(v.data(),m+static_cast<uint32_t>(a)-0x80000000u,12); return v;
}
static float normal(float yaw) { return yaw-360*std::floor(yaw/360); }
static float diff(float a,float b) { return normal(a-b+180)-180; }
#define STUB(name) extern "C" void name(uint8_t *rdram,recomp_context *ctx)
#define RET(name,value) STUB(name) { (void)rdram; ctx->r2=(value); }
RET(getGameMode,mode) RET(map_get,map) RET(level_get,11) RET(ncCamera_getType,camera)
RET(ncDynamicCamera_getState,dynamic) RET(ncFirstPersonCamera_getState,fp)
RET(bs_getState,action) RET(player_is_present,present) RET(player_movementGroup,movement)
RET(gctransition_active,0) RET(player_getWaterState,water)
RET(bainput_should_rotate_camera_left,cLeft) RET(bainput_should_rotate_camera_right,cRight)
RET(bakey_held,rButton) RET(recomp_in_demo_playback_game_mode,demo)
RET(recomp_get_analog_cam_enabled,analog) RET(func_80290D48,node) RET(func_8029105C,0)
RET(func_802BE60C,0) RET(func_802BC84C,0)
#define FLOATRET(name,value) STUB(name) { (void)rdram; ctx->f0.fl=(value); }
FLOATRET(time_getDelta,dt) FLOATRET(func_802BD8D4,550) FLOATRET(func_802C0780,180)
FLOATRET(player_getYPosition,0)
STUB(recomp_get_analog_inverted_axes) { MEM_W(0,ctx->r4)=0; MEM_W(0,ctx->r5)=0; }
STUB(recomp_get_right_analog_inputs) { f(rdram,uint32_t(ctx->r4))=0; f(rdram,uint32_t(ctx->r5))=0; }
STUB(gu_sqrtf) { (void)rdram; ctx->f0.fl=std::sqrt(ctx->f12.fl); }
STUB(ml_vec3f_copy) { put(rdram,ctx->r4,get(rdram,ctx->r5)); }
STUB(ml_vec3f_clear) { put(rdram,ctx->r4,{0,0,0}); }
STUB(ml_vec3f_diff_copy) {
    auto a=get(rdram,ctx->r5),b=get(rdram,ctx->r6);
    put(rdram,ctx->r4,{a[0]-b[0],a[1]-b[1],a[2]-b[2]});
}
STUB(ml_vec3f_add) {
    auto a=get(rdram,ctx->r5),b=get(rdram,ctx->r6);
    put(rdram,ctx->r4,{a[0]+b[0],a[1]+b[1],a[2]+b[2]});
}
STUB(ml_vec3f_distance) {
    auto a=get(rdram,ctx->r4),b=get(rdram,ctx->r5); float d=0;
    for(int i=0;i<3;++i) d+=(a[i]-b[i])*(a[i]-b[i]); ctx->f0.fl=std::sqrt(d);
}
STUB(ncDynamicCamera_getPosition) { put(rdram,ctx->r4,position); }
STUB(ncDynamicCamera_getRotation) { put(rdram,ctx->r4,rotation); }
STUB(ncDynamicCamera_setPosition) { position=get(rdram,ctx->r4); }
STUB(ncDynamicCamera_setRotation) { rotation=get(rdram,ctx->r4); }
STUB(ncDynamicCamera_setState) {
    const int next=int(ctx->r4);
    if (next!=dynamic && next==0x13) call(ncDynamicCam13_init,rdram);
    dynamic=next;
}
STUB(func_80291488) { MEM_B(0,addr(K_D_8037C062))=ctx->r4; }
STUB(func_802C0150) { MEM_B(0,addr(K_D_8037DB40))=ctx->r4; }
#define TARGET(name) STUB(name) { put(rdram,ctx->r4,target); }
TARGET(func_802C019C) TARGET(func_802C01BC) TARGET(func_802C01DC)
TARGET(func_802C0234) TARGET(func_802C026C) TARGET(func_802C02B4)
STUB(func_8025801C) {
    auto v=get(rdram,ctx->r4); f(rdram,uint32_t(ctx->r5))=normal(std::atan2(v[0],v[2])*57.2957795f);
}
STUB(func_80256E24) {
    float yaw; uint32_t word=uint32_t(ctx->r6); std::memcpy(&yaw,&word,4);
    const float radius=f(rdram,uint32_t(ctx->r29)+20);
    const float rad=yaw*0.01745329252f;
    put(rdram,ctx->r4,{std::sin(rad)*radius,0,std::cos(rad)*radius});
}
STUB(func_802BE6FC) { put(rdram,ctx->r4,{0,f(rdram,K_D_8037DBA8),0}); }
STUB(func_802C095C) { (void)ctx; f(rdram,K_D_8037DBA4)=180; }
STUB(recomp_printf) { (void)rdram; (void)ctx; ++logs; }
#define NOOP(name) STUB(name) { (void)rdram; (void)ctx; }
NOOP(func_802BE230) NOOP(func_802BE244) NOOP(func_802C2264) NOOP(func_802C1B20)
NOOP(func_80290F14) NOOP(recomp_analog_camera_update)
// The native dispatch and its original-ROM oracle see the same observable callback sequence.
STUB(ncDynamicCamera_update) {
    ++nativeCalls; trace.push_back(2);
    if(dynamic==0x13) call(ncDynamicCam13_update,rdram);
    (void)ctx;
}
#define TRACE(name,value) STUB(name) { (void)rdram; (void)ctx; trace.push_back(value); }
TRACE(ncStaticCamera_update,3) TRACE(ncRandomCamera_update,4)
TRACE(func_802BB4D8,5) TRACE(viewport_update,6) TRACE(func_802BEFB0,7) TRACE(func_802BBA84,8)
STUB(viewport_getPosition_vec3f) { put(rdram,ctx->r4,position); }
STUB(viewport_getRotation_vec3f) { put(rdram,ctx->r4,rotation); }
STUB(viewport_setPosition_vec3f) { position=get(rdram,ctx->r4); }
STUB(viewport_setRotation_vec3f) { rotation=get(rdram,ctx->r4); }
extern "C" recomp_func_t *get_function(int32_t) { assert(false); return nullptr; }
extern "C" void switch_error(const char*,uint32_t,uint32_t) { assert(false); }
extern "C" void do_break(uint32_t) { assert(false); }

int main() {
    std::vector<uint8_t> memory(32*1024*1024); auto *rdram=memory.data();
    auto load=[&](const char *file,uint32_t rom,uint32_t vram,uint32_t size) {
        std::ifstream in(file,std::ios::binary); assert(in); in.seekg(rom);
        std::vector<uint8_t> data(size); in.read(reinterpret_cast<char*>(data.data()),size); assert(in);
        for(uint32_t i=0;i<size;++i) memory[(vram-0x80000000u+i)^3]=data[i];
    };
    load("banjo.us.v10.decompressed.z64",K_core1_rom,K_core1_vram,K_core1_size);
    load("banjo.us.v10.decompressed.z64",K_core2_rom,K_core2_vram,K_core2_size);
    std::ifstream patch("patches/patches.bin",std::ios::binary|std::ios::ate); assert(patch);
    load("patches/patches.bin",0,0x80801000u,uint32_t(patch.tellg()));
    MEM_B(0,addr(K_D_8037D8C5))=1;
    MEM_B(0,addr(K_cameraUpdateEnabled))=1;
    MEM_B(0,addr(K_D_8037DB40))=2;
    MEM_B(0,addr(K_D_8037C062))=2;
    // Inactive gate cannot consume input, change yaw, query scene/control, or log.
    dk64vr_set_active(0); dk64vr_request_turn(45);
    f(rdram,K_D_8037DBA4)=123; call(vr_apply_comfort_turn,rdram,2);
    assert(f(rdram,K_D_8037DBA4)==123 && logs==0 && dk64vr_take_turn()==45);
    for (int type : {0,1,2,3,4}) {
        camera=type; MEM_W(0,addr(K_ncCameraType))=type;
        trace.clear(); call(baseline_ncCamera_update,rdram); const auto before=trace;
        trace.clear(); call(ncCamera_update,rdram); assert(trace==before);
    }
    camera=2; MEM_W(0,addr(K_ncCameraType))=2;
    dk64vr_set_active(1); call(vr_apply_comfort_turn,rdram,2); assert(dk64vr_turn_applicable());
    dk64vr_request_turn(45); call(vr_apply_comfort_turn,rdram,2);
    assert(dynamic==0x13 && std::fabs(diff(f(rdram,K_D_8037DBA4),315))<0.001f);
    assert(f(rdram,K_analog_inherited_distance)==550); // works with desktop analog disabled
    // The existing previous-focus initialization fix must also work with desktop analog off.
    // Eye belongs to the old focus; recomputing yaw from a moving player's new focus drifts.
    MEM_W(0,addr(K_dynamic_camera_target_frame))=55;
    MEM_W(0,addr(K_dynamic_camera_target_frames))=54;
    put(rdram,addr(K_dynamic_camera_targets),target);
    target[0]+=100;
    MEM_W(0,addr(K_r_look_get_target_from_previous_frame_fix))=1;
    call(func_802C069C,rdram);
    assert(std::fabs(f(rdram,K_D_8037DBA8))<0.001f);
    target[0]-=100;
    MEM_W(0,addr(K_r_look_get_target_from_previous_frame_fix))=0;
    MEM_W(0,addr(K_dynamic_camera_target_frame))=0;
    assert(call(vr_comfort_camera_held,rdram).r2==1);
    float seconds=0;
    while(std::fabs(diff(f(rdram,K_D_8037DBA8),315))>0.01f && seconds<8) {
        call(ncCamera_update,rdram); seconds+=dt;
    }
    assert(seconds<8);
    std::cout << "native R-look 45 deg arrival <0.01 deg: " << seconds << " s (30 Hz, no collision)\n";
    assert(std::fabs(rotation[1]-f(rdram,K_D_8037DBA8))<0.001f);
    // Free swimming/flying (including bee) use the existing analog selector retention.
    dk64vr_request_turn(45); call(vr_apply_comfort_turn,rdram,2);
    water=2; assert(call(func_80290E8C,rdram).r2==1 && dynamic==0x13);
    water=0; dynamic=4; call(vr_apply_comfort_turn,rdram,2);
    dk64vr_request_turn(45); call(vr_apply_comfort_turn,rdram,2);
    call(func_80291108,rdram); call(func_802911E0,rdram); assert(dynamic==0x13);
    // More than one full turn of debt must steer the native follower in the same direction.
    dk64vr_request_turn(720); call(vr_apply_comfort_turn,rdram,2);
    assert(f(rdram,K_vr_turn_remaining)<-700);
    assert(diff(f(rdram,K_D_8037DBA4),f(rdram,K_D_8037DBA8))<0);
    int largeTicks=0;
    while(std::fabs(f(rdram,K_vr_turn_remaining))>=0.01f && largeTicks<239) {
        call(ncCamera_update,rdram);
        assert(diff(f(rdram,K_D_8037DBA4),f(rdram,K_D_8037DBA8))<=0.02f);
        ++largeTicks;
    }
    assert(std::fabs(f(rdram,K_vr_turn_remaining))<0.01f && largeTicks<239);
    std::cout << "native unwrapped debt arrival: " << largeTicks*dt << " s\n";
    dk64vr_request_turn(45); call(vr_apply_comfort_turn,rdram,2);
    position[0]+=500; call(vr_apply_comfort_turn,rdram,2);
    assert(f(rdram,K_vr_turn_remaining)==0);
    assert(f(rdram,K_D_8037DBA4)==f(rdram,K_D_8037DBA8));
    assert(call(vr_comfort_camera_held,rdram).r2==0); // no old heading imposed across a cut
    rButton=true; dk64vr_request_turn(45); call(vr_apply_comfort_turn,rdram,2);
    assert(!dk64vr_turn_applicable()); rButton=false;
    cLeft=true; dk64vr_request_turn(45); call(vr_apply_comfort_turn,rdram,2);
    assert(!dk64vr_turn_applicable()); cLeft=false;
    // Locked/node/action cameras, boss map, first person, missing player, and disabled dispatch
    // reject queued turns and reset host applicability. Returning to play cannot replay them.
    for(int excluded : {0,5,6,8,9,10,12,13,15,16,17,18}) {
        dynamic=excluded; dk64vr_request_turn(45);
        call(vr_apply_comfort_turn,rdram,2); assert(!dk64vr_turn_applicable());
        dynamic=0xB; MEM_B(0,addr(K_D_8037C062))=2;
        call(vr_apply_comfort_turn,rdram,2); assert(dk64vr_take_turn()==0);
    }
    for(int type : {0,1,3,4}) { call(vr_apply_comfort_turn,rdram,type); assert(!dk64vr_turn_applicable()); }
    for(int nonplay : {0,1,2,4,7,8,9,10}) {
        mode=nonplay; call(vr_apply_comfort_turn,rdram,2); assert(!dk64vr_turn_applicable());
    }
    mode=3; fp=1; call(vr_apply_comfort_turn,rdram,2); assert(!dk64vr_turn_applicable());
    fp=0; present=false; call(vr_apply_comfort_turn,rdram,2); assert(!dk64vr_turn_applicable());
    present=true; map=0x90; call(vr_apply_comfort_turn,rdram,2); assert(!dk64vr_turn_applicable());
    map=1; MEM_B(0,addr(K_cameraUpdateEnabled))=0;
    call(vr_apply_comfort_turn,rdram,2); assert(!dk64vr_turn_applicable());
    std::cout << "generated turn import, native heading/follower, water/flight retention, rejection and flat dispatch parity passed\n";
}
