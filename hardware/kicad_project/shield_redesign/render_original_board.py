#!/usr/bin/env python3
"""Regenere hardware/board_top.* et board_bottom.* depuis les Gerbers D'ORIGINE.

Ces rendus servent de reference de tracage dans RECONSTRUCTION_SCHEMA.md : le
document renvoie a `board_top.svg` pour suivre les pistes de la carte source.
Or ils avaient ete ecrases a plusieurs reprises par des rendus de NOTRE propre
reconstruction (commits 9f9ce09, 9c0f954, 2746b1d...) : le fichier montrait un
etat intermediaire de notre shield, cartouche KiCad compris. Quiconque suivait
les instructions du document regardait la mauvaise carte.

Ce script les reconstruit depuis la source authentique.

Deux pieges rencontres :
  - le dossier iCloud contient AUSSI une copie de nos propres Gerbers, ce qui
    fait echouer LayerStack.open sur des noms de couches ambigus. On copie donc
    les seuls fichiers `SysEx_Programmer_BIG_display.*` dans un dossier isole.
  - le `.GML` n'est PAS reconnu comme contour de carte ; gerbonara deduit
    l'emprise du cuivre, ce qui convient pour une reference de tracage.

Usage: python3 shield_redesign/render_original_board.py
"""
import shutil, sys, tempfile, pathlib, warnings

warnings.filterwarnings("ignore")

SOURCE = pathlib.Path(
    "/Users/gaudry/Library/Mobile Documents/com~apple~CloudDocs/"
    "MUZIKALOID/MIDI Sysex Controler/Universal SysEx Prog"
)
PREFIXE = "SysEx_Programmer_BIG_display"
SORTIE = pathlib.Path(__file__).resolve().parents[2]      # hardware/

try:
    from gerbonara import LayerStack
except ImportError:
    sys.exit("gerbonara absent : pip3 install gerbonara")

if not SOURCE.is_dir():
    sys.exit("Gerbers d'origine introuvables : %s" % SOURCE)

with tempfile.TemporaryDirectory() as tmp:
    tmp = pathlib.Path(tmp)
    n = 0
    for f in SOURCE.glob(PREFIXE + ".*"):
        shutil.copy(f, tmp / f.name)
        n += 1
    print("%d fichiers d'origine isoles" % n)

    stack = LayerStack.open(tmp, lazy=False)
    print("couches lues :", sorted(str(k) for k in stack.graphic_layers))

    # `to_pretty_svg` exige un contour de carte, que le .GML ne fournit pas, et
    # `LayerStack.to_svg` est casse dans gerbonara 0.13 (NameError page_bg).
    # On compose donc soi-meme : chaque couche est rendue separement, sur une
    # emprise commune forcee pour qu'elles se superposent au pixel pres.
    # On calcule l'emprise depuis les couches elles-memes : `board_bounds` est
    # une methode ici, et le contour de carte est de toute facon absent.
    boites = [l.bounding_box() for l in stack.graphic_layers.values()]
    boites = [b for b in boites if b]
    (x0, y0), (x1, y1) = boites[0]
    for (a, b), (c, d) in boites[1:]:
        x0, y0 = min(x0, a), min(y0, b)
        x1, y1 = max(x1, c), max(y1, d)
    bounds = ((x0 - 2, y0 - 2), (x1 + 2, y1 + 2))
    print("emprise commune : %.1f x %.1f mm" % (bounds[1][0] - bounds[0][0],
                                                bounds[1][1] - bounds[0][1]))

    COULEURS = {"copper": "#c87137", "silk": "#f0f0f0", "mask": "#1b6b33"}

    for cote, nom in (("top", "board_top"), ("bottom", "board_bottom")):
        morceaux, entete = [], None
        for ordre in ("mask", "copper", "silk"):
            couche = stack.graphic_layers.get((cote, ordre))
            if couche is None:
                continue
            brut = str(couche.to_svg(force_bounds=bounds, fg=COULEURS[ordre],
                                     bg="none"))
            if entete is None:
                entete = brut[: brut.index(">", brut.index("<svg")) + 1]
            debut = brut.index(">", brut.index("<svg")) + 1
            morceaux.append('<g id="%s" opacity="%s">%s</g>'
                            % (ordre, "0.35" if ordre == "mask" else "1",
                               brut[debut: brut.rindex("</svg>")]))
        cible = SORTIE / (nom + ".svg")
        cible.write_text(entete + "\n".join(morceaux) + "</svg>")
        print("  %-18s %8d octets" % (cible.name, cible.stat().st_size))

print("\nSVG regeneres depuis la carte d'ORIGINE.")
print("Conversion en PNG (rsvg-convert, paquet librsvg) :")
print("  rsvg-convert -w 2000 hardware/board_top.svg -o hardware/board_top.png")
