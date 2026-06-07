import pathlib
from sexpr import find_blocks, prop, block_at
PCB = pathlib.Path("SysEx_Patcher.kicad_pcb").read_text()
pos = {}
for a, b in find_blocks(PCB, "(footprint "):
    blk = PCB[a:b]
    ref = prop(blk, "Reference")
    if ref and ref.startswith("SW"):
        at = block_at(blk)
        pos[ref] = (at[0], at[1])
# ne garder que les boutons utilisateur (SW2..SW8) ; ignorer un eventuel SW1
sws = [r for r in pos if r != "SW1"]
assert len(sws) >= 7, f"boutons trouves: {sws}"
xs = [pos[r][0] for r in sws]
right = [x for x in xs if x > 120]
left = [x for x in xs if x < 28]
assert len(right) >= 5 and len(left) >= 2, \
    f"repartition D/G: droite={len(right)} gauche={len(left)}"
print("check_controls_pcb OK -- %d boutons (droite %d / gauche %d)"
      % (len(sws), len(right), len(left)))
