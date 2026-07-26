#!/usr/bin/env python3
"""v1.1 : corrige le corps des embases MIDI et re-repartit le bord haut.

Le corps de l'embase DIN etait dessine comme un CERCLE de rayon 11,4 mm
(diametre 22,8). Mesure au pied a coulisse sur une embase reelle : c'est un
RECTANGLE de 20,5 x 15,0 mm. L'erreur n'etait pas cosmetique — elle faisait
croire que J1 touchait l'optocoupleur U1 a -0,02 mm et donc qu'il ne pouvait
pas etre deplace, alors que le corps reel s'arrete a y=41,9 quand U1 commence
a y=45, soit plus de 3 mm de degagement.

Une fois la geometrie corrigee, on ecarte le bord haut :
  J1  44 -> 50   jeu a la vis M3 de MH1 : 2,75 -> 8,75 mm
  J2  81 -> 87   entraxe MIDI maintenu a 37 mm
  J10 107 -> 114 jeu au MIDI OUT : 6,72 mm
  J3  inchange   jeu au mini-XLR : 15,62 mm (le deplacer etait inutile)

Edition s-expr directe, jamais via l'API pcbnew (elle plante sur ce projet).
"""
import sys, re, pathlib

sys.path.insert(0, str(pathlib.Path(__file__).parent))
from sexpr import find_blocks, prop

PCB_PATH = pathlib.Path("SysEx_Patcher.kicad_pcb")
MOD_PATH = pathlib.Path("SysEx_Patcher.pretty/MIDI_DIN5_180deg.kicad_mod")

# Corps reel mesure : 20,5 mm de large (x) sur 15,0 mm de profond (y),
# centre sur le meme point que l'ancien cercle, soit le local (0, -3,6).
CX, CY = 0.0, -3.6
HALF_W, HALF_D = 20.5 / 2, 15.0 / 2
X0, X1 = CX - HALF_W, CX + HALF_W
Y0, Y1 = CY - HALF_D, CY + HALF_D

# Positions cibles du bord haut.
MOVES = {"J1": (50.0, 38.0, 0), "J2": (87.0, 38.0, 0), "J10": (114.0, 40.0, 180)}


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
    return text[:a] + set_at(blk, x, y, rot) + text[b:]


def rect_lines(layer, width):
    """Quatre segments fermant le rectangle, sur la couche demandee."""
    corners = [(X0, Y0), (X1, Y0), (X1, Y1), (X0, Y1), (X0, Y0)]
    out = []
    for (sx, sy), (ex, ey) in zip(corners, corners[1:]):
        out.append(
            '(fp_line (start %s %s)(end %s %s)'
            '(stroke (width %s)(type solid))(layer "%s"))' % (sx, sy, ex, ey, width, layer)
        )
    return "\n  ".join(out)


def swap_circle_for_rect(text, layer, width, label):
    """Remplace le fp_circle de corps par un rectangle, sur `layer`.

    On ne se fie pas a l'ecriture exacte des nombres (le fichier melange
    `0` et `0.0`) : on repere tout fp_circle de la couche visee, on calcule
    son rayon, et on ne remplace que celui du corps.
    """
    n = 0
    # find_blocks equilibre les parentheses : indispensable ici, car le board
    # ecrit le fp_circle en multi-ligne avec un (uuid ...) apres la couche,
    # alors que la bibliotheque l'ecrit compact sur une seule ligne.
    for a, b in reversed(find_blocks(text, "(fp_circle")):
        blk = text[a:b]
        m = re.search(r'\(center ([-\d.]+) ([-\d.]+)\)\s*\(end ([-\d.]+) ([-\d.]+)\)', blk)
        lay = re.search(r'\(layer "([^"]+)"\)', blk)
        if not m or not lay or lay.group(1) != layer:
            continue
        cx, cy, ex, ey = (float(m.group(i)) for i in (1, 2, 3, 4))
        rayon = ((ex - cx) ** 2 + (ey - cy) ** 2) ** 0.5
        if abs(cy - CY) > 0.01 or rayon < 10.0:
            continue                   # pas le cercle de corps, on n'y touche pas
        text = text[:a] + rect_lines(layer, width) + text[b:]
        n += 1
    print("  %-22s %d cercle(s) -> rectangle" % (label, n))
    return text, n


# --- 1. la bibliotheque du projet ---------------------------------------
mod = MOD_PATH.read_text()
print("Bibliotheque %s :" % MOD_PATH.name)
mod, n1 = swap_circle_for_rect(mod, "F.SilkS", "0.12", "serigraphie")
mod, n2 = swap_circle_for_rect(mod, "F.CrtYd", "0.05", "courtyard")
mod = mod.replace(
    "5-pin DIN 180deg MIDI socket",
    "5-pin DIN 180deg MIDI socket, body 20.5x15.0mm (measured)",
)
MOD_PATH.write_text(mod)

# --- 2. les instances embarquees dans le board --------------------------
src = PCB_PATH.read_text()
print("\nInstances dans le board :")
for ref in ("J1", "J2"):
    a, b, blk = fp_block(src, ref)
    blk, n = swap_circle_for_rect(blk, "F.SilkS", "0.12", ref)
    if n == 0:
        raise SystemExit("corps circulaire introuvable dans " + ref)
    src = src[:a] + blk + src[b:]

# --- 3. re-repartition du bord haut -------------------------------------
print("\nDeplacements :")
for ref, (x, y, rot) in MOVES.items():
    before = re.search(r"\(at [-\d.]+ [-\d.]+(?: [-\d.]+)?\)", fp_block(src, ref)[2]).group(0)
    src = reposition(src, ref, x, y, rot)
    after = re.search(r"\(at [-\d.]+ [-\d.]+(?: [-\d.]+)?\)", fp_block(src, ref)[2]).group(0)
    print("  %-4s %-28s -> %s" % (ref, before, after))

PCB_PATH.write_text(src)
print("\nfix_din_body OK")
