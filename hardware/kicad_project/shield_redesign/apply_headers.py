#!/usr/bin/env python3
"""Task 3 — embases Mega aux positions reelles du template + bloc 2x18 (J7).

Convention-free construction: chaque pad recoit un offset local explicite
(dx,dy) = (wx-X, wy-Y) de sorte que origine+local == pads_abs EXACTEMENT.
Les empreintes male d'origine (JMA*/JMD*) sont supprimees et remplacees par
J1..J7 (embases male) placees aux positions derivees du template Arduino Mega.

Format net: ce PCB (KiCad v20260206) utilise (net "NOM") sans table de nets
de premier niveau ni id entier. On ecrit donc directement (net "NOM").
"""
import json, pathlib, sys, uuid
sys.path.insert(0, str(pathlib.Path(__file__).parent))
from sexpr import find_blocks, prop, block_at

ROOT = pathlib.Path(__file__).resolve().parent.parent
PCB = ROOT / "SysEx_Patcher.kicad_pcb"

# Les 7 embases Mega actuellement sur le PCB (anciennes positions miroir).
OLD_REFS = ["JMP1", "JMA1", "JMA2", "JMD1", "JMD2", "JMD3", "JMX1"]

# Cle template (placement.json) -> reference reelle sur le PCB.
TKEY_TO_PCBREF = {
    "J1": "JMP1",   # power 1x8
    "J3": "JMA1",   # analog A0-A7
    "J5": "JMA2",   # analog A8-A15
    "J2": "JMD1",   # digital 1x10
    "J4": "JMD2",   # digital
    "J6": "JMD3",   # digital
    "J7": "JMX1",   # 2x18
}

# signal (cleaned Mega name, from placement.json) -> existing net NAME spelling.
# Built from reading the old strip pads (exact spellings the PCB uses).
# Signals NOT present here get either: a new single-pad net named after the
# signal (Dxx, AREF), or NO net for the J1 NC pins (handled below).
SIGMAP = {
    # analog
    **{f"A{i}": f"A{i}" for i in range(16)},
    # digital that already exist
    "D0": "D0_RX", "D1": "D1_TX",
    **{f"D{i}": f"D{i}" for i in range(2, 14)},
    "D20": "D20_SDA", "D21": "D21_SCL",
    # power
    "GND": "GND", "+5V": "+5V",
    "VCC": "VIN_RAW",   # J1 pad8 sits at the old VIN position
}
# Signals that must NOT receive a net (J1 NC / not-routed pins on old board).
NO_NET = {"unconnected-(J1-Pin_1-Pad1)", "IOREF", "~{RESET}", "+3V3"}

# Footprint library id per header ref (for BOM/identification).
LIBID = {
    "J1": "Connector_PinHeader_2.54mm:PinHeader_1x08_P2.54mm_Vertical",
    "J3": "Connector_PinHeader_2.54mm:PinHeader_1x08_P2.54mm_Vertical",
    "J5": "Connector_PinHeader_2.54mm:PinHeader_1x08_P2.54mm_Vertical",
    "J2": "Connector_PinHeader_2.54mm:PinHeader_1x10_P2.54mm_Vertical",
    "J4": "Connector_PinHeader_2.54mm:PinHeader_1x08_P2.54mm_Vertical",
    "J6": "Connector_PinHeader_2.54mm:PinHeader_1x08_P2.54mm_Vertical",
    "J7": "Connector_PinHeader_2.54mm:PinHeader_2x18_P2.54mm_Vertical",
}

# Pad shape copied verbatim from an existing strip pad (JMA2):
PAD_SIZE = "1.7 1.7"
PAD_DRILL = "1"
PAD_LAYERS = '"*.Cu" "*.Mask"'


def net_for(signal):
    """Return the net NAME string to write, or None for no-net pads."""
    if signal in NO_NET:
        return None
    if signal in SIGMAP:
        return SIGMAP[signal]
    # New signal: single-pad net named exactly after the signal (D14..D19,
    # D22..D53, AREF, ...). Net NAME == signal.
    return signal


def u():
    return str(uuid.uuid4())


def make_pad(num, dx, dy, signal, pin1):
    shape = "rect" if pin1 else "circle"
    net = net_for(signal)
    lines = [
        f'\t\t(pad "{num}" thru_hole {shape}',
        f'\t\t\t(at {fmt(dx)} {fmt(dy)})',
        f'\t\t\t(size {PAD_SIZE})',
        f'\t\t\t(drill {PAD_DRILL})',
        f'\t\t\t(layers {PAD_LAYERS})',
        '\t\t\t(remove_unused_layers no)',
    ]
    if net is not None:
        lines.append(f'\t\t\t(net "{net}")')
    lines.append(f'\t\t\t(uuid "{u()}")')
    lines.append('\t\t)')
    return "\n".join(lines)


def fmt(v):
    # KiCad style: trim trailing zeros but keep integers as integers
    r = round(v, 6)
    if r == int(r):
        return str(int(r))
    s = ("%.6f" % r).rstrip("0").rstrip(".")
    return s


def silk_line(lx1, ly1, lx2, ly2):
    return (
        '\t\t(fp_line\n'
        f'\t\t\t(start {fmt(lx1)} {fmt(ly1)})\n'
        f'\t\t\t(end {fmt(lx2)} {fmt(ly2)})\n'
        '\t\t\t(stroke\n'
        '\t\t\t\t(width 0.12)\n'
        '\t\t\t\t(type solid)\n'
        '\t\t\t)\n'
        '\t\t\t(layer "F.SilkS")\n'
        f'\t\t\t(uuid "{u()}")\n'
        '\t\t)'
    )


def silk_pin1(cx, cy):
    # fp_circle filled, radius 0.3mm (center=cx,cy; end on circle => cx+0.3)
    return (
        '\t\t(fp_circle\n'
        f'\t\t\t(center {fmt(cx)} {fmt(cy)})\n'
        f'\t\t\t(end {fmt(cx + 0.3)} {fmt(cy)})\n'
        '\t\t\t(stroke\n'
        '\t\t\t\t(width 0.12)\n'
        '\t\t\t\t(type solid)\n'
        '\t\t\t)\n'
        '\t\t\t(fill yes)\n'
        '\t\t\t(layer "F.SilkS")\n'
        f'\t\t\t(uuid "{u()}")\n'
        '\t\t)'
    )


def make_silk(h, X, Y):
    """Rectangle outline (bbox+0.7mm) + pin-1 filled circle, en coords LOCALES."""
    pads = h["pads_abs"]
    xs = [p["x"] for p in pads]
    ys = [p["y"] for p in pads]
    minx, maxx = min(xs), max(xs)
    miny, maxy = min(ys), max(ys)
    # pads_abs donne le CENTRE des pads. Pad 1.7mm => demi-largeur 0.85mm.
    # Marge = demi-pad (0.85) + demi-trait silk (0.06) + jeu ~0.15 = ~1.06mm,
    # pour que le contour passe juste a l'EXTERIEUR des pads (pas dessus).
    m = 1.06
    # corners en coords locales (footprint rot 0 => local = world - origine)
    x0, y0 = minx - m - X, miny - m - Y
    x1, y1 = maxx + m - X, maxy + m - Y
    lines = [
        silk_line(x0, y0, x1, y0),
        silk_line(x1, y0, x1, y1),
        silk_line(x1, y1, x0, y1),
        silk_line(x0, y1, x0, y0),
    ]
    # pin-1 marker: 1.0mm en diagonale a l'EXTERIEUR du pad 1.
    p1 = next(p for p in pads if p["num"] == "1")
    p1lx, p1ly = p1["x"] - X, p1["y"] - Y
    # centre de l'array (local) pour determiner la direction "exterieure"
    cxa = (minx + maxx) / 2 - X
    cya = (miny + maxy) / 2 - Y
    # 1.0mm par axe => distance diagonale 1.414mm; bord du cercle (r=0.3) a
    # 1.114mm du centre du pad, donc hors du pad (rayon 0.85mm).
    d = 1.0
    sx = -1 if p1lx <= cxa else 1
    sy = -1 if p1ly <= cya else 1
    mx, my = p1lx + sx * d, p1ly + sy * d
    lines.append(silk_pin1(mx, my))
    return "\n".join(lines)


def make_footprint(h):
    tkey = h["ref"]           # cle template (J1..J7) pour la lib/identification
    ref = TKEY_TO_PCBREF[tkey]  # reference reelle sur le PCB (JMP1/JMA*/JMD*/JMX1)
    X, Y = h["x"], h["y"]
    libid = LIBID[tkey]
    out = []
    out.append(f'\t(footprint "{libid}"')
    out.append('\t\t(layer "F.Cu")')
    out.append(f'\t\t(uuid "{u()}")')
    out.append(f'\t\t(at {fmt(X)} {fmt(Y)} 0)')
    out.append(f'\t\t(descr "Through hole pin header, {ref}, 2.54mm pitch")')
    out.append('\t\t(attr through_hole)')
    # Reference / Value text properties
    out.append(f'\t\t(property "Reference" "{ref}"')
    out.append('\t\t\t(at 0 -2.38 0)')
    out.append('\t\t\t(layer "F.SilkS")')
    out.append(f'\t\t\t(uuid "{u()}")')
    out.append('\t\t\t(effects (font (size 1 1) (thickness 0.15)))')
    out.append('\t\t)')
    out.append(f'\t\t(property "Value" "MEGA_HDR"')
    out.append('\t\t\t(at 0 -4 0)')
    out.append('\t\t\t(layer "F.Fab")')
    out.append(f'\t\t\t(uuid "{u()}")')
    out.append('\t\t\t(effects (font (size 1 1) (thickness 0.15)))')
    out.append('\t\t)')
    out.append('\t\t(property "Datasheet" "" (at 0 0 0) (layer "F.Fab") (hide yes)')
    out.append(f'\t\t\t(uuid "{u()}") (effects (font (size 1.27 1.27))))')
    out.append('\t\t(property "Description" "" (at 0 0 0) (layer "F.Fab") (hide yes)')
    out.append(f'\t\t\t(uuid "{u()}") (effects (font (size 1.27 1.27))))')
    # Pads with explicit offsets
    pads = h["pads_abs"]
    for p in pads:
        dx = p["x"] - X
        dy = p["y"] - Y
        pin1 = (p["num"] == "1")
        out.append(make_pad(p["num"], dx, dy, p["signal"], pin1))
    # Silkscreen: contour rectangulaire + marqueur pin 1.
    out.append(make_silk(h, X, Y))
    out.append('\t)')
    return "\n".join(out)


def main():
    placement = json.loads((ROOT / "shield_redesign" / "placement.json").read_text())
    s = PCB.read_text()

    # 1. Remove the 7 old footprints by Reference.
    fps = find_blocks(s, '\t(footprint ')
    to_remove = []
    for a, b in fps:
        if prop(s[a:b], "Reference") in OLD_REFS:
            to_remove.append((a, b))
    if len(to_remove) != 7:
        sys.exit(f"ERREUR: trouve {len(to_remove)} anciennes embases, attendu 7")
    # Remove from end to start to keep indices valid; also swallow the
    # trailing newline after each footprint block.
    for a, b in sorted(to_remove, reverse=True):
        end = b
        if s[end:end+1] == "\n":
            end += 1
        s = s[:a] + s[end:]

    # 2. Build the 7 new footprints text.
    blocks = [make_footprint(h) for h in placement["headers"]]
    new_text = "\n" + "\n".join(blocks) + "\n"

    # 3. Insert before the final closing paren of the kicad_pcb block.
    # The file ends with a line ")\n"; insert right before it.
    idx = s.rfind("\n)")
    if idx < 0:
        sys.exit("ERREUR: impossible de trouver la parenthese finale")
    s = s[:idx] + new_text + s[idx:]

    PCB.write_text(s)
    print(f"OK: 7 anciennes embases supprimees, {len(blocks)} nouvelles ajoutees (J1..J7).")


if __name__ == "__main__":
    main()
