#!/usr/bin/env python3
"""Task 9: remplace l'empreinte de J10 (jack 6.35mm -> mini-XLR-5 Switchcraft
TRAPC horizontal) et place le trio filtre/pull-down du canal switch bend
(R32 serie, R31 pull-down, C20 filtre), sur le modele du canal 1 existant
(R27/C18)."""
import sys, re, uuid, pathlib
sys.path.insert(0, str(pathlib.Path(__file__).parent))
from sexpr import find_blocks, prop

PCB_PATH = pathlib.Path("SysEx_Patcher.kicad_pcb")
LIB_MOD = pathlib.Path(
    "/Applications/KiCad/KiCad.app/Contents/SharedSupport/footprints/"
    "Connector_Audio.pretty/MiniXLR-5_Switchcraft_TRAPC_Horizontal.kicad_mod"
)
FP_NAME = "Connector_Audio:MiniXLR-5_Switchcraft_TRAPC_Horizontal"

J10_AT = (107.0, 40.0, 180.0)   # x, y, rot -- voir justification dans le rapport de tache
J10_PAD_NETS = {"1": "GND", "2": "+5V", "3": "JACK_W", "4": "SW_BEND"}
# pad "5" : pas de net (omis volontairement)

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
# 1) J10 : construire le nouveau footprint mini-XLR-5 a partir de la
#    librairie KiCad, en l'adaptant a l'imbrication du fichier .kicad_pcb.

lib_src = LIB_MOD.read_text()

# nom complet "Bibliotheque:Empreinte"
lib_src = lib_src.replace(
    '(footprint "MiniXLR-5_Switchcraft_TRAPC_Horizontal"',
    '(footprint "%s"' % FP_NAME, 1)

# la reference dans le fichier de bibliotheque est le placeholder REF** ;
# on la remplace par la reference reelle du connecteur remplace (J10),
# et la Value par celle du schema (property "Value" "MINI-XLR-5" sur J10)
lib_src = lib_src.replace('"Reference" "REF**"', '"Reference" "J10"', 1)
lib_src = lib_src.replace(
    '"Value" "MiniXLR-5_Switchcraft_TRAPC_Horizontal"',
    '"Value" "MINI-XLR-5"', 1)

# retirer les lignes d'entete propres a un fichier de bibliotheque autonome
lib_src = re.sub(r'\n\t\(version [^\n]*\)', '', lib_src, count=1)
lib_src = re.sub(r'\n\t\(generator "[^"]*"\)', '', lib_src, count=1)
lib_src = re.sub(r'\n\t\(generator_version "[^"]*"\)', '', lib_src, count=1)

# inserer (uuid ...) et (at x y rot) juste apres (layer "F.Cu")
at_str = "(at %s %s %s)" % J10_AT
lib_src = lib_src.replace(
    '\t(layer "F.Cu")\n',
    '\t(layer "F.Cu")\n\t(uuid "%s")\n\t%s\n' % (newuuid(), at_str), 1)

# retirer les graphismes de courtyard (F.CrtYd / B.CrtYd) -- convention du
# reste de la carte : les courtyards sont volontairement absentes.
out, pos = [], 0
for a, b in find_blocks(lib_src, "(fp_line"):
    seg = lib_src[a:b]
    if '"F.CrtYd"' in seg or '"B.CrtYd"' in seg:
        # retirer aussi le retour-ligne + tabulation qui precedent le bloc
        start = a
        while start > 0 and lib_src[start - 1] in "\t":
            start -= 1
        if start > 0 and lib_src[start - 1] == "\n":
            start -= 1
        out.append(lib_src[pos:start])
        pos = b
out.append(lib_src[pos:])
lib_src = "".join(out)

# ajouter un niveau d'indentation (le fichier de bibliotheque autonome utilise
# 1 tabulation par niveau ; imbrique dans le .kicad_pcb il en faut 2)
lines = lib_src.split("\n")
lib_src = "\n".join([lines[0]] + ["\t" + ln if ln else ln for ln in lines[1:]])

# nets sur les pastilles 1 a 4 (pastille 5 : aucun net, on ne touche pas)
for padnum, net in J10_PAD_NETS.items():
    pat = re.compile(
        r'(\(pad "%s" thru_hole \w+\s*(?:\n\t*\([^\n]*\))*?\n\t*)(\(uuid)'
        % re.escape(padnum))
    def add_net(m, net=net):
        return m.group(1) + '(net "%s")\n\t\t\t' % net + m.group(2)
    new_src, n = pat.subn(add_net, lib_src, count=1)
    if n != 1:
        raise SystemExit("insertion net echouee pour pad %s" % padnum)
    lib_src = new_src

j10_new_block = freshen_uuids(lib_src)

# remplacer l'ancien bloc J10 (footprint jack 6.35mm) par le nouveau
a, b, _old = fp_block(src, "J10")
src = src[:a] + j10_new_block + src[b:]

# ---------------------------------------------------------------------------
# 2) R32 (1k, serie), R31 (470k, pull-down), C20 (100nF, filtre) --
#    calques sur R27/C18 (filtre canal 1), replaces dans la bande basse
#    pres du mux U2, en zone libre verifiee par calcul (cf. rapport).

new_fps = []

new_fps.append(clone(src, "R27", "R32", 86.0, 182.0, 0.0, "1k",
                     [("1", "JACK_W", "SW_BEND"), ("2", "MUX_CH1", "MUX_CH2")]))
new_fps.append(clone(src, "R27", "R31", 108.0, 182.0, 0.0, "470k",
                     [("1", "JACK_W", "MUX_CH2"), ("2", "MUX_CH1", "GND")]))
new_fps.append(clone(src, "C18", "C20", 99.0, 182.0, 0.0, "100nF",
                     [("1", "MUX_CH1", "MUX_CH2"), ("2", "GND", "GND")]))

ins = "\n\t" + "\n\t".join(new_fps) + "\n"
last = src.rfind(")")
src = src[:last] + ins + src[last:]

PCB_PATH.write_text(src)
print("apply_j10_filter OK -- J10 remplace, %d nouveaux footprints inseres" % len(new_fps))
