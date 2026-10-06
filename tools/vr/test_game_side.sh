#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/../.."
check_dir=build/vr-check
mkdir -p "$check_dir"
"${CC:-clang}" -c src/vr/game/banjo_profile.c -o "$check_dir/banjo_profile.o"
"${CXX:-clang++}" -std=c++20 -pthread -Wall -Wextra -Werror \
    -Ilib/N64ModernRuntime/N64Recomp/include \
    -Ilib/N64ModernRuntime/librecomp/include \
    -Ilib/N64ModernRuntime/ultramodern/include -iquote lib/bk-decomp/include \
    src/game/vr_api.cpp src/vr/tests/game_side_test.cpp "$check_dir/banjo_profile.o" \
    -o "$check_dir/game_side_test"
BK_VR_FORCE_PATCHES=0 "$check_dir/game_side_test"
BK_VR_FORCE_PATCHES=1 "$check_dir/game_side_test" forced
