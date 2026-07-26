#!/usr/bin/env python3
"""Task 10 (corrected): deplace uniquement J2 (MIDI OUT) de (72,38,0) a
(81,38,0) pour porter l'entraxe MIDI IN/OUT a 37mm, sans toucher J1
(garde a x=44 a cause de la collision decouverte avec MH1), ni J10, ni J3.
Reutilise la logique reposition() de apply_controls.py (edition s-expr
directe, jamais via l'API pcbnew)."""
import sys, re, pathlib
sys.path.insert(0, str(pathlib.Path(__file__).parent))
from sexpr import find_blocks, prop

PCB_PATH = pathlib.Path("SysEx_Patcher.kicad_pcb")
src = PCB_PATH.read_text()


def fp_block(text, ref):
    for a, b in find_blocks(text, "(footprint "):
        if prop(text[a:b], "Reference") == ref:
            return a, b, text[a:b]
    raise SystemExit("footprint introuvable: " + ref)


def set_at(blk, x, y, rot):
    def repl(m):
        return "(at %s %s %s)" % (x, y, rot) if rot else "(at %s %s)" % (x, y)
    return re.sub(r"\(at [-\d.]+ [-\d.]+(?: [-\d.]+)?\)", repl, blk, count=1)


def reposition(text, ref, x, y, rot):
    a, b, blk = fp_block(text, ref)
    nb = set_at(blk, x, y, rot)
    return text[:a] + nb + text[b:]


before = fp_block(src, "J2")[2]
m = re.search(r"\(at [-\d.]+ [-\d.]+(?: [-\d.]+)?\)", before)
print("J2 avant:", m.group(0))

src = reposition(src, "J2", 81.0, 38.0, 0)

after_blk = fp_block(src, "J2")[2]
m2 = re.search(r"\(at [-\d.]+ [-\d.]+(?: [-\d.]+)?\)", after_blk)
print("J2 apres:", m2.group(0))

PCB_PATH.write_text(src)
print("move_j2 OK")
