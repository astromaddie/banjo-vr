#include <atomic>
#include <cmath>
#include <cstdio>
#include <initializer_list>
#include "dk64vr_turn.h"
#include "dk64vr_math.h"
#include "dk64vr_input.h"
#include "dk64vr_control.h"
#include "dk64vr_game_state.h"
#include "../../../patches/vr_turn_rules.h"

std::atomic_bool exited{false};
static int checks = 0, failures = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++failures; std::printf("FAIL line %d: %s\n", __LINE__, #c); } } while (0)
#define CLOSE(a,b) CHECK(std::fabs((a)-(b)) < 0.0003f)

int main() {
    using namespace dk64vr;
    constexpr float pi = 3.14159265359f;
    TurnRequests queue;
    queue.add(45, 1); queue.add(-15, 1.01);
    CHECK(queue.take(true, 1.1) == 30 && queue.take(true, 1.1) == 0);
    queue.add(45, 2); CHECK(queue.take(false, 2.1) == 0);
    CHECK(queue.take(true, 2.2) == 0); // rejection cannot land later
    queue.add(45, 3); CHECK(queue.take(true, 3.301) == 0);

    // Actual host import, not a test replica: inactive, paused, rejected, accepted, one-shot.
    recomp_context ctx{};
    auto poll = [&](int applicable) { ctx.r4 = applicable; recomp_vr_take_turn(nullptr, &ctx); return int(ctx.r2); };
    dk64vr_set_active(0); dk64vr_request_turn(45); CHECK(poll(1) == 0);
    CHECK(!dk64vr_turn_applicable());
    dk64vr_set_active(1); CHECK(poll(1) == 0 && dk64vr_turn_applicable());
    dk64vr_request_turn(45); CHECK(poll(1) == 45 && poll(1) == 0);
    dk64vr_request_turn(45); CHECK(poll(0) == 0 && !dk64vr_turn_applicable());
    CHECK(poll(1) == 0);
    dk64vr_request_turn(45); dk64vr_set_paused(1); CHECK(poll(1) == 0);
    dk64vr_set_paused(0); CHECK(poll(1) == 0);
    dk64vr_request_turn(45); dk64vr_set_active(0); dk64vr_set_active(1); CHECK(poll(1) == 0);

    for (int dynamic = 0; dynamic <= 0x13; ++dynamic) {
        const bool free = dynamic == 1 || dynamic == 3 || dynamic == 4 || dynamic == 0xB || dynamic == 0x13;
        CHECK(bool(vr_turn_camera_allowed(VR_SCENE_GAMEPLAY,1,1,dynamic,2,0,1)) == free);
    }
    for (int scene = 0; scene < VR_SCENE_COUNT; ++scene)
        CHECK(bool(vr_turn_camera_allowed(scene,1,1,0xB,2,0,1)) == (scene == VR_SCENE_GAMEPLAY));
    for (int mode : {0,5,6,7,8,9,10}) CHECK(!vr_turn_camera_allowed(1,1,1,0xB,mode,0,1));
    CHECK(!vr_turn_camera_allowed(1,0,1,0xB,2,0,1));
    CHECK(!vr_turn_camera_allowed(1,1,0,0xB,2,0,1));
    CHECK(!vr_turn_camera_allowed(1,1,1,0xB,2,BSGROUP_5_CLIMB,1));
    CHECK(!vr_turn_camera_allowed(1,1,1,0xB,2,0,MAP_90_GL_BATTLEMENTS));

    ComfortTurn turn;
    CHECK(turn.apply(45,0,true) == 0); // wait for a valid rendered heading
    turn.update(0,0,true,false);
    CHECK(turn.apply(45,0,true) == 45);
    turn.update(0,0,true,false); CLOSE(turn.shownDelta,-pi/4);
    // Native camera arrives gradually; display heading stays at the target on every frame.
    for (int i=1; i<100; ++i) {
        const float game = -pi/4 * i/100;
        turn.update(game,i/100.0,true,false);
        CLOSE(turn.heading+turn.shownDelta,-pi/4);
        const Vec3 pivot{100,80,300};
        Pose pose; pose.p = {100,180,850}; pose.q = quatFromYaw(game);
        const Mat4 orbit = orbitViewAround(rigidInverse(poseMatrix(pose)),turn.shownDelta,pivot);
        CLOSE(viewHeading(orbit),-pi/4);
        const Vec3 eye = viewPosition(orbit);
        CLOSE(std::sqrt((eye.x-pivot.x)*(eye.x-pivot.x)+(eye.z-pivot.z)*(eye.z-pivot.z)),550.0f);
    }
    CHECK(turn.update(-pi/4,1,true,false) == TurnEnd::Arrived && !turn.active);

    // Main/world and sky cameras have different origins. With a translated world, the same
    // camera-space correction must match an orbit about the sky's correctly rebased focus.
    // Include smoothing and lower/closer offsets before turning, as the renderer does.
    for (float origin : {0.0f, 1000.0f, -4000.0f}) {
        Pose raw; raw.p = {origin + 100, 180, origin + 850}; raw.q = quatFromYaw(0.3f);
        Pose smoothed = raw; smoothed.p.x += 10; smoothed.q = quatFromYaw(0.4f);
        const Mat4 rawView = rigidInverse(poseMatrix(raw));
        const Mat4 smoothView = rigidInverse(poseMatrix(smoothed));
        const Vec3 pivot{origin + 100,80,origin + 300};
        const Mat4 adjusted = thirdPersonView(smoothView,pivot,0.5f,0.2f);
        Pose sky = raw; sky.p = {0,0,0};
        const Mat4 preTurn = mul(mul(rigidInverse(poseMatrix(sky)),mul(rigidInverse(rawView),smoothView)),
            mul(rigidInverse(smoothView),adjusted));
        const Vec3 skyPivot{pivot.x-raw.p.x,pivot.y-raw.p.y,pivot.z-raw.p.z};
        for (float angle : {-pi/4, pi/2, -8*pi/3}) {
            const Mat4 correction = orbitViewCorrection(adjusted,angle,pivot);
            const Mat4 mainShown = mul(adjusted,correction);
            const Mat4 skyShown = mul(preTurn,correction);
            const Mat4 mainExpected = orbitViewAround(adjusted,angle,pivot);
            const Mat4 skyExpected = orbitViewAround(preTurn,angle,skyPivot);
            for (int r=0; r<4; ++r) for (int c=0; c<4; ++c) {
                CHECK(std::fabs(mainShown.m[r][c]-mainExpected.m[r][c]) < 0.003f);
                CHECK(std::fabs(skyShown.m[r][c]-skyExpected.m[r][c]) < 0.003f);
            }
        }
    }

    // Smooth turn across multiple wraps; shown rate/stop are independent of native easing.
    turn.reset(); turn.update(0,0,true,false);
    int whole = 0;
    for (int i=1; i<=480; ++i) {
        whole += turn.apply(1, i/120.0, true);
        turn.update(wrapAngle(-i*0.008f),i/120.0,true,false);
        CLOSE(turn.heading+turn.shownDelta,-i*pi/180);
    }
    CHECK(whole == 480 && std::fabs(turn.target) > 2*pi);
    const float stopped = turn.target;
    for (int i=481; i<=600; ++i) {
        turn.update(wrapAngle(-i*0.008f),i/120.0,true,false);
        CLOSE(turn.heading+turn.shownDelta,stopped);
    }
    CHECK(turn.update(turn.raw,5,false,false) == TurnEnd::NotApplicable);
    CHECK(!turn.active && turn.shownDelta == 0 && turn.remainder == 0);
    CHECK(turn.apply(45,5,false) == 0);
    turn.reset(); turn.update(0,0,true,false);
    turn.apply(45,0,true); turn.update(0,0,true,false);
    CHECK(turn.update(0,8.1,true,false) == TurnEnd::TimedOut);
    turn.update(0,8.3,true,false); CLOSE(turn.shownDelta,-pi/8);
    turn.update(0,8.6,true,false); CLOSE(turn.shownDelta,0);
    // Abandoning multi-revolution debt must not replay those revolutions during the unwind.
    turn.reset(); turn.update(0,0,true,false); turn.apply(765,0,true);
    turn.update(0,0,true,false);
    CHECK(turn.update(0,8.1,true,false) == TurnEnd::TimedOut);
    CLOSE(wrapAngle(turn.shownDelta),-pi/4);
    CHECK(std::fabs(turn.shownDelta) <= pi);
    float previousShown = turn.shownDelta;
    for (int i=1; i<=20; ++i) {
        turn.update(0,8.1+i*0.02,true,false);
        CHECK(std::fabs(wrapAngle(turn.shownDelta-previousShown)) < 0.1f);
        CHECK(std::fabs(turn.shownDelta) <= pi);
        previousShown = turn.shownDelta;
    }
    CLOSE(turn.shownDelta,0);
    turn.apply(45,9,true); CHECK(turn.update(0,9,true,true) == TurnEnd::Overtaken);
    turn.reset(); CHECK(!turn.headingValid && turn.shownDelta == 0);
    // Whole-degree import carries subdegree increments without bias across direction changes.
    turn.update(0,0,true,false); whole = 0;
    for (int i=0; i<100; ++i) whole += turn.apply(0.2f,0,true);
    CHECK(whole == 20);
    for (int i=0; i<100; ++i) whole += turn.apply(-0.2f,0,true);
    CHECK(whole == 0);
    // A low-speed smooth hold must accumulate subdegree input across display frames.
    // Each increment is smaller than the arrival tolerance, before the game can follow.
    turn.reset(); turn.update(0,0,true,false); whole = 0;
    for (int i=1; i<=120; ++i) {
        const double now = i/120.0;
        whole += turn.apply(0.125f,now,true);
        turn.update(0,now,true,false);
        CLOSE(turn.heading+turn.shownDelta,-i*0.125f*pi/180);
    }
    CHECK(whole == 15 && turn.active);
    float x=0,y=1; rotateStick(-pi/4,x,y); CLOSE(x,std::sqrt(0.5f)); CLOSE(y,std::sqrt(0.5f));
    std::printf("comfort turn: %d checks, %d failures\n",checks,failures);
    return failures ? 1 : 0;
}
