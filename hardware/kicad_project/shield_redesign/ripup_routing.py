#!/usr/bin/env python3
"""
ripup_routing.py — Supprime toutes les pistes (segment) et vias au niveau
racine du .kicad_pcb, en conservant les zones (coulée GND) et tout le
reste (empreintes, bordures...). C'est le "rip-up complet" de l'attempt B :
Freerouting repart avec une page blanche pour le cuivre et garde 100% de
liberté, contrairement au routage incrémental qui peut rester bloqué par
de la géométrie héritée (ex : anciens stubs près d'un connecteur déplacé).

Édition directe du s-expr (jamais via l'API pcbnew.RemoveNative — segfault
connu sur ce board).

Usage: python3 shield_redesign/ripup_routing.py [pcb_in] [pcb_out]
"""
import sys, pathlib

HERE = pathlib.Path(__file__).parent
PCB_DEFAULT = HERE.parent / "SysEx_Patcher.kicad_pcb"

def find_top_level_blocks(lines, token):
    """Retourne les indices (start, end) de lignes pour chaque bloc
    top-level '\t(token' à un seul niveau d'indentation."""
    blocks = []
    i = 0
    n = len(lines)
    while i < n:
        if lines[i].startswith('\t' + token) and not lines[i].startswith('\t\t' + token):
            # count depth from this line until it returns to 0
            depth = lines[i].count('(') - lines[i].count(')')
            j = i
            while depth > 0:
                j += 1
                depth += lines[j].count('(') - lines[j].count(')')
            blocks.append((i, j))
            i = j + 1
        else:
            i += 1
    return blocks

def main():
    pcb_in = str(sys.argv[1]) if len(sys.argv) > 1 else str(PCB_DEFAULT)
    pcb_out = str(sys.argv[2]) if len(sys.argv) > 2 else pcb_in

    with open(pcb_in) as f:
        text = f.read()
    lines = text.split('\n')

    seg_blocks = find_top_level_blocks(lines, '(segment')
    via_blocks = find_top_level_blocks(lines, '(via')

    to_remove = set()
    for s, e in seg_blocks + via_blocks:
        for k in range(s, e + 1):
            to_remove.add(k)

    print(f"Segments trouvés: {len(seg_blocks)}, vias trouvés: {len(via_blocks)}")
    print(f"Lignes à supprimer: {len(to_remove)}")

    kept = [line for idx, line in enumerate(lines) if idx not in to_remove]
    new_text = '\n'.join(kept)

    with open(pcb_out, 'w') as f:
        f.write(new_text)

    print(f"Sauvegardé: {pcb_out}")

if __name__ == "__main__":
    main()
