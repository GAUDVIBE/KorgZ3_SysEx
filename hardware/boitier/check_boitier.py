#!/usr/bin/env python3
"""Verifie que les percages de boitier.scad tombent en face des composants.

Le fichier .scad porte un bloc « releve sur le PCB » : axes des 16
potentiometres, centres des 7 boutons, des 7 LEDs, trous M3. Ces valeurs ont
ete extraites de SysEx_Patcher.kicad_pcb ; ce script les recalcule depuis la
source et compare. Toute derive du .scad ou du PCB est signalee.

Le piege recurrent de ce projet : l'ancre d'une empreinte n'est PAS le centre
du composant. Les offsets sont donc relus dans les empreintes elles-memes.

  - Alps RK09K  : axe a (7.5, 2.5) de l'ancre, avant rotation
  - SW_PUSH_6mm : centre a (3.25, 2.25) de l'ancre
  - LED_D3.0mm  : centre a (1.27, 0) de l'ancre

Repere du modele :  X = x_kicad - 20    Y = 192 - y_kicad

Usage: python3 boitier/check_boitier.py
"""
import math
import pathlib
import re
import sys

HERE = pathlib.Path(__file__).parent
KI = HERE.parent / "kicad_project"
sys.path.insert(0, str(KI / "shield_redesign"))
from sexpr import find_blocks, prop, block_at  # noqa: E402

PCB = KI / "SysEx_Patcher.kicad_pcb"
SCAD = HERE / "boitier.scad"

OFFSETS = {"RV": (7.5, 2.5), "SW": (3.25, 2.25), "D": (1.27, 0.0)}
TOL = 0.02  # mm — on compare des constantes, pas des mesures


def modele(x, y):
    return (round(x - 20, 3), round(192 - y, 3))


def depuis_pcb():
    src = PCB.read_text()
    out = {}
    for a, b in find_blocks(src, "(footprint "):
        blk = src[a:b]
        ref = prop(blk, "Reference")
        if not ref:
            continue
        fam = re.match(r"([A-Z]+)\d+$", ref)
        if not fam:
            continue
        fam = fam.group(1)
        if fam == "D" and "LED" not in blk[:200]:
            continue  # D1 est une diode, pas une LED
        if fam not in OFFSETS:
            continue
        ox, oy, rt = block_at(blk)
        dx, dy = OFFSETS[fam]
        r = math.radians(rt)
        out[ref] = modele(ox + dx * math.cos(r) - dy * math.sin(r),
                          oy + dx * math.sin(r) + dy * math.cos(r))
    # trous M3
    for a, b in find_blocks(src, "(footprint "):
        blk = src[a:b]
        ref = prop(blk, "Reference")
        if ref and ref.startswith("MH"):
            ox, oy, _ = block_at(blk)
            out[ref] = modele(ox, oy)
    return out


def liste_scad(nom):
    txt = SCAD.read_text()
    m = re.search(nom + r"\s*=\s*(\[.*?\]);", txt, re.S)
    if not m:
        return None
    return [tuple(float(v) for v in p.split(","))
            for p in re.findall(r"\[([^\[\]]+)\]", m.group(1))]


def main():
    pcb = depuis_pcb()
    faute = 0

    # --- potentiometres : le .scad les decrit par une grille croisee
    txt = SCAD.read_text()
    def axe(nom):
        bloc = re.search(nom + r"\s*=\s*\[(.*?)\]", txt, re.S).group(1)
        return sorted({float(v) for v in re.findall(r"[-\d.]+", bloc)})
    grille = {(x, y) for x in axe("POT_X") for y in axe("POT_Y")}
    reels = {pcb[k] for k in pcb if k.startswith("RV")}
    manque = reels - grille
    exces = grille - reels
    print("Potentiometres : %d axes sur le PCB, %d dans la grille du .scad"
          % (len(reels), len(grille)))
    for p in sorted(manque):
        print("   ABSENT du .scad : %s" % (p,)); faute += 1
    for p in sorted(exces):
        print("   EN TROP dans le .scad : %s" % (p,)); faute += 1

    # --- boutons, LEDs, vis
    for nom, pfx, cle in [("Boutons", "SW", "BOUTONS"),
                          ("LEDs", "D", "LEDS"),
                          ("Vis M3", "MH", "VIS")]:
        att = {pcb[k] for k in pcb if k.startswith(pfx)}
        got = {tuple(round(v, 3) for v in p) for p in (liste_scad(cle) or [])}
        print("%-14s : %d sur le PCB, %d dans le .scad" % (nom, len(att), len(got)))
        for p in sorted(att - got):
            proche = min(got, key=lambda q: (q[0]-p[0])**2 + (q[1]-p[1])**2) if got else None
            d = math.dist(p, proche) if proche else 999
            if d <= TOL:
                continue
            print("   ECART : PCB %s, plus proche dans le .scad %s (%.3f mm)"
                  % (p, proche, d)); faute += 1

    print()
    if faute:
        print("ECHEC : %d ecart(s)" % faute)
        return 1
    print("check_boitier OK — les percages tombent en face des composants")
    return 0


if __name__ == "__main__":
    sys.exit(main())
