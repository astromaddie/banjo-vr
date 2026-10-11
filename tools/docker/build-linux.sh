#!/usr/bin/env bash
# Builds Banjo: Recompiled for Linux inside the pinned environment from linux.Dockerfile. The
# target architecture is the Docker host's: arm64 on Apple Silicon, which is the Steam Frame's.
# Build files go to build-linux-<arch>/ in the repository (gitignored), and so do the generated
# recompiler outputs (RecompiledFuncs/, RecompiledPatches/, rsp/n_aspMain.cpp) and the
# decompressed ROM in the repository root. None of them is ever committed.
#
#   BK_ROM="/path/to/Banjo-Kazooie (USA).z64" tools/docker/build-linux.sh [step]
#
# Steps (default: all):
#   image    build the Docker image (tools/docker/linux.Dockerfile)
#   tools    build N64Recomp and RSPRecomp from lib/N64ModernRuntime/N64Recomp
#   rom      decompress the ROM (needs BK_ROM; its sha1 is checked first)
#   recomp   run N64Recomp banjo.us.rev0.toml and RSPRecomp n_aspMain.us.rev0.toml
#   build    CMake + Ninja: the BanjoRecompiled binary
#   package  stage build-linux-<arch>/package/BanjoRecompiled (binary, assets, lib/, launcher)
#   abi      the ABI check on the staged package (fails above glibc 2.35 / GLIBCXX 3.4.30)
#   smoke    start the package under Xvfb + lavapipe for a few seconds (BK_ROM optional)
#   vrtest   build and run the VR bridge's host unit tests
#   all      image, tools, rom, recomp, build, package, abi
#
# JOBS sets the build parallelism (default: every CPU of the container). BK_VR=1 makes the
# OpenXR VR build, in build-linux-<arch>-vr/.
set -euo pipefail

repo="$(cd "$(dirname "$0")/../.." && pwd)"
# The image is the one the DK64 port builds from the same Dockerfile.
image="${BK_VR_IMAGE:-dk64-vr-linux}"
step="${1:-all}"
want_sha1="1fe1632098865f639e22c11b9a81ee8f29c75d7a"

# Passed through only when set, so the container script's own defaults apply otherwise.
env_args=()
for var in BUILD_DIR JOBS BUILD_TYPE SMOKE_SECONDS BK_VR; do
	[[ -z "${!var:-}" ]] || env_args+=(-e "$var=${!var}")
done

in_container() {
	local extra=("$@")
	docker run --rm -v "$repo":/src -w /src ${env_args[@]+"${env_args[@]}"} \
		${extra[@]+"${extra[@]}"} "$image" tools/docker/container-steps.sh "$STEP"
}

run_step() {
	STEP="$1"
	shift
	in_container "$@"
}

check_rom() {
	[[ -n "${BK_ROM:-}" && -f "$BK_ROM" ]] || { echo "set BK_ROM to your US 1.0 .z64" >&2; exit 1; }
	have="$(shasum -a 1 "$BK_ROM" | cut -d' ' -f1)"
	[[ "$have" == "$want_sha1" ]] || { echo "$BK_ROM is not the US ROM (sha1 $have, want $want_sha1)" >&2; exit 1; }
}

prepare_vr_dependencies() {
	bash "$repo/tools/vr/prepare-public-dependencies.sh"
}

case "$step" in
	image) docker build -t "$image" -f "$repo/tools/docker/linux.Dockerfile" "$repo/tools/docker" ;;
	tools | recomp | package | abi) run_step "$step" ;;
	build)
		if [[ "${BK_VR:-0}" == 1 ]]; then prepare_vr_dependencies; fi
		run_step "$step"
		;;
	vrtest)
		prepare_vr_dependencies
		run_step "$step"
		;;
	rom)
		check_rom
		# Read-only mount: the ROM is never copied anywhere on the host.
		run_step rom -v "$BK_ROM":/rom/baserom.us.z64:ro -e ROM_FILE=/rom/baserom.us.z64
		;;
	smoke)
		if [[ -n "${BK_ROM:-}" ]]; then
			check_rom
			run_step smoke -v "$BK_ROM":/rom/baserom.us.z64:ro -e ROM_FILE=/rom/baserom.us.z64
		else
			run_step smoke
		fi
		;;
	all)
		check_rom
		if [[ "${BK_VR:-0}" == 1 ]]; then prepare_vr_dependencies; fi
		"$0" image
		"$0" tools
		"$0" rom
		"$0" recomp
		"$0" build
		"$0" package
		"$0" abi
		;;
	*) echo "unknown step $step" >&2; exit 2 ;;
esac
