#!/usr/bin/env bash
# Offline integration coverage for the real bundle maker and installer.
set -euo pipefail
repo_tools="$(cd "$(dirname "$0")" && pwd)"
tmp="$(mktemp -d)"
cleanup() { rm -rf "$tmp"; }
trap cleanup EXIT HUP INT TERM
mkdir -p "$tmp/bin" "$tmp/source/assets" "$tmp/source/lib" "$tmp/local-home" "$tmp/frame-home/devkit-utils"
printf 'native executable fixture\n' > "$tmp/source/BanjoRecompiled"
printf '#!/usr/bin/env bash\n' > "$tmp/source/LAUNCH-STEAM-FRAME.sh"
printf 'asset\n' > "$tmp/source/assets/asset with spaces.txt"
printf 'controller db\n' > "$tmp/source/recompcontrollerdb.txt"
printf 'library\n' > "$tmp/source/lib/libfixture.so"
chmod +x "$tmp/source/BanjoRecompiled" "$tmp/source/LAUNCH-STEAM-FRAME.sh"
/usr/bin/python3 - "$tmp/source" <<'PYTHON'
import hashlib, json, os, sys
root=sys.argv[1]
files={}
for current, dirs, names in os.walk(root):
    for name in names:
        rel=os.path.relpath(os.path.join(current,name),root).replace(os.sep,'/')
        if rel != 'BUILD-METADATA.json':
            with open(os.path.join(root,rel),'rb') as f: files[rel]=hashlib.sha256(f.read()).hexdigest()
with open(os.path.join(root,'BUILD-METADATA.json'),'w') as f:
    json.dump({'source_revision':'test-revision','compiler':'test compiler','tree':'clean','files':files},f)
PYTHON
printf 'owned test ROM\n' > "$tmp/test-rom.z64"
cat > "$tmp/bin/shasum" <<'SHASUM'
#!/usr/bin/env bash
if [[ "${1:-}" == -a && "${2:-}" == 1 ]]; then
  last=""
  for arg in "$@"; do last="$arg"; done
  case "$(basename "$last")" in test-rom.z64|bk.n64.us.1.0.z64) printf '1fe1632098865f639e22c11b9a81ee8f29c75d7a  %s\n' "$last" ;; *) exec /usr/bin/shasum -a 1 "$last" ;; esac
else
  exec /usr/bin/shasum "$@"
fi
SHASUM
cat > "$tmp/bin/sha1sum" <<'SHA1'
#!/usr/bin/env bash
last=""
for arg in "$@"; do last="$arg"; done
case "$(basename "$last")" in .bk.n64.us.1.0.z64.install-*) printf '1fe1632098865f639e22c11b9a81ee8f29c75d7a  %s\n' "$last" ;; *) exec /usr/bin/shasum -a 1 "$last" ;; esac
SHA1
cat > "$tmp/bin/ssh" <<'SSH'
#!/usr/bin/env bash
set -euo pipefail
while [[ "${1:-}" == -o ]]; do shift 2; done
host="$1"; shift
printf '%s\n' "$*" >> "$FAKE_SSH_LOG"
remote_home="$FAKE_FRAME_HOME"
if [[ "$1" == true ]]; then exit 0; fi
if [[ "$1" == mktemp* ]]; then
  path="$(mktemp -d "$remote_home/.banjovr-local-install.XXXXXX")"
  printf '%s\n' "$path"
  exit 0
fi
if [[ "$1" == tar\ * ]]; then
  dest="${1##* -C }"; dest="${dest#\'}"; dest="${dest%\'}"
  tar -xf - -C "$dest"
  exit 0
fi
if [[ "$1" == bash && "$2" == -s ]]; then
  shift
  HOME="$remote_home" PATH="$PATH" bash "$@"
  exit $?
fi
if [[ "$1" == rm\ -rf\ * ]]; then
  dest="${1#rm -rf }"; dest="${dest#\'}"; dest="${dest%\'}"
  rm -rf "$dest"
  exit 0
fi
echo "unexpected mocked ssh command from $host: $*" >&2
exit 90
SSH
cat > "$tmp/bin/pgrep" <<'PGREP'
#!/usr/bin/env bash
[[ "${FAKE_RUNNING_TITLE:-0}" == 1 ]] && exit 0
exec /usr/bin/pgrep "$@"
PGREP
cat > "$tmp/bin/python3" <<'PYTHON'
#!/usr/bin/env bash
if [[ "${1:-}" == -c ]]; then exit 0; fi
if [[ "${1:-}" == steam-client-create-shortcut ]]; then
  shift
  printf '%s\n' "$*" > "$FAKE_REGISTRATION_LOG"
  exit 0
fi
exec /usr/bin/python3 "$@"
PYTHON
chmod +x "$tmp/bin/ssh" "$tmp/bin/shasum" "$tmp/bin/sha1sum" "$tmp/bin/python3" "$tmp/bin/pgrep"
export PATH="$tmp/bin:$PATH"
export HOME="$tmp/local-home"
export FAKE_FRAME_HOME="$tmp/frame-home"
export FAKE_REGISTRATION_LOG="$tmp/registration.txt"
export FAKE_SSH_LOG="$tmp/ssh.log"

bundle="$tmp/bundle with spaces"
"$repo_tools/package-local-test.sh" --package "$tmp/source" --rom "$tmp/test-rom.z64" --output "$bundle"
printf 'wrong ROM\n' > "$tmp/bad-rom.z64"
if "$repo_tools/package-local-test.sh" --package "$tmp/source" --rom "$tmp/bad-rom.z64" --output "$tmp/bad-bundle" >/dev/null 2>&1; then
  echo 'wrong ROM unexpectedly passed package validation' >&2; exit 1
fi
cp -R "$tmp/source" "$tmp/tampered-source"
printf 'tampered\n' >> "$tmp/tampered-source/assets/asset with spaces.txt"
if "$repo_tools/package-local-test.sh" --package "$tmp/tampered-source" --rom "$tmp/test-rom.z64" --output "$tmp/tampered-bundle" >/dev/null 2>&1; then
  echo 'metadata-mismatched package unexpectedly passed validation' >&2; exit 1
fi
"$bundle/install.sh" --verify-only
[[ ! -s "$FAKE_SSH_LOG" ]] || { echo 'offline verification contacted SSH' >&2; exit 1; }
"$bundle/install.sh" --dry-run >/dev/null
[[ ! -s "$FAKE_SSH_LOG" ]] || { echo 'offline dry-run contacted SSH' >&2; exit 1; }
# Both offline modes must fail before SSH when required input is absent or altered.
cp -R "$bundle" "$tmp/missing-bundle"
rm "$tmp/missing-bundle/game-data/bk.n64.us.1.0.z64"
if "$tmp/missing-bundle/install.sh" --dry-run >/dev/null 2>&1; then echo 'missing ROM unexpectedly passed' >&2; exit 1; fi
[[ ! -s "$FAKE_SSH_LOG" ]] || { echo 'missing-file check contacted SSH' >&2; exit 1; }
cp -R "$bundle" "$tmp/corrupt-bundle"
printf 'tampered\n' >> "$tmp/corrupt-bundle/payload/assets/asset with spaces.txt"
if "$tmp/corrupt-bundle/install.sh" --dry-run >/dev/null 2>&1; then echo 'corrupt payload unexpectedly passed' >&2; exit 1; fi
[[ ! -s "$FAKE_SSH_LOG" ]] || { echo 'corrupt-file check contacted SSH' >&2; exit 1; }

# Seed an existing installation and persistent data. The local installer must preserve data,
# back up overwritten files with SHA-256 records, register the native devkit title, and verify.
mkdir -p "$FAKE_FRAME_HOME/devkit-game/BanjoVR" "$FAKE_FRAME_HOME/.local/share/BanjoRecompiled/saves"
printf 'older runtime\n' > "$FAKE_FRAME_HOME/devkit-game/BanjoVR/BanjoRecompiled"
printf 'older ROM bytes through symlink\n' > "$tmp/external-rom-data"
ln -s "$tmp/external-rom-data" "$FAKE_FRAME_HOME/.local/share/BanjoRecompiled/bk.n64.us.1.0.z64"
printf 'keep save\n' > "$FAKE_FRAME_HOME/.local/share/BanjoRecompiled/saves/keep.sav"
printf 'keep settings\n' > "$FAKE_FRAME_HOME/.local/share/BanjoRecompiled/settings.json"
printf '#!/usr/bin/env bash\nexit 0\n' > "$FAKE_FRAME_HOME/devkit-utils/steam-client-create-shortcut"
"$bundle/install.sh" steamos@frame.local
cmp "$bundle/payload/BanjoRecompiled" "$FAKE_FRAME_HOME/devkit-game/BanjoVR/BanjoRecompiled"
cmp "$bundle/game-data/bk.n64.us.1.0.z64" "$FAKE_FRAME_HOME/.local/share/BanjoRecompiled/bk.n64.us.1.0.z64"
[[ ! -L "$FAKE_FRAME_HOME/.local/share/BanjoRecompiled/bk.n64.us.1.0.z64" ]]
[[ "$(cat "$tmp/external-rom-data")" == 'older ROM bytes through symlink' ]]
[[ "$(cat "$FAKE_FRAME_HOME/.local/share/BanjoRecompiled/saves/keep.sav")" == 'keep save' ]]
[[ "$(cat "$FAKE_FRAME_HOME/.local/share/BanjoRecompiled/settings.json")" == 'keep settings' ]]
backup="$(find "$FAKE_FRAME_HOME/devkit-game" -maxdepth 1 -type d -name '.BanjoVR-backup-*' -print | head -1)"
[[ -n "$backup" && -f "$backup/payload/current/BanjoRecompiled" && -f "$backup/game-data/bk.n64.us.1.0.z64" && -f "$backup/SHA256SUMS" ]]
(cd "$backup" && shasum -a 256 -c SHA256SUMS)
grep -q 'SteamLinuxRuntime_4-arm64' "$FAKE_REGISTRATION_LOG"
grep -q 'steam_play' "$FAKE_REGISTRATION_LOG"

# Missing device registration tools fail before replacing either existing file.
missing_home="$tmp/frame-missing-tools"
mkdir -p "$missing_home/devkit-game/BanjoVR" "$missing_home/.local/share/BanjoRecompiled"
printf 'old runtime preserved\n' > "$missing_home/devkit-game/BanjoVR/BanjoRecompiled"
printf 'old ROM preserved\n' > "$missing_home/.local/share/BanjoRecompiled/bk.n64.us.1.0.z64"
FAKE_FRAME_HOME="$missing_home" "$bundle/install.sh" steamos@frame.local >/dev/null 2>&1 && {
  echo 'missing registrar unexpectedly passed preflight' >&2; exit 1;
}
[[ "$(cat "$missing_home/devkit-game/BanjoVR/BanjoRecompiled")" == 'old runtime preserved' ]]
[[ "$(cat "$missing_home/.local/share/BanjoRecompiled/bk.n64.us.1.0.z64")" == 'old ROM preserved' ]]

# Refusing a running title also leaves its installation and data intact.
running_home="$tmp/frame-running"
mkdir -p "$running_home/devkit-utils" "$running_home/devkit-game/BanjoVR" "$running_home/.local/share/BanjoRecompiled"
printf 'old runtime preserved\n' > "$running_home/devkit-game/BanjoVR/BanjoRecompiled"
printf 'old ROM preserved\n' > "$running_home/.local/share/BanjoRecompiled/bk.n64.us.1.0.z64"
printf 'registrar\n' > "$running_home/devkit-utils/steam-client-create-shortcut"
FAKE_FRAME_HOME="$running_home" FAKE_RUNNING_TITLE=1 "$bundle/install.sh" steamos@frame.local >/dev/null 2>&1 && {
  echo 'running title unexpectedly passed preflight' >&2; exit 1;
}
[[ "$(cat "$running_home/devkit-game/BanjoVR/BanjoRecompiled")" == 'old runtime preserved' ]]
[[ "$(cat "$running_home/.local/share/BanjoRecompiled/bk.n64.us.1.0.z64")" == 'old ROM preserved' ]]
# A first installation must create both the runtime and persistent data directories.
cold_home="$tmp/frame-cold"
mkdir -p "$cold_home/devkit-utils"
printf 'registrar\n' > "$cold_home/devkit-utils/steam-client-create-shortcut"
FAKE_FRAME_HOME="$cold_home" "$bundle/install.sh" steamos@frame.local >/dev/null
cmp "$bundle/payload/BanjoRecompiled" "$cold_home/devkit-game/BanjoVR/BanjoRecompiled"
cmp "$bundle/game-data/bk.n64.us.1.0.z64" "$cold_home/.local/share/BanjoRecompiled/bk.n64.us.1.0.z64"
[[ ! -L "$cold_home/.local/share/BanjoRecompiled/bk.n64.us.1.0.z64" ]]
echo 'local installer integration checks passed'
