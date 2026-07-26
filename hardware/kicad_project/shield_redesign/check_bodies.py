#!/usr/bin/env python3
"""Controle geometrique des corps de composants sur le bord haut.

ATTENTION — convention KiCad, source d'une erreur d'analyse le 2026-07-26 :
la position absolue d'un element d'empreinte est `origine + rotation(local)`,
SANS negation de y. Un controle qui ecrit `oy - local_y` place les composants
en miroir vertical et invente des collisions inexistantes (c'est ce qui a fait
croire que J1 touchait l'optocoupleur U1, alors qu'il en est a 12 mm).

On mesure les corps reels : les fp_line/fp_rect de serigraphie quand ils
existent, sinon l'enveloppe des pastilles.
"""
import sys, re, math, pathlib

sys.path.insert(0, str(pathlib.Path(__file__).parent))
from sexpr import find_blocks, prop, block_at

PCB = pathlib.Path("SysEx_Patcher.kicad_pcb").read_text()
MARGE = 1.5          # mm de jeu exige entre deux corps
TETE_VIS = 3.0       # rayon d'une tete de vis M3


def rot_xy(x, y, deg):
    r = math.radians(deg)
    return (x * math.cos(r) - y * math.sin(r), x * math.sin(r) + y * math.cos(r))


def enveloppe(blk):
    """Boite englobante absolue : serigraphie si presente, sinon pastilles."""
    ox, oy, rot = block_at(blk)
    locaux = []
    for m in re.finditer(r'\(fp_line\s*\(start ([-\d.]+) ([-\d.]+)\)\s*\(end ([-\d.]+) ([-\d.]+)\)', blk):
        locaux += [(float(m.group(1)), float(m.group(2))), (float(m.group(3)), float(m.group(4)))]
    for m in re.finditer(r'\(fp_circle\s*\(center ([-\d.]+) ([-\d.]+)\)\s*\(end ([-\d.]+) ([-\d.]+)\)', blk):
        cx, cy, ex, ey = (float(m.group(i)) for i in (1, 2, 3, 4))
        r = math.hypot(ex - cx, ey - cy)
        locaux += [(cx - r, cy - r), (cx + r, cy + r)]
    if not locaux:
        for a, b in find_blocks(blk, "(pad "):
            m = re.search(r'\(at ([-\d.]+) ([-\d.]+)', blk[a:b])
            if m:
                locaux.append((float(m.group(1)), float(m.group(2))))
    if not locaux:
        return None
    pts = [(ox + dx, oy + dy) for dx, dy in (rot_xy(x, y, rot) for x, y in locaux)]
    xs = [q[0] for q in pts]
    ys = [q[1] for q in pts]
    return (min(xs), max(xs), min(ys), max(ys))


corps = {}
for a, b in find_blocks(PCB, "(footprint "):
    blk = PCB[a:b]
    ref = prop(blk, "Reference")
    at = block_at(blk)
    if not ref or not at or at[1] > 80:
        continue
    e = enveloppe(blk)
    if e:
        corps[ref] = e

print("Corps du bord haut (y < 80) :")
for ref in sorted(corps, key=lambda r: corps[r][0]):
    x0, x1, y0, y1 = corps[ref]
    print("  %-5s x %6.1f..%6.1f   y %6.1f..%6.1f" % (ref, x0, x1, y0, y1))

print("\nCollisions (marge exigee %.1f mm) :" % MARGE)
refs = sorted(corps)
faute = 0
for i, r1 in enumerate(refs):
    for r2 in refs[i + 1:]:
        a1, b1, c1, d1 = corps[r1]
        a2, b2, c2, d2 = corps[r2]
        dx = max(a2 - b1, 0, a1 - b2)
        dy = max(c2 - d1, 0, c1 - d2)
        if math.hypot(dx, dy) < MARGE:
            print("  ATTENTION %s <-> %s : %.2f mm" % (r1, r2, math.hypot(dx, dy)))
            faute += 1
if not faute:
    print("  aucune")

print("\nJeu des embases MIDI vis-a-vis des vis de fixation :")
for din in ("J1", "J2"):
    if din not in corps:
        continue
    x0, x1, y0, y1 = corps[din]
    for mh in ("MH1", "MH2", "MH3", "MH4"):
        if mh not in corps:
            continue
        cx = (corps[mh][0] + corps[mh][1]) / 2
        cy = (corps[mh][2] + corps[mh][3]) / 2
        dx = max(x0 - cx, 0, cx - x1)
        dy = max(y0 - cy, 0, cy - y1)
        d = math.hypot(dx, dy) - TETE_VIS
        if d < 25:
            print("  %s <-> %s : %+.2f mm" % (din, mh, d))

print("\nEntraxe MIDI IN <-> MIDI OUT : %.1f mm"
      % abs((corps["J2"][0] + corps["J2"][1]) / 2 - (corps["J1"][0] + corps["J1"][1]) / 2))
sys.exit(1 if faute else 0)
