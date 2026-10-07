#!/usr/bin/env bash
# Standalone macOS-compatible local installer for a BanjoVR local test bundle.
set -euo pipefail

bundle="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
frame_host="${FRAME_HOST:-steamos@frame.local}"
title=BanjoVR
remote_dir="devkit-game/$title"
rom_name=bk.n64.us.1.0.z64
want_sha1=1fe1632098865f639e22c11b9a81ee8f29c75d7a
mode=install

usage() {
  cat <<'USAGE'
Usage: install.sh [--verify-only | --dry-run] [FRAME_HOST]

With no option, installs on FRAME_HOST (default steamos@frame.local). A positional
host overrides FRAME_HOST; the USB address is steamos@10.86.200.233. The local bundle
is always verified before any device connection. --verify-only and --dry-run are offline.
USAGE
}
while (($#)); do
  case "$1" in
    --verify-only) mode=verify; shift ;;
    --dry-run) mode=dry-run; shift ;;
    -h|--help) usage; exit 0 ;;
    --*) echo "unknown option: $1" >&2; usage >&2; exit 2 ;;
    *) frame_host="$1"; shift ;;
  esac
done
[[ "$frame_host" =~ ^[A-Za-z0-9._-]+@[A-Za-z0-9.-]+$ || "$frame_host" =~ ^[A-Za-z0-9.-]+$ ]] || {
  echo "invalid FRAME_HOST; use user@host or host" >&2; exit 2;
}
fail() { echo "install-local: $*" >&2; exit 1; }
for required in SHA256SUMS install.sh README.txt payload/BanjoRecompiled payload/LAUNCH-STEAM-FRAME.sh \
  payload/BUILD-METADATA.json payload/assets payload/recompcontrollerdb.txt "game-data/$rom_name"; do
  [[ -e "$bundle/$required" ]] || fail "bundle missing required file: $required"
done
[[ -x "$bundle/payload/BanjoRecompiled" && -x "$bundle/payload/LAUNCH-STEAM-FRAME.sh" ]] || fail "payload executables are not executable"
command -v shasum >/dev/null || fail "shasum is required"
# Reject absolute paths and parent traversal before feeding the manifest to shasum.
while IFS= read -r line || [[ -n "$line" ]]; do
  rel="${line#*  }"
  [[ "$rel" != "$line" ]] || fail "malformed SHA256SUMS entry"
  [[ "$rel" != /* && "$rel" != *../* && "$rel" != ../* ]] || fail "unsafe manifest path: $rel"
  [[ -f "$bundle/$rel" ]] || fail "manifest file missing: $rel"
done < "$bundle/SHA256SUMS"
manifest_count="$(wc -l < "$bundle/SHA256SUMS" | tr -d ' ')"
file_count="$(find "$bundle" -type f ! -name SHA256SUMS -print | wc -l | tr -d ' ')"
[[ "$manifest_count" == "$file_count" ]] || fail "SHA256SUMS does not cover every bundle file"
manifest_dupes="$(cut -c 67- "$bundle/SHA256SUMS" | sort | uniq -d)"
[[ -z "$manifest_dupes" ]] || fail "SHA256SUMS contains duplicate paths"
(cd "$bundle" && shasum -a 256 -c SHA256SUMS) || fail "bundle checksum verification failed"
rom_sha1="$(shasum -a 1 "$bundle/game-data/$rom_name" | awk '{print $1}')"
[[ "$rom_sha1" == "$want_sha1" ]] || fail "ROM SHA-1 mismatch: got $rom_sha1, expected US 1.0 $want_sha1"
if [[ "$mode" == verify ]]; then
  echo "Offline bundle verification passed (ROM SHA-1 $rom_sha1)."
  exit 0
fi
if [[ "$mode" == dry-run ]]; then
  echo "Offline dry run passed; no device contacted and no files changed."
  echo "Would copy payload to ~/$remote_dir and ROM to ~/.local/share/BanjoRecompiled/$rom_name."
  echo "Would register $title with SteamLinuxRuntime_4-arm64 and steam_play=0."
  exit 0
fi

ssh_opts=(-o ConnectTimeout=5 -o BatchMode=yes)
lock_script=""
for candidate in "$HOME/.codex/skills/steam-frame-vr-port/scripts/frame-lock.sh" "$HOME/.claude/skills/steam-frame-vr-port/scripts/frame-lock.sh"; do
  if [[ -x "$candidate" ]]; then lock_script="$candidate"; break; fi
done
lock_session="banjovr-install-$$"
lock_claimed=0
remote_stage=""
cleanup() {
  status=$?
  if [[ -n "$remote_stage" ]]; then
    ssh "${ssh_opts[@]}" "$frame_host" "rm -rf '$remote_stage'" >/dev/null 2>&1 || true
  fi
  if ((lock_claimed)); then "$lock_script" release "$lock_session" >/dev/null || true; fi
  exit "$status"
}
trap cleanup EXIT HUP INT TERM
if [[ -n "$lock_script" ]]; then
  # The shared lock script stores its lock under ~/code by default and assumes this parent exists.
  mkdir -p "$HOME/code"
  "$lock_script" claim "$lock_session" "install BanjoVR local test bundle" 20 || fail "Steam Frame lock is held; no device changes made"
  lock_claimed=1
fi
ssh "${ssh_opts[@]}" "$frame_host" true 2>/dev/null || fail "Frame is unreachable at $frame_host (USB cable/network and SSH access required)"
remote_stage="$(ssh "${ssh_opts[@]}" "$frame_host" 'mktemp -d "$HOME/.banjovr-local-install.XXXXXX"')"
[[ "$remote_stage" == */.banjovr-local-install.* ]] || fail "unexpected remote staging path: $remote_stage"
COPYFILE_DISABLE=1 tar -C "$bundle" -cf - payload game-data SHA256SUMS | ssh "${ssh_opts[@]}" "$frame_host" "tar -xf - -C '$remote_stage'"

ssh "${ssh_opts[@]}" "$frame_host" bash -s -- "$remote_stage" "$title" <<'REMOTE'
set -euo pipefail
stage="$1"
title="$2"
remote_dir="$HOME/devkit-game/$title"
data_dir="$HOME/.local/share/BanjoRecompiled"
rom_name=bk.n64.us.1.0.z64
# Fail before changing the installation if this Frame lacks the tools used below or
# the devkit shortcut registrar. The command is invoked only after the copy is verified.
for tool in python3 sha256sum sha1sum pgrep cp; do
  command -v "$tool" >/dev/null 2>&1 || { echo "Frame is missing required tool: $tool" >&2; exit 1; }
done
[[ -d "$HOME/devkit-utils" && -f "$HOME/devkit-utils/steam-client-create-shortcut" ]] || {
  echo "Frame is missing devkit-utils/steam-client-create-shortcut" >&2; exit 1;
}
python3 -c 'import sys' >/dev/null 2>&1 || { echo "Frame python3 is not usable" >&2; exit 1; }
# A running title can be damaged by replacing its mapped files.
if pgrep -x BanjoRecompiled >/dev/null 2>&1; then
  echo "BanjoVR is running; close it from Steam before installing." >&2
  exit 1
fi
while IFS= read -r line || [[ -n "$line" ]]; do
  expected="${line%% *}"
  rel="${line#*  }"
  case "$rel" in payload/*|game-data/*) ;; *) continue ;; esac
  actual="$(sha256sum "$stage/$rel" | awk '{print $1}')"
  [[ "$actual" == "$expected" ]] || { echo "staged checksum mismatch: $rel" >&2; exit 1; }
done < "$stage/SHA256SUMS"
mkdir -p "$HOME/devkit-game" "$data_dir"
backup="$HOME/devkit-game/.${title}-backup-$(date +%Y%m%d-%H%M%S)-$$"
mkdir -p "$backup/payload" "$backup/game-data"
if [[ -d "$remote_dir" ]]; then
  # This directory contains runtime payload only; persistent saves/settings/mods live elsewhere.
  cp -a "$remote_dir" "$backup/payload/current"
fi
if [[ -e "$data_dir/$rom_name" || -L "$data_dir/$rom_name" ]]; then cp -p "$data_dir/$rom_name" "$backup/game-data/$rom_name"; fi
if find "$backup" -type f | grep -q .; then
  (cd "$backup" && find payload game-data -type f -exec sha256sum {} \;) > "$backup/SHA256SUMS"
  echo "Backed up overwritten files at $backup"
else
  rmdir "$backup/payload" "$backup/game-data" "$backup"
  backup=""
fi
mkdir -p "$remote_dir"
cp -a "$stage/payload/." "$remote_dir/"
rom_tmp="$data_dir/.${rom_name}.install-$$"
cp "$stage/game-data/$rom_name" "$rom_tmp"
[[ "$(sha1sum "$rom_tmp" | awk '{print $1}')" == 1fe1632098865f639e22c11b9a81ee8f29c75d7a ]] || {
  rm -f "$rom_tmp"; echo "staged ROM identity changed during install" >&2; exit 1;
}
# mv replaces a destination symlink itself instead of following it to another file.
mv -f "$rom_tmp" "$data_dir/$rom_name"
# Verify each installed payload file against its bundle checksum, then verify the ROM identity.
while IFS= read -r line || [[ -n "$line" ]]; do
  expected="${line%% *}"
  rel="${line#*  }"
  case "$rel" in
    payload/*) target="$remote_dir/${rel#payload/}" ;;
    game-data/*) target="$data_dir/${rel#game-data/}" ;;
    *) continue ;;
  esac
  actual="$(sha256sum "$target" | awk '{print $1}')"
  [[ "$actual" == "$expected" ]] || { echo "installed checksum mismatch: $target" >&2; exit 1; }
done < "$stage/SHA256SUMS"
[[ "$(sha1sum "$data_dir/$rom_name" | awk '{print $1}')" == 1fe1632098865f639e22c11b9a81ee8f29c75d7a ]]
cd "$HOME/devkit-utils"
python3 steam-client-create-shortcut --parms '{"gameid": "BanjoVR", "directory": "/home/steamos/devkit-game/BanjoVR", "force_appid": "", "argv": ["LAUNCH-STEAM-FRAME.sh"], "env": {}, "settings": {"steam_play": "0", "compat_tool": "SteamLinuxRuntime_4-arm64"}, "lepton_args": ""}'
echo "Installed and registered BanjoVR. Start it from the Frame's Steam library."
REMOTE
