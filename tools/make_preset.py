#!/usr/bin/env python3
"""Genere des tableaux de preset Korg Z3 pour le sketch Arduino.

FORMAT DU DUMP (verifie contre le sketch et contre deux dumps reels du Z3) :
  5 octets d'en-tete  F0 42 30 1D 40
  89 octets de donnees
  1 octet  F7
  = 95 octets

  offset = 5 + n_parametre        pour les parametres 0..11
  offset = 6 + n_parametre        pour les parametres 12..87

Le decalage vient du LFO Rate (parametre 11), seul parametre sur DEUX octets :
poids fort a l'offset 16, poids faible a l'offset 17.

Les parametres 0..7 sont le nom, 8 caracteres ASCII. Aucun potentiometre ne les
pilote : le contrôleur ne peut PAS renommer un patch a l'execution, et
`extractName()` derive le nom affiche de ces octets. Un preset ecrit ici est
donc le seul moyen d'avoir un nom juste a la fois sur l'ecran et sur le Z3.

Les parametres 16..87 sont dix-huit blocs de 4, dans l'ordre M-1, C-1, M-2, C-2.
M = modulateur, C = porteuse.

CONVENTION DU TOTAL LEVEL — deduite de deux dumps reels, PAS du manuel :
une valeur HAUTE = plus de niveau. SynLead a ses modulateurs a 98-99 et sonne
piquant ; Flute__1 les a a 27-36 et sonne doux. La convention Yamaha habituelle
(0 = plus fort) donnerait l'inverse et ne colle pas.

Usage: python3 tools/make_preset.py
"""

GLOBAUX = {8: "Algorithme", 9: "Feed Back", 10: "LFO Wave", 11: "LFO Rate",
           12: "PMD", 13: "PMS", 14: "AMD", 15: "AMS"}
BLOCS = [(16, "OSC Wave"), (20, "Mul1"), (24, "Mul2"), (28, "Detune1"),
         (32, "Detune2"), (36, "Total Level"), (40, "Attack"), (44, "Decay1"),
         (48, "Sustain"), (52, "Decay2"), (56, "Release"), (60, "Key Scale"),
         (64, "AMS Enable"), (68, "EG Shift"), (72, "Reverb Level"),
         (76, "Reverb Rate"), (80, "Velocity Int"), (84, "Keyboard Track")]
OPS = ["M-1", "C-1", "M-2", "C-2"]

# Dump reel capture sur le Z3.
FLUTE_SOURCE = """
F0 42 30 1D 40 46 6C 75 74 65 5F 5F 31 03 07 02 01 4B 20 04 26 00 00 00 00 00
03 04 02 02 00 00 00 00 04 04 04 04 01 00 00 00 1B 2F 24 46 1F 1F 1F 10 14 11
1F 1F 05 02 00 00 13 00 00 00 09 06 05 0B 01 00 00 01 00 00 00 01 00 00 00 00
00 00 00 00 00 00 00 00 00 00 00 0A 00 04 04 01 F7
"""


def octets_de(texte):
    d = [int(x, 16) for x in texte.split()]
    assert len(d) == 95, "un dump Z3 fait 95 octets, recu %d" % len(d)
    assert d[0] == 0xF0 and d[-1] == 0xF7, "en-tete ou fin de SysEx invalide"
    return d


def offset(p):
    return 5 + p if p <= 11 else 6 + p


def lire(d, p):
    return d[16] * 128 + d[17] if p == 11 else d[offset(p)]


def ecrire(d, p, v):
    if p == 11:
        d[16], d[17] = v // 128, v % 128
    else:
        d[offset(p)] = v


def poser_nom(d, texte):
    for i, c in enumerate((texte + "        ")[:8]):
        d[5 + i] = ord(c)


def afficher(d, titre):
    print("\n" + titre)
    print("  nom embarque : «%s»" % "".join(chr(b) for b in d[5:13]))
    print("  " + " | ".join("%s %s" % (n, lire(d, p)) for p, n in GLOBAUX.items()))
    print("  %-16s%5s%5s%5s%5s" % ("", *OPS))
    for base, nom in BLOCS:
        print("  %-16s%5d%5d%5d%5d" % (nom, *(lire(d, base + i) for i in range(4))))


def en_c(d, nom_c, commentaire):
    out = ["// %s" % commentaire, "const byte %s[] PROGMEM = {" % nom_c]
    for i in range(0, len(d), 12):
        out.append("  " + ", ".join("0x%02X" % b for b in d[i:i + 12]) + ",")
    out[-1] = out[-1].rstrip(",")
    out += ["};", "const int %s_SIZE = sizeof(%s);" % (nom_c, nom_c)]
    return "\n".join(out)


# ---------------------------------------------------------------------------
# FLUTE SENSIBLE A L'ATTAQUE
#
# Le Flute__1 d'origine n'a de Velocity Int que sur C-2, une PORTEUSE : la
# velocite n'y fait varier que le volume, jamais le timbre. C'est pour cela
# qu'il ne repond pas a l'attaque de la guitare.
#
# SynLead, lui, en a sur M-1 et M-2, les MODULATEURS : attaquer fort augmente
# la modulation, donc ouvre le timbre. C'est ce comportement qu'on transpose.
#
# On ne touche a RIEN d'autre. L'equilibre des niveaux, les enveloppes et les
# rapports de frequence font le caractere doux : les modifier reviendrait a
# refaire le son au lieu de lui ajouter une reponse au jeu.
# ---------------------------------------------------------------------------
INTENSITES = {"Doux": 5, "Moyen": 8, "Fort": 12}

if __name__ == "__main__":
    base = octets_de(FLUTE_SOURCE)
    afficher(base, "=== SOURCE : dump reel Flute__1 ===")

    for etiquette, vi in INTENSITES.items():
        d = base[:]
        poser_nom(d, "FlutAtk" + etiquette[0])   # FlutAtkD / FlutAtkM / FlutAtkF
        ecrire(d, 80, vi)      # Velocity Int M-1 : 0 -> vi
        ecrire(d, 82, vi)      # Velocity Int M-2 : 0 -> vi
        print("\n" + "=" * 70)
        print(en_c(d, "FLUTE_ATK_%s" % etiquette.upper(),
                   "Flute douce, timbre ouvert par l'attaque "
                   "(Velocity Int %d sur les modulateurs M-1 et M-2)" % vi))
