#!/usr/bin/env python3
"""Verifie l'ordre physique de la rangee analogique sur le PCB.

Sur un vrai Arduino Mega 2560, en parcourant la rangee analogique par x
croissant on lit:
    A7 A6 A5 A4 A3 A2 A1 A0 A15 A14 A13 A12 A11 A10 A9 A8
(A0..A7 puis A8..A15, chaque groupe en ordre decroissant de pin).

Ce test echouait sur la v0.9 (rangee monotone A0..A15). Apres Task 3, les
embases J3 (A0..A7) et J5 (A8..A15) sont placees aux positions reelles du
template, donc l'ordre ci-dessus doit etre respecte.

On lit directement le PCB: pour chaque pad des embases analogiques, position
monde = origine footprint + offset local, et le signal = nom de net (les nets
analogiques A0..A15 portent le meme nom que le signal).
"""
import pathlib, re, sys
sys.path.insert(0, str(pathlib.Path(__file__).parent))
from sexpr import find_blocks, prop, block_at

ROOT = pathlib.Path(__file__).resolve().parent.parent
PCB = ROOT / "SysEx_Patcher.kicad_pcb"

# Embases portant la rangee analogique apres Task 3 (refs PCB reelles:
# template J3 -> JMA1 (A0..A7), template J5 -> JMA2 (A8..A15)).
ANALOG_REFS = ["JMA1", "JMA2"]

EXPECTED = ["A7", "A6", "A5", "A4", "A3", "A2", "A1", "A0",
            "A15", "A14", "A13", "A12", "A11", "A10", "A9", "A8"]

s = PCB.read_text()
fps = find_blocks(s, '\t(footprint ')

pads = []  # (world_x, net_name)
for a, b in fps:
    blk = s[a:b]
    if prop(blk, "Reference") not in ANALOG_REFS:
        continue
    X, Y, _ = block_at(blk)
    for m in re.finditer(r'\(pad "(\d+)" thru_hole \w+', blk):
        seg = blk[m.start():m.start()+320]
        am = re.search(r'\(at ([-\d.]+) ([-\d.]+)', seg)
        nm = re.search(r'\(net "([^"]*)"\)', seg)
        if not nm:
            continue
        net = nm.group(1)
        if re.fullmatch(r'A\d+', net):
            pads.append((X + float(am.group(1)), net))

if not pads:
    sys.exit("ECHEC: aucun pad analogique trouve (J3/J5 absentes ?)")

pads.sort(key=lambda t: t[0])  # x croissant
order = [net for _, net in pads]

if order != EXPECTED:
    print("ECHEC ordre rangee analogique:")
    print("  obtenu  :", order)
    print("  attendu :", EXPECTED)
    sys.exit(1)

print("check_pinorder OK — ordre rangee analogique (x croissant) =", " ".join(order))
