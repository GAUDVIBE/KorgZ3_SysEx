#!/usr/bin/env python3
"""Verification forte: re-parse le .kicad_pcb resultant et, pour chaque pad
des nouvelles embases J1..J7, calcule sa position monde (origine footprint +
offset local du pad) et verifie:
  (a) elle egale pads_abs de placement.json a 0.05mm pres,
  (b) le nom de net correspond au mapping signal->net attendu.
Echec bruyant a la moindre divergence.
"""
import json, pathlib, re, sys
sys.path.insert(0, str(pathlib.Path(__file__).parent))
from sexpr import find_blocks, prop, block_at
from apply_headers import net_for  # reuse the exact mapping

ROOT = pathlib.Path(__file__).resolve().parent.parent
PCB = ROOT / "SysEx_Patcher.kicad_pcb"
TOL = 0.05

placement = json.loads((ROOT / "shield_redesign" / "placement.json").read_text())
heads = {h["pcb_ref"]: h for h in placement["headers"]}

s = PCB.read_text()
fps = find_blocks(s, '\t(footprint ')

found = {}
for a, b in fps:
    blk = s[a:b]
    r = prop(blk, "Reference")
    if r in heads:
        found[r] = blk

missing = set(heads) - set(found)
if missing:
    sys.exit(f"ECHEC: embases manquantes dans le PCB: {sorted(missing)}")

# Les anciennes embases NON reutilisees doivent avoir disparu (JMA3, JMD4),
# et toutes les embases reutilisees doivent etre des embases Mega (MEGA_HDR).
GONE = {"JMA3", "JMD4"}
for a, b in fps:
    blk = s[a:b]
    r = prop(blk, "Reference")
    if r in GONE:
        sys.exit(f"ECHEC: ancienne embase {r} encore presente")
    if r in heads and prop(blk, "Value") != "MEGA_HDR":
        sys.exit(f"ECHEC: {r} n'est pas une embase Mega (Value!=MEGA_HDR)")

max_err = 0.0
npads = 0
for ref, h in heads.items():
    blk = found[ref]
    X, Y, rotdeg = block_at(blk)
    if abs(rotdeg) > 1e-6:
        sys.exit(f"ECHEC: {ref} rotation footprint != 0 ({rotdeg})")
    # parse pads: num, local at, net
    pad_iter = list(re.finditer(r'\(pad "(\d+)" thru_hole \w+', blk))
    pads = {}
    for m in pad_iter:
        seg = blk[m.start():m.start()+320]
        am = re.search(r'\(at ([-\d.]+) ([-\d.]+)', seg)
        nm = re.search(r'\(net "([^"]*)"\)', seg)
        pads[m.group(1)] = (float(am.group(1)), float(am.group(2)),
                            nm.group(1) if nm else None)
    exp = {p["num"]: p for p in h["pads_abs"]}
    if set(pads) != set(exp):
        sys.exit(f"ECHEC: {ref} pads {sorted(pads)} != attendu {sorted(exp)}")
    for num, (dx, dy, net) in pads.items():
        wx, wy = X + dx, Y + dy
        tx, ty = exp[num]["x"], exp[num]["y"]
        err = max(abs(wx - tx), abs(wy - ty))
        max_err = max(max_err, err)
        npads += 1
        if err > TOL:
            sys.exit(f"ECHEC: {ref} pad {num} monde=({wx:.3f},{wy:.3f}) "
                     f"attendu=({tx:.3f},{ty:.3f}) err={err:.4f}mm")
        # net check
        expected_net = net_for(exp[num]["signal"])
        if net != expected_net:
            sys.exit(f"ECHEC: {ref} pad {num} signal={exp[num]['signal']!r} "
                     f"net={net!r} attendu={expected_net!r}")

print(f"check_headers_world OK — {npads} pads verifies sur 7 embases, "
      f"err_max={max_err:.5f}mm (tol {TOL}mm), nets conformes.")
