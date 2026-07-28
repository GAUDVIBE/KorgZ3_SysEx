#!/usr/bin/env python3
"""Verifie que les connecteurs du shield tombent en face d'un VRAI Arduino Mega.

REMPLACE check_headers_world.py, qui validait contre le gabarit `Arduino_Mega`
livre avec KiCad. Ce gabarit place le bloc de 10 broches (JP6) et le bloc 2x18
(XIO31) a des positions FAUSSES : s'y fier a produit une v1.0 et une v1.1 qui
ne s'enfichaient pas, alors qu'un script annoncait « err_max = 0.00 mm ». Il
verifiait fidelement la mauvaise reference.

La reference est desormais `mega2560_reel.json`, extraite d'un modele KiCad de
Mega 2560 Rev3 reel. Recoupement independant : la carte de baritonomarchetto,
fabriquee et qui s'enfiche, coincide avec cette reference a 0,25 mm pres — un
ecart absorbe par le jeu des broches dans les trous.

Ancrage : la 1re broche du connecteur d'ALIMENTATION. Toutes les positions sont
relatives, ce qui rend la verification independante du placement de la carte.

Usage: python3 shield_redesign/check_mega_fit.py
"""
import json, math, pathlib, re, sys

HERE = pathlib.Path(__file__).parent
sys.path.insert(0, str(HERE))
from sexpr import find_blocks, prop, block_at

PCB = HERE.parent / "SysEx_Patcher.kicad_pcb"
REF = json.loads((HERE / "mega2560_reel.json").read_text())

# Correspondance entre les connecteurs du Mega et ceux du shield.
# XIO31 (le 2x18) est absent : le shield ne l'implante pas, ses 6 signaux
# utiles ayant ete reportes sur D14-D19, libres sur le bloc JMD3.
PAIRES = [
    ("POWER31", "JMP1"),
    ("ADCL31", "JMA1"),
    ("ADCH31", "JMA2"),
    ("JP6", "JMD1"),
    ("PWML31", "JMD2"),
    ("COMMUNICATION31", "JMD3"),
]
TOLERANCE = 0.30  # mm — jeu reel d'une broche dans un trou de 1,0 mm


def rot(x, y, deg):
    r = math.radians(deg)
    return (x * math.cos(r) - y * math.sin(r), x * math.sin(r) + y * math.cos(r))


def blocs_du_pcb():
    src = PCB.read_text()
    out = {}
    for a, b in find_blocks(src, "(footprint "):
        blk = src[a:b]
        ref = prop(blk, "Reference")
        if not ref or not ref.startswith("JM"):
            continue
        ox, oy, rt = block_at(blk)
        pts = []
        for pa, pb in find_blocks(blk, "(pad "):
            m = re.search(r"\(at ([-\d.]+) ([-\d.]+)", blk[pa:pb])
            if m:
                dx, dy = rot(float(m.group(1)), float(m.group(2)), rt)
                pts.append((ox + dx, oy + dy))
        if pts:
            out[ref] = pts
    return out


def main():
    pcb = blocs_du_pcb()
    manquants = [n for _, n in PAIRES if n not in pcb]
    if manquants:
        print("ECHEC : connecteurs absents du PCB :", ", ".join(manquants))
        return 1

    x0 = min(x for x, y in pcb["JMP1"])
    y0 = pcb["JMP1"][0][1]
    faute = 0
    print("Ecart au Mega 2560 Rev3 reel (ancrage : 1re broche d'alimentation)\n")
    print("%-18s%-8s%10s%10s%10s" % ("bloc Mega", "shield", "attendu", "mesure", "ecart"))
    for rm, rn in PAIRES:
        att = REF["blocs"][rm]["x_min"]
        mes = round(min(x for x, y in pcb[rn]) - x0, 3)
        d = mes - att
        etat = "OK" if abs(d) <= TOLERANCE else "  <-- HORS TOLERANCE"
        print("%-18s%-8s%10.2f%10.2f%+10.3f  %s" % (rm, rn, att, mes, d, etat))
        if abs(d) > TOLERANCE:
            faute += 1
        # entraxe des rangees
        dy = round(pcb[rn][0][1] - y0, 3)
        att_y = REF["blocs"][rm]["y"][0]
        if abs(dy - att_y) > TOLERANCE:
            print("     ECART EN Y : attendu %.2f, mesure %.2f" % (att_y, dy))
            faute += 1

    if "JMX1" in pcb:
        print("\nATTENTION : JMX1 (2x18) est present. Sa position reelle traverse la")
        print("colonne de potentiometres RV3/RV7/RV11/RV15 — court-circuits garantis.")
        faute += 1

    print()
    if faute:
        print("ECHEC : %d ecart(s)" % faute)
        return 1
    print("check_mega_fit OK — les 6 connecteurs tombent en face du Mega")
    return 0


if __name__ == "__main__":
    sys.exit(main())
