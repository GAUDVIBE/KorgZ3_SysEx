#!/usr/bin/env python3
"""v1.5 — remet les nets des embases Mega dans le BON ordre.

DÉFAUT CORRIGÉ (v1.2 fabriquée) : les 6 blocs d'embases sont posés aux bonnes
positions (la carte s'enfiche, géométrie validée contre mega2560_reel.json) mais
l'affectation net→pastille est faite de la DERNIÈRE broche vers la PREMIÈRE à
l'intérieur de chaque bloc. Conséquences : masse du shield sur RESET/IOREF,
+5 V sur la sortie 3V3, D10/D11 sur AREF/GND.

CE SCRIPT NE DÉPLACE AUCUNE PASTILLE. Il ne réécrit que le net de chacune,
d'après la broche Mega réellement située à cette position. Les positions sont
comparées avant/après et le script échoue si l'une bouge d'un micron.

Preuve que le sens ci-dessous est le bon, sans documentation externe : sur un
Arduino, VIN est à l'extrémité du bloc POWER opposée à l'USB, face au bloc
analogique — et c'est A0 qui lui fait face. La v1.2 place « réservé » face à A7,
ce qui n'existe sur aucune carte.

Usage : python3 shield_redesign/fix_mega_pinorder.py   (depuis hardware/kicad_project/)
"""
import pathlib, re, sys

sys.path.insert(0, str(pathlib.Path(__file__).parent))
from sexpr import find_blocks, prop, block_at
from mega_pinout import BLOCS as MEGA, PITCH, net_attendu

ROOT = pathlib.Path(__file__).resolve().parent.parent
PCB = ROOT / "SysEx_Patcher.kicad_pcb"


def pads_of(blk, X, Y):
    """(index_debut, index_fin, x_monde, y_monde, net) pour chaque pastille."""
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
        out.append(dict(num=m.group(1), a=m.start(), b=i + 1,
                        x=X + float(at.group(1)), y=Y + float(at.group(2)),
                        net=nm.group(1) if nm else None))
    return out


def set_net(seg, net):
    """Réécrit — ou retire — la ligne (net "...") d'une pastille."""
    has = re.search(r'\n\t\t\t\(net "[^"]*"\)', seg)
    if net is None:
        return seg[:has.start()] + seg[has.end():] if has else seg
    if has:
        return seg[:has.start()] + f'\n\t\t\t(net "{net}")' + seg[has.end():]
    # Insérer juste avant l'uuid, pour rester dans l'ordre habituel de KiCad.
    u = re.search(r'\n\t\t\t\(uuid ', seg)
    return seg[:u.start()] + f'\n\t\t\t(net "{net}")' + seg[u.start():]


def main():
    s = PCB.read_text()

    # Origine x_rel = pastille de plus petit x du bloc POWER (1re broche, x_rel 0
    # par définition). Ancrage PUREMENT GÉOMÉTRIQUE : s'ancrer sur un net ferait
    # dépendre le correctif de l'affectation qu'il est justement en train de
    # corriger, et le rendrait faux si on le relançait sur une carte déjà saine.
    origine = None
    for a, b in find_blocks(s, '\t(footprint '):
        blk = s[a:b]
        if prop(blk, "Reference") != "JMP1":
            continue
        X, Y, _ = block_at(blk)
        origine = min(p["x"] for p in pads_of(blk, X, Y))
    if origine is None:
        sys.exit("ERREUR: embase JMP1 introuvable (origine x_rel)")

    avant, apres, edits, changes = {}, {}, [], 0
    for a, b in find_blocks(s, '\t(footprint '):
        blk = s[a:b]
        ref = prop(blk, "Reference")
        if ref not in MEGA:
            continue
        X, Y, _ = block_at(blk)
        x0, signaux = MEGA[ref]
        pads = pads_of(blk, X, Y)
        if len(pads) != len(signaux):
            sys.exit(f"ERREUR: {ref} a {len(pads)} pastilles, attendu {len(signaux)}")

        nouveau = []
        for p in pads:
            xr = p["x"] - origine
            try:
                sig, net = net_attendu(ref, xr)
            except ValueError as e:
                sys.exit(f"ERREUR: {ref} pastille {p['num']} — {e}")
            nouveau.append((p, sig, net))
            if net != p["net"]:
                changes += 1

        avant[ref] = [(p["x"], p["y"], p["net"]) for p in pads]
        apres[ref] = [(p["x"], p["y"], n) for p, _s, n in nouveau]
        # Éditions différées : positions absolues dans le fichier.
        for p, _sig, net in nouveau:
            edits.append((a + p["a"], a + p["b"], net))

        largeur = max(len(str(n)) for _p, _s, n in nouveau)
        print(f"\n{ref} (x_rel {x0:+.3f} …)")
        for p, sig, net in sorted(zip([q[0] for q in nouveau],
                                      [q[1] for q in nouveau],
                                      [q[2] for q in nouveau]),
                                  key=lambda t: t[0]["x"]):
            flag = "  " if net == p["net"] else "→ "
            print(f"   x_rel {p['x']-origine:8.3f}  broche Mega {sig:<10s} "
                  f"{flag}net {str(p['net']):<10s} devient {str(net):<{largeur}s}")

    if len(avant) != 6:
        sys.exit(f"ERREUR: {len(avant)} embases trouvées, attendu 6")

    # Appliquer de la fin vers le début pour garder les index valides.
    for a, b, net in sorted(edits, reverse=True):
        s = s[:a] + set_net(s[a:b], net) + s[b:]

    PCB.write_text(s)

    # ── Garde-fou : aucune pastille ne doit avoir bougé ───────────────────────
    s2 = PCB.read_text()
    for a, b in find_blocks(s2, '\t(footprint '):
        blk = s2[a:b]
        ref = prop(blk, "Reference")
        if ref not in MEGA:
            continue
        X, Y, _ = block_at(blk)
        relu = sorted([(p["x"], p["y"], p["net"]) for p in pads_of(blk, X, Y)])
        if relu != sorted(apres[ref]):
            sys.exit(f"ERREUR: relecture de {ref} incohérente avec l'intention")
        pos_avant = sorted((x, y) for x, y, _ in avant[ref])
        pos_apres = sorted((x, y) for x, y, _ in relu)
        for (xa, ya), (xb, yb) in zip(pos_avant, pos_apres):
            if abs(xa - xb) > 1e-9 or abs(ya - yb) > 1e-9:
                sys.exit(f"ERREUR: une pastille de {ref} a bougé — annuler")

    print(f"\nOK — {changes} pastilles réaffectées, 0 pastille déplacée.")
    print("⚠️  Le routage existant est maintenant caduc : re-router avant fabrication.")


if __name__ == "__main__":
    main()
