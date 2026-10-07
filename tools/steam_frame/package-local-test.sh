#!/usr/bin/env bash
# Build a portable, ROM-bearing local test bundle from an already-built Steam Frame package.
set -euo pipefail

usage() {
  cat <<'USAGE'
Usage: package-local-test.sh --package DIR --rom FILE --output DIR

Stages a local-only test bundle containing the native runtime package and a verified
US 1.0 ROM. The original package remains ROM-free. Run the bundle's install.sh locally
on macOS when you are ready to install it on a Steam Frame.
USAGE
}

package=""
rom=""
out=""
while (($#)); do
  case "$1" in
    --package) (($# >= 2)) || { usage >&2; exit 2; }; package="$2"; shift 2 ;;
    --rom) (($# >= 2)) || { usage >&2; exit 2; }; rom="$2"; shift 2 ;;
    --output) (($# >= 2)) || { usage >&2; exit 2; }; out="$2"; shift 2 ;;
    -h|--help) usage; exit 0 ;;
    *) echo "unknown option: $1" >&2; usage >&2; exit 2 ;;
  esac
done
[[ -n "$package" && -n "$rom" && -n "$out" ]] || { usage >&2; exit 2; }
[[ -d "$package" ]] || { echo "package directory not found: $package" >&2; exit 1; }
[[ -f "$rom" ]] || { echo "ROM file not found: $rom" >&2; exit 1; }
for required in BanjoRecompiled LAUNCH-STEAM-FRAME.sh BUILD-METADATA.json assets recompcontrollerdb.txt; do
  [[ -e "$package/$required" ]] || { echo "package missing required file: $required" >&2; exit 1; }
done
[[ -x "$package/BanjoRecompiled" && -x "$package/LAUNCH-STEAM-FRAME.sh" ]] || {
  echo "package executable or launcher is not executable" >&2; exit 1;
}
command -v python3 >/dev/null || { echo "python3 is required to validate build metadata" >&2; exit 1; }
python3 - "$package" <<'PY'
import hashlib, json, os, sys
root = os.path.realpath(sys.argv[1])
metadata_path = os.path.join(root, "BUILD-METADATA.json")
try:
    with open(metadata_path, encoding="utf-8") as f:
        data = json.load(f)
    revision = data.get("source_revision")
    compiler = data.get("compiler")
    files = data.get("files")
    if not isinstance(revision, str) or not revision.strip() or revision == "unknown" or not isinstance(compiler, str) or not compiler.strip() or not isinstance(files, dict) or not files:
        raise ValueError("source_revision, compiler, and non-empty files mapping are required")
    for required in ("BanjoRecompiled", "LAUNCH-STEAM-FRAME.sh"):
        if required not in files:
            raise ValueError("files mapping is missing " + required)
    actual_names = set()
    for current, dirs, names in os.walk(root, followlinks=False):
        dirs[:] = [d for d in dirs if not os.path.islink(os.path.join(current, d))]
        for name in names:
            path = os.path.join(current, name)
            rel = os.path.relpath(path, root).replace(os.sep, "/")
            if rel != "BUILD-METADATA.json":
                actual_names.add(rel)
    if actual_names != set(files):
        missing = sorted(actual_names - set(files))
        extra = sorted(set(files) - actual_names)
        raise ValueError("files mapping does not match package contents (unrecorded=%r, missing=%r)" % (missing, extra))
    for rel, expected in files.items():
        if not isinstance(rel, str) or rel.startswith("/") or ".." in rel.split("/"):
            raise ValueError("unsafe file path in metadata: %r" % rel)
        if not isinstance(expected, str) or len(expected) != 64:
            raise ValueError("invalid SHA-256 for %s" % rel)
        path = os.path.join(root, *rel.split("/"))
        digest = hashlib.sha256()
        with open(path, "rb") as f:
            for chunk in iter(lambda: f.read(1024 * 1024), b""):
                digest.update(chunk)
        if digest.hexdigest() != expected.lower():
            raise ValueError("package file hash does not match BUILD-METADATA.json: %s" % rel)
except Exception as exc:
    raise SystemExit("invalid or stale package metadata: %s" % exc)
PY
want_sha1=1fe1632098865f639e22c11b9a81ee8f29c75d7a
have_sha1="$(shasum -a 1 "$rom" | awk '{print $1}')"
[[ "$have_sha1" == "$want_sha1" ]] || {
  echo "ROM SHA-1 mismatch: got $have_sha1, expected $want_sha1 (US 1.0)" >&2; exit 1;
}
if find "$package" \( -iname '*.z64' -o -iname '*.n64' -o -iname '*.v64' -o -iname '*.sav' -o -iname 'saves' \) -print | grep -q .; then
  echo "source package contains ROM or save data; refusing to bundle it" >&2
  exit 1
fi
[[ ! -e "$out" ]] || { echo "output already exists: $out" >&2; exit 1; }
parent="$(cd "$(dirname "$out")" && pwd)"
out="$parent/$(basename "$out")"
tmp="$(mktemp -d "$parent/.banjovr-local-test.XXXXXX")"
cleanup() { rm -rf "$tmp"; }
trap cleanup EXIT HUP INT TERM
mkdir -p "$tmp/payload" "$tmp/game-data"
cp -R "$package/." "$tmp/payload/"
cp "$rom" "$tmp/game-data/bk.n64.us.1.0.z64"
cp "$(cd "$(dirname "$0")" && pwd)/install-local.sh" "$tmp/install.sh"
chmod +x "$tmp/install.sh"
cat > "$tmp/README.txt" <<'README'
BanjoVR Steam Frame local test bundle

This folder contains the native ARM64 runtime package, a locally supplied US 1.0 ROM,
and the standalone installer. The ROM and this bundle are for local testing only; do not
commit or redistribute either. BUILD-METADATA.json preserves the source/build identity
from the package. SHA256SUMS covers every bundle file except itself.

Install on macOS with:
  ./install.sh --verify-only
  ./install.sh --dry-run
  ./install.sh [steamos@frame.local]

For a direct USB connection, use:
  ./install.sh steamos@10.86.200.233
An alternate host can also be selected with FRAME_HOST=steamos@192.168.1.31.
The default is steamos@frame.local. The installer copies the runtime to
~/devkit-game/BanjoVR and the ROM to ~/.local/share/BanjoRecompiled. Existing saves,
settings and mods in that data directory are preserved. Replaced runtime files and ROMs
are backed up with SHA-256 records. The installer registers a Steam devkit title using
SteamLinuxRuntime_4-arm64 and steam_play=0. It never starts the game or restarts SteamVR.

After installing, start BanjoVR from the Frame's Steam library. The launcher opens the
normal game menu; choose Banjo-Kazooie and begin a new game. Intro may run before Spiral
Mountain. Check the 45-degree right-stick comfort turn, pause HUD over the frozen stereo
world, and recenter from the system View hold. These behaviors are documented in the
source's docs/VR_ARCHITECTURE.md. This bundle has not been tested on a Frame by its maker.
README
{
  cd "$tmp"
  find payload game-data -type f -print
  printf '%s\n' install.sh README.txt
} | while IFS= read -r f; do
  (cd "$tmp" && shasum -a 256 "$f")
done > "$tmp/SHA256SUMS"
# The manifest is the only generated file not listed in itself.
mkdir -p "$(dirname "$out")"
mv "$tmp" "$out"
trap - EXIT HUP INT TERM
echo "Local test bundle staged at: $out"
echo "Verify it offline with: $out/install.sh --verify-only"
