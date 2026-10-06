#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/../.."
check_dir=build/vr-check
python3 tools/vr/select_generated.py
"${CC:-clang}" ${VR_TEST_FLAGS:-} -std=c11 -w -ffp-contract=off -fno-strict-aliasing \
    -Ilib/N64ModernRuntime/N64Recomp/include -I"$check_dir" \
    -c "$check_dir/selected.c" -o "$check_dir/selected.o"
"${CXX:-clang++}" ${VR_TEST_FLAGS:-} -std=c++20 -pthread -Wall -Wextra -Werror -ffp-contract=off \
    -Ilib/N64ModernRuntime/N64Recomp/include -Ilib/N64ModernRuntime/librecomp/include \
    -Ilib/N64ModernRuntime/ultramodern/include -I"$check_dir" \
    src/game/vr_api.cpp src/vr/tests/generated_gate_test.cpp "$check_dir/selected.o" \
    -o "$check_dir/generated_gate_test"
BK_VR_FORCE_PATCHES=0 "$check_dir/generated_gate_test"
