#!/usr/bin/env python3
"""Task 5: place les 7 paires bouton+LED (5 droite / 2 gauche) et ajoute
les 3 nouveaux controles (SW6/7/8, R28/29/30, D7/8/9)."""
import sys, re, uuid, pathlib
sys.path.insert(0, str(pathlib.Path(__file__).parent))
from sexpr import find_blocks, prop

PCB_PATH = pathlib.Path("SysEx_Patcher.kicad_pcb")
src = PCB_PATH.read_text()


def newuuid():
    return str(uuid.uuid4())


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


def set_ref(blk, ref):
    return blk.replace('"Reference" "%s"' % prop(blk, "Reference"),
                        '"Reference" "%s"' % ref, 1)


def set_value(blk, val):
    m = re.search(r'"Value" "([^"]*)"', blk)
    return blk.replace('"Value" "%s"' % m.group(1), '"Value" "%s"' % val, 1)


def freshen_uuids(blk):
    return re.sub(r'\(uuid "[0-9a-f-]+"\)',
                  lambda m: '(uuid "%s")' % newuuid(), blk)


def set_pad_net(blk, padnum, oldnet, newnet):
    out, pos = [], 0
    for pa, pb in find_blocks(blk, "(pad "):
        out.append(blk[pos:pa])
        pad = blk[pa:pb]
        num = re.search(r'\(pad "?([^" ]+)"?', pad).group(1)
        if num == padnum:
            pad = pad.replace('(net "%s")' % oldnet, '(net "%s")' % newnet, 1)
        out.append(pad)
        pos = pb
    out.append(blk[pos:])
    return "".join(out)


def clone(text, src_ref, new_ref, x, y, rot, value, pad_nets):
    _, _, blk = fp_block(text, src_ref)
    blk = set_at(blk, x, y, rot)
    blk = set_ref(blk, new_ref)
    if value is not None:
        blk = set_value(blk, value)
    for padnum, oldnet, newnet in pad_nets:
        blk = set_pad_net(blk, padnum, oldnet, newnet)
    blk = freshen_uuids(blk)
    return blk


# ---------------------------------------------------------------------------
# Coordonnees colonnes
RB_X, RL_X, RR_X = 151, 160, 166   # droite : bouton / LED / R
LB_X, LL_X, LR_X = 26, 22, 36      # gauche : bouton / LED / R
# y des 2 paires gauche : evite J1/MIDI (y27-41), D1/R1 (y58-66),
# la rangee de cavaliers Mega JMD1 (y133-135) et le mux U2 (y>179).
LEFT_Y = [100, 160]

# Chaine existante -> (LEDref, Rref)
existing = {"SW2": ("D3", "R6"), "SW3": ("D4", "R7"),
            "SW4": ("D5", "R8"), "SW5": ("D6", "R9")}

# 1) Re-positionner les 4 paires existantes a DROITE (y = 84,108,132,156)
right_existing = [("SW2", 84), ("SW3", 108), ("SW4", 132), ("SW5", 156)]
for sw, y in right_existing:
    led, res = existing[sw]
    src = reposition(src, sw, RB_X, y, 0)
    src = reposition(src, led, RL_X, y, 90)
    src = reposition(src, res, RR_X, y, 90)

# 2) Cloner les nouveaux footprints
#    SW6 (D22), SW7 (D24), SW8 (D26) -- bouton: pad1=signal, pad2=GND
#    R28 (D23)->LED5_A, R29 (D25)->LED6_A, R30 (D27)->LED7_A
#    D7 ->LED5_A, D8 ->LED6_A, D9(comp) ->LED7_A ; cathode pad1=GND
new_fps = []

# SW6 a DROITE y=60 ; SW7/SW8 a GAUCHE y=84/132
# bouton template SW2 : pad1 net "D7" (signal), pad2 net "GND"
# NB: a y=60 le condensateur existant C19 (100uF, x~162) occupe la colonne
#     LED/R standard ; on decale donc D7 et R28 plus a droite (x=168 / x=178)
#     pour eviter tout chevauchement avec C19 (non deplacable).
SW6_LED_X, SW6_R_X = 168, 178
new_fps.append(clone(src, "SW2", "SW6", RB_X, 60, 0, None,
                     [("1", "D7", "D22")]))
new_fps.append(clone(src, "SW2", "SW7", LB_X, LEFT_Y[0], 0, None,
                     [("1", "D7", "D24")]))
new_fps.append(clone(src, "SW2", "SW8", LB_X, LEFT_Y[1], 0, None,
                     [("1", "D7", "D26")]))

# resistance template R6 : pad1 "D12", pad2 "LED1_A"
new_fps.append(clone(src, "R6", "R28", SW6_R_X, 60, 90, "1k",
                     [("1", "D12", "D23"), ("2", "LED1_A", "LED5_A")]))
new_fps.append(clone(src, "R6", "R29", LR_X, LEFT_Y[0], 90, "1k",
                     [("1", "D12", "D25"), ("2", "LED1_A", "LED6_A")]))
new_fps.append(clone(src, "R6", "R30", LR_X, LEFT_Y[1], 90, "1k",
                     [("1", "D12", "D27"), ("2", "LED1_A", "LED7_A")]))

# LED template D3 : pad1 "GND" (cathode), pad2 "LED1_A" (anode)
new_fps.append(clone(src, "D3", "D7", SW6_LED_X, 60, 90, None,
                     [("2", "LED1_A", "LED5_A")]))
new_fps.append(clone(src, "D3", "D8", LL_X, LEFT_Y[0], 90, None,
                     [("2", "LED1_A", "LED6_A")]))
new_fps.append(clone(src, "D3", "D9", LL_X, LEFT_Y[1], 90, None,
                     [("2", "LED1_A", "LED7_A")]))

# 3) Inserer les nouveaux footprints juste avant le dernier ')' du fichier
ins = "\n\t" + "\n\t".join(new_fps) + "\n"
last = src.rfind(")")
src = src[:last] + ins + src[last:]

PCB_PATH.write_text(src)
print("apply_controls OK -- %d nouveaux footprints inseres" % len(new_fps))
