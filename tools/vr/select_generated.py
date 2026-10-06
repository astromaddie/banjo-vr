#!/usr/bin/env python3
"""Extract unchanged generated functions for the production-path regression harness."""
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / 'build/vr-check'
PATCHED = ['func_802BBA84', 'func_80302C94', 'func_80344138', 'func_80344424',
           'func_80344720', 'viewport_isBoundingBoxInFrustum', 'viewport_func_8024DB50',
           'viewport_isBoundingBoxInFrustumCopy', 'vr_capture_camera', 'vr_frame_hook', 'func_80350818']
ORIGINAL = PATCHED[:5] + ['viewport_isBoundingBoxInFrustum', 'viewport_func_8024DB50', 'func_80350818']
SUPPORT = ['coords_func_8033EAF8', 'ml_vec3f_dot_product', 'mlAbsF']


def extract(text, name):
    match = re.search(r'^RECOMP_FUNC void ' + name + r'\(.*?(?=^RECOMP_FUNC void |\Z)',
                      text, re.M | re.S)
    return match.group(0) if match else None


patched = (ROOT / 'RecompiledPatches/patches.c').read_text()
functions = [extract(patched, name) for name in PATCHED]
assert all(functions), 'regenerate patches first'
needed = set(ORIGINAL + SUPPORT)
for path in (ROOT / 'RecompiledFuncs').glob('*.c'):
    text = path.read_text()
    for name in list(needed):
        body = extract(text, name)
        if body:
            if name in ORIGINAL:
                body = body.replace('void ' + name + '(', 'void baseline_' + name + '(', 1)
            functions.append(body)
            needed.remove(name)
    if not needed:
        break
assert not needed, f'missing original functions {needed}'
text = '\n'.join(functions)
names = set(re.findall(r'^RECOMP_FUNC void (\w+)\(', text, re.M))
deps = set(re.findall(r'^\s+(\w+)\(rdram, ctx\);', text, re.M))
header = '#include "recomp.h"\n#ifdef __cplusplus\nextern "C" {\n#endif\n'
header += ''.join(f'void {name}(uint8_t *, recomp_context *);\n' for name in sorted(names | deps))
header += '#ifdef __cplusplus\n}\n#endif\n'
OUT.mkdir(parents=True, exist_ok=True)
(OUT / 'selected.h').write_text(header)
(OUT / 'selected.c').write_text('#include "selected.h"\n' + text)
print('Generated production functions:', len(names), '; controlled callees:', len(deps - names))

# Resolve global addresses from the live link and the game's pinned data symbol file.
syms = dict(re.findall(r'name = "([^"]+)", vram = (0x[0-9A-Fa-f]+)',
                       (ROOT / 'BanjoRecompSyms/bk.us.rev0.datasyms.toml').read_text()))
for line in (ROOT / 'patches/patches.map').read_text().splitlines():
    match = re.match(r'([0-9a-f]+)\s+[0-9a-f]+\s+[0-9a-f]+\s+\d+\s+(\w+)$', line)
    if match:
        syms[match[2]] = '0x' + match[1]
used = ['sCubeList', 'sViewportPosition', 'sViewportRotation', 'sViewportFOVy',
        'sViewportFrustumPlanes', 'D_8037D8CC', 'D_8037D8C8', 'D_80371EC0',
        'D_80371ECC', 'D_803858B0', 'frustum_checks_enabled', 'vr_sky_drawn',
        'D_80386170', 'D_8038617C']
(OUT / 'test_symbols.h').write_text(''.join(f'#define K_{name} {syms[name]}u\n' for name in used))

section_text = (ROOT / 'BanjoRecompSyms/bk.us.rev0.syms.toml').read_text()
with (OUT / 'test_symbols.h').open('a') as out:
    for tag in ('core1', 'core2'):
        block = re.search(r'name = "\.' + tag + r'"\n(.*?)(?=functions)', section_text, re.S)[1]
        for key in ('rom', 'vram', 'size'):
            value = re.search(key + r' = (0x[0-9A-Fa-f]+)', block)[1]
            out.write(f'#define K_{tag}_{key} {value}u\n')

# Comfort-turn production harness: native R-look follower, real yaw/target initialization,
# VR request gate and the existing tagged camera dispatch. Scenery/collision/input are controlled.
turn_patched = ['vr_apply_comfort_turn', 'vr_comfort_camera_active', 'vr_comfort_camera_held',
                'ncCamera_update', 'ncDynamicCam13_init', 'func_802C069C', 'func_802C02D4',
                'recomp_analog_camera_held', 'func_80290E8C', 'func_80291108', 'func_802911E0']
turn_patched += re.findall(r'^RECOMP_FUNC void (vr_turn_allowed_\w+)\(', patched, re.M)
turn_original = ['ncCamera_update']
turn_support = ['ncDynamicCam13_update', 'func_802BDB30', 'mlDiffDegF', 'mlNormalizeAngle',
                'mlAbsF', 'ml_mapAbsRange_f', 'ml_map_f', 'ml_clamp_f', 'func_802C0640']
turn_functions = [extract(patched, name) for name in turn_patched]
assert all(turn_functions), 'regenerate turn patches first'
needed = set(turn_original + turn_support)
for path in (ROOT / 'RecompiledFuncs').glob('*.c'):
    original_text = path.read_text()
    for name in list(needed):
        body = extract(original_text, name)
        if body:
            if name in turn_original:
                body = body.replace('void ' + name + '(', 'void baseline_' + name + '(', 1)
            turn_functions.append(body)
            needed.remove(name)
    if not needed: break
assert not needed, f'missing turn originals {needed}'
turn_text = '\n'.join(turn_functions)
names = set(re.findall(r'^RECOMP_FUNC void (\w+)\(', turn_text, re.M))
deps = set(re.findall(r'^\s+(\w+)\(rdram, ctx\);', turn_text, re.M))
header = '#include "recomp.h"\n#ifdef __cplusplus\nextern "C" {\n#endif\n'
header += ''.join(f'void {name}(uint8_t *, recomp_context *);\n' for name in sorted(names | deps))
header += '#ifdef __cplusplus\n}\n#endif\n'
(OUT / 'selected_turn.h').write_text(header)
(OUT / 'selected_turn.c').write_text('#include "selected_turn.h"\n' + turn_text)
with (OUT / 'test_symbols.h').open('a') as out:
    for name in ['D_8037C060','D_8037C062','D_8037DB40','D_8037DBA0','D_8037DBA4','D_8037DBA8',
                 'D_8037DBAC','D_8037D9E0','D_8037D9C8','D_8037D8C5','D_8037D8C6','ncCameraType',
                 'cameraUpdateEnabled','vr_turn_remaining','vr_turn_last_yaw',
                 'dynamic_camera_target_frame','dynamic_camera_target_frames','dynamic_camera_targets',
                 'r_look_initialized_from_r_button','r_look_get_target_from_previous_frame_fix',
                 'analog_inherited_distance']:
        out.write(f'#define K_{name} {syms[name]}u\n')
print('Generated comfort-turn functions:', len(names), '; controlled callees:', ', '.join(sorted(deps-names)))
