#!/usr/bin/env python3
"""Vérifie que CHAQUE pastille d'embase porte le net de la broche Mega qu'elle touche.

⚠️ HISTORIQUE — ce fichier attendait auparavant l'ordre analogique
`A7 A6 … A0 | A15 … A8`, c'est-à-dire l'ordre INVERSÉ. Il avait été écrit à
partir de la même erreur que le générateur, donc il passait au vert sur la carte
v1.2 fabriquée, dont la masse atterrissait sur RESET et le +5 V sur la sortie
3V3. Un test qui partage la croyance fausse du code qu'il vérifie ne vérifie rien.

Il lit désormais son attendu dans `mega_pinout.py`, l'unique source de vérité,
partagée avec `fix_mega_pinorder.py` — les deux ne peuvent plus diverger. Et il
couvre les 6 blocs, pas seulement la rangée analogique.

Usage : python3 shield_redesign/check_pinorder.py   (depuis hardware/kicad_project/)
"""
import pathlib, re, sys

sys.path.insert(0, str(pathlib.Path(__file__).parent))
from sexpr import find_blocks, prop, block_at
from mega_pinout import BLOCS, net_attendu

ROOT = pathlib.Path(__file__).resolve().parent.parent
PCB = ROOT / "SysEx_Patcher.kicad_pcb"


def pads_of(blk, X, Y):
    out = []
    for m in re.finditer(r'\t\t\(pad "(\d+)" thru_hole', blk):
        depth, i = 0, m.start()
        while True:
            if blk[i] == "(":
                depth += 1
            elif blk[i] == ")":
                depth -= 1
                if depth == 0:
                    break
            i += 1
        seg = blk[m.start():i + 1]
        at = re.search(r'\(at ([-\d.]+) ([-\d.]+)', seg)
        nm = re.search(r'\(net "([^"]*)"\)', seg)
        out.append((m.group(1), X + float(at.group(1)), Y + float(at.group(2)),
                    nm.group(1) if nm else None))
    return out


def main():
    s = PCB.read_text()
    blocs = {}
    for a, b in find_blocks(s, '\t(footprint '):
        blk = s[a:b]
        ref = prop(blk, "Reference")
        if ref in BLOCS:
            X, Y, _ = block_at(blk)
            blocs[ref] = pads_of(blk, X, Y)

    manquants = set(BLOCS) - set(blocs)
    if manquants:
        sys.exit(f"ECHEC: embases absentes du PCB : {sorted(manquants)}")

    # Origine x_rel = pastille de plus petit x du bloc POWER, qui est la 1re
    # broche (x_rel 0) par définition du bloc. Ancrage PUREMENT GÉOMÉTRIQUE :
    # s'ancrer sur un net (VIN_RAW) reviendrait à supposer vrai ce qu'on teste,
    # et rendrait les échecs illisibles sur une carte mal affectée.
    origine = min(p[1] for p in blocs["JMP1"])

    erreurs = []
    total = 0
    for ref in sorted(BLOCS):
        for num, x, _y, net in sorted(blocs[ref], key=lambda p: p[1]):
            total += 1
            xr = x - origine
            try:
                broche, attendu = net_attendu(ref, xr)
            except ValueError as e:
                erreurs.append(f"{ref} pad {num}: {e}")
                continue
            if net != attendu:
                erreurs.append(
                    f"{ref} pad {num} (x_rel {xr:+8.3f}, broche Mega {broche}): "
                    f"net={net} attendu={attendu}")

    if erreurs:
        print(f"ECHEC ordre des broches — {len(erreurs)} pastille(s) sur {total} :")
        for e in erreurs:
            print("  ", e)
        sys.exit(1)

    print(f"check_pinorder OK — {total} pastilles, les 6 blocs correspondent "
          f"broche à broche au Mega 2560 réel.")


if __name__ == "__main__":
    main()
