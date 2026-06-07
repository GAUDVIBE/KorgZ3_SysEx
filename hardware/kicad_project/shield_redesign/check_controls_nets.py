"""Self-check des nets des nouveaux controles (Task 5)."""
import pathlib, re
from sexpr import find_blocks, prop

PCB = pathlib.Path("SysEx_Patcher.kicad_pcb").read_text()


def pad_nets(ref):
    """retourne {padnum: set(nets)} pour le footprint ref."""
    for a, b in find_blocks(PCB, "(footprint "):
        blk = PCB[a:b]
        if prop(blk, "Reference") != ref:
            continue
        res = {}
        for pa, pb in find_blocks(blk, "(pad "):
            pad = blk[pa:pb]
            num = re.search(r'\(pad "?([^" ]+)"?', pad).group(1)
            m = re.search(r'\(net \d* ?"([^"]*)"\)', pad)
            if m:
                res.setdefault(num, set()).add(m.group(1))
        return res
    raise AssertionError("footprint absent: " + ref)


def nets_of(ref):
    s = set()
    for v in pad_nets(ref).values():
        s |= v
    return s


# 1) Boutons : pad signal + pad GND
btn = {"SW6": "D22", "SW7": "D24", "SW8": "D26"}
for sw, sig in btn.items():
    ns = nets_of(sw)
    assert sig in ns, f"{sw}: net signal {sig} absent ({ns})"
    assert "GND" in ns, f"{sw}: GND absent ({ns})"

# 2) Resistances + 3) LEDs : R pad1=signal, R pad2 == anode LED ; LED cathode=GND
chains = [("R28", "D23", "D7"), ("R29", "D25", "D8"), ("R30", "D27", "D9")]
nodes = []
for r, sig, d in chains:
    rp = pad_nets(r)
    rn = nets_of(r)
    assert sig in rn, f"{r}: net signal {sig} absent ({rn})"
    # pad2 de R = noeud intermediaire
    node = (rp["2"] - {sig}).pop() if rp.get("2") else None
    node = next(iter(rp["2"]))
    assert node not in ("GND", sig), f"{r}: noeud invalide {node}"
    nodes.append(node)
    # LED : anode (pad2) == node, cathode (pad1) == GND
    dp = pad_nets(d)
    assert node in dp["2"], f"{d}: anode != noeud {node} ({dp['2']})"
    assert "GND" in dp["1"], f"{d}: cathode != GND ({dp['1']})"

assert len(set(nodes)) == 3, f"noeuds non uniques: {nodes}"
print("check_controls_nets OK -- chaines:",
      ", ".join("%s->%s->%s(%s)" % (c[0], c[1], c[2], n)
                for c, n in zip(chains, nodes)))
