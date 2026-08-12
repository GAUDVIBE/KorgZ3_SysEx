"""Protocole SysEx du Korg Z3, extrait du firmware KorgZ3_SysEx_26-07-2026.ino.

Le Z3 ne connait pas le changement de parametre individuel : on entretient un
buffer contenant le dump COMPLET (F0 42 30 1D 40 + nom 8 car. + 81 octets + F7,
95 octets) et on le renvoie en entier a chaque modification — c'est ce que fait
le firmware Arduino, valide sur la machine.

Chaque parametre porte en plus un type de widget et un libelle court, calques
sur l'editeur midierror : dial pour ce qui est continu et musical (niveaux,
enveloppes), numbox pour les petits entiers a lire exactement (multiplicateurs,
formes d'onde, detunes), on/off pour les booleens.
"""

DUMP_REQUEST = [0xF0, 0x42, 0x30, 0x1D, 0x10, 0xF7]
DUMP_HEADER  = [0xF0, 0x42, 0x30, 0x1D, 0x40]
NAME_POS, NAME_LEN = 5, 8

SYNLEAD = [
  0xF0,0x42,0x30,0x1D,0x40,
  0x53,0x79,0x6E,0x4C,0x65,0x61,0x64,0x20,
  0x03,0x02,0x02,0x01,0x2F,0x17,0x03,0x04,0x02,0x02,
  0x01,0x00,0x01,0x0A,0x01,0x01,0x01,0x00,0x00,0x00,
  0x00,0x03,0x07,0x07,0x03,0x00,0x00,0x00,0x00,0x63,
  0x1C,0x62,0x00,0x19,0x18,0x06,0x15,0x00,0x00,0x00,
  0x1F,0x00,0x02,0x00,0x00,0x00,0x1F,0x00,0x06,0x0E,
  0x08,0x07,0x08,0x01,0x00,0x00,0x00,0x00,0x00,0x01,
  0x00,0x02,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x04,0x06,0x02,0x04,0x00,0x00,0x03,
  0x00,
  0xF7
]

# (longname, shortname, position, max, widget, libelle affiche)
GLOBAL = [
    ("Z3 Algorithm", "Algo",    13, 7,   "numbox", "Algo"),
    ("Z3 Feedback",  "FeedB",   14, 7,   "numbox", "Feedb"),
    ("Z3 LFO Wave",  "LFOWave", 15, 3,   "numbox", "Wave"),
    ("Z3 LFO Rate",  "Rate",    16, 255, "dial",   "Rate"),
    ("Z3 PMD",       "PMD",     18, 127, "dial",   "PMD"),
    ("Z3 PMS",       "PMS",     19, 7,   "numbox", "PMS"),
    ("Z3 AMD",       "AMD",     20, 127, "dial",   "AMD"),
    ("Z3 AMS",       "AMS",     21, 3,   "numbox", "AMS"),
]

OSC = ["M1", "C1", "M2", "C2"]

# (suffixe, base, max, widget, libelle) — position = base + index d'oscillateur.
# Ordre = ordre d'affichage dans le panneau (rangee 1 = les 6 dials).
OSC_PARAMS = [
    ("Level",   42, 127, "dial",   "Level"),
    ("Attack",  46, 31,  "dial",   "Attack"),
    ("Decay1",  50, 31,  "dial",   "Decay 1"),
    ("Sustain", 54, 15,  "dial",   "Sustain"),
    ("Decay2",  58, 31,  "dial",   "Decay 2"),
    ("Release", 62, 15,  "dial",   "Release"),

    ("Wave",    22, 7,   "numbox", "Wave"),
    ("Mult1",   26, 15,  "numbox", "Mult 1"),
    ("Mult2",   30, 15,  "numbox", "Mult 2"),
    ("Detune1", 34, 7,   "numbox", "Det 1"),
    ("Detune2", 38, 3,   "numbox", "Det 2"),
    ("KScale",  66, 3,   "numbox", "K Scale"),

    ("EG",      74, 3,   "numbox", "EG"),
    ("RateMod", 82, 7,   "numbox", "Rate M"),
    ("VeloInt", 86, 15,  "numbox", "Velo"),
    ("KTrack",  90, 15,  "numbox", "K Track"),
    ("AMS",     70, 1,   "onoff",  "AMS"),
    ("Reverb",  78, 1,   "onoff",  "Reverb"),
]

def all_params():
    """Liste canonique triee par position — MEME ordre que POSITIONS dans le pont JS."""
    ps = list(GLOBAL)
    for k, o in enumerate(OSC):
        for suf, base, mx, w, lab in OSC_PARAMS:
            ps.append((f"Z3 {o} {suf}", f"{o}{suf}"[:13], base + k, mx, w, lab))
    return sorted(ps, key=lambda p: p[2])

def value_at(dump, pos):
    return dump[16] * 128 + dump[17] if pos == 16 else dump[pos]

if __name__ == "__main__":
    ps = all_params()
    manquants = sorted(set(range(13, 94)) - {p[2] for p in ps} - {17})
    print(f"{len(ps)} parametres | positions manquantes : {manquants or 'aucune'}")
    from collections import Counter
    print("widgets :", dict(Counter(p[4] for p in ps)))
