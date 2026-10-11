# Host unit tests for the VR bridge (with DK64's profile, test_profile_dk64.c), buildable on their own (no ROM, no recompiled code):
#   cmake -S src/vr/tests -B build-vrtest && cmake --build build-vrtest && build-vrtest/dk64vr_tests
cmake_minimum_required(VERSION 3.20)
project(dk64vr_tests LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD 11)

# The production host tests use assert() for their checks. Keep them live even when the caller
# selects Release (as the Docker wrapper and public CI do).
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-UNDEBUG)
endif()

set(DK64VR_ROOT ${CMAKE_CURRENT_LIST_DIR}/../../..)
add_executable(dk64vr_tests
    ${CMAKE_CURRENT_LIST_DIR}/dk64vr_tests.cpp
    ${DK64VR_ROOT}/src/vr/bridge/dk64vr_math.cpp
    ${DK64VR_ROOT}/src/vr/bridge/dk64vr_camsmooth.cpp
    ${DK64VR_ROOT}/src/vr/bridge/dk64vr_foveation.cpp
    ${DK64VR_ROOT}/src/vr/bridge/dk64vr_sched.cpp
    ${DK64VR_ROOT}/src/vr/bridge/dk64vr_settings.c
    ${DK64VR_ROOT}/src/vr/bridge/dk64vr_log.c
    ${DK64VR_ROOT}/src/vr/bridge/dk64vr_input.cpp
    ${DK64VR_ROOT}/src/vr/menu/dk64vr_menu.cpp
    ${CMAKE_CURRENT_LIST_DIR}/test_profile_dk64.c
    ${DK64VR_ROOT}/src/vr/game/profile_ext_default.c
    ${DK64VR_ROOT}/third_party/sfvr/src/sfvr_settings.c
    ${DK64VR_ROOT}/third_party/sfvr/src/sfvr_adjust.c
    ${DK64VR_ROOT}/third_party/sfvr/src/sfvr_camera.c
    ${DK64VR_ROOT}/third_party/sfvr/src/sfvr_view.c
    ${DK64VR_ROOT}/third_party/sfvr/src/sfvr_turn.c
    ${DK64VR_ROOT}/third_party/sfvr/src/sfvr_haptics.c)
target_include_directories(dk64vr_tests PRIVATE ${DK64VR_ROOT}/src/vr/include ${DK64VR_ROOT}/src/vr/bridge ${DK64VR_ROOT}/src/vr/menu ${DK64VR_ROOT}/third_party/sfvr/include ${DK64VR_ROOT}/lib/rt64/src)
target_compile_options(dk64vr_tests PRIVATE -Wall -Wextra -ffp-contract=off)
target_link_libraries(dk64vr_tests PRIVATE m)
# The same layer with Banjo's profile.
add_executable(banjo_profile_tests
    ${CMAKE_CURRENT_LIST_DIR}/banjo_profile_tests.cpp
    ${DK64VR_ROOT}/src/vr/game/banjo_profile.c
    ${DK64VR_ROOT}/src/vr/game/banjo_profile_ext.c
    ${DK64VR_ROOT}/src/vr/bridge/dk64vr_settings.c
    ${DK64VR_ROOT}/src/vr/bridge/dk64vr_log.c
    ${DK64VR_ROOT}/src/vr/bridge/dk64vr_input.cpp
    ${DK64VR_ROOT}/src/vr/menu/dk64vr_menu.cpp
    ${DK64VR_ROOT}/third_party/sfvr/src/sfvr_settings.c
    ${DK64VR_ROOT}/third_party/sfvr/src/sfvr_view.c
    ${DK64VR_ROOT}/third_party/sfvr/src/sfvr_turn.c
    ${DK64VR_ROOT}/third_party/sfvr/src/sfvr_haptics.c)
target_include_directories(banjo_profile_tests PRIVATE ${DK64VR_ROOT}/src/vr/include ${DK64VR_ROOT}/src/vr/bridge ${DK64VR_ROOT}/src/vr/menu ${DK64VR_ROOT}/third_party/sfvr/include ${DK64VR_ROOT}/lib/rt64/src)
target_compile_options(banjo_profile_tests PRIVATE -Wall -Wextra -ffp-contract=off)
target_link_libraries(banjo_profile_tests PRIVATE m)

enable_testing()
add_test(NAME dk64vr_tests COMMAND dk64vr_tests)
add_test(NAME banjo_profile_tests COMMAND banjo_profile_tests)

add_executable(comfort_turn_tests
    ${CMAKE_CURRENT_LIST_DIR}/comfort_turn_tests.cpp
    ${DK64VR_ROOT}/src/game/vr_api.cpp
    ${DK64VR_ROOT}/src/vr/game/vr_control.cpp
    ${DK64VR_ROOT}/src/vr/bridge/dk64vr_turn.cpp
    ${DK64VR_ROOT}/src/vr/bridge/dk64vr_math.cpp
    ${DK64VR_ROOT}/third_party/sfvr/src/sfvr_camera.c
    ${DK64VR_ROOT}/src/vr/bridge/dk64vr_input.cpp
    ${DK64VR_ROOT}/third_party/sfvr/src/sfvr_view.c
    ${DK64VR_ROOT}/third_party/sfvr/src/sfvr_turn.c)
target_include_directories(comfort_turn_tests PRIVATE
    ${DK64VR_ROOT}/src/vr/include ${DK64VR_ROOT}/src/vr/bridge
    ${DK64VR_ROOT}/third_party/sfvr/include ${DK64VR_ROOT}/lib/rt64/src
    ${DK64VR_ROOT}/lib/N64ModernRuntime/N64Recomp/include
    ${DK64VR_ROOT}/lib/N64ModernRuntime/librecomp/include
    ${DK64VR_ROOT}/lib/N64ModernRuntime/ultramodern/include)
target_compile_options(comfort_turn_tests PRIVATE -Wall -Wextra -ffp-contract=off
    -iquote${DK64VR_ROOT}/lib/bk-decomp/include)
target_link_libraries(comfort_turn_tests PRIVATE m)
add_test(NAME comfort_turn_tests COMMAND comfort_turn_tests)
set_property(TARGET comfort_turn_tests PROPERTY CXX_STANDARD 20)

add_executable(rt64_vr_eye_slots_test ${DK64VR_ROOT}/lib/rt64/src/tests/rt64_vr_eye_slots_test.cpp)
target_include_directories(rt64_vr_eye_slots_test PRIVATE ${DK64VR_ROOT}/lib/rt64/src)
target_compile_options(rt64_vr_eye_slots_test PRIVATE -Wall -Wextra)
add_test(NAME rt64_vr_eye_slots_test COMMAND rt64_vr_eye_slots_test)
