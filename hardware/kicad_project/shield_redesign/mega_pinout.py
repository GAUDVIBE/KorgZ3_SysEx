#!/usr/bin/env python3
"""Brochage réel de l'Arduino Mega 2560 Rev3 — SOURCE DE VÉRITÉ UNIQUE.

Importé par `fix_mega_pinorder.py` (qui applique) et `check_pinorder.py` (qui
vérifie). Les deux ne peuvent donc plus diverger : c'est cette divergence qui a
laissé passer le défaut v1.2 — le test attendait l'ordre inversé, exactement
celui que produisait le générateur, et il passait au vert sur une carte fausse.

Origine `x_rel = 0` : 1re broche du bloc POWER, côté USB. Les `x_min` viennent
de `mega2560_reel.json`. Chaque liste est donnée en **x croissant**.

Pourquoi ce sens est le bon, sans recours à une documentation externe : sur un
Arduino, VIN occupe l'extrémité du bloc POWER opposée à l'USB, face au bloc
analogique, et c'est A0 qui lui fait face. Le gabarit KiCad `Arduino_Mega`
place « réservé » face à A7 — cette carte n'existe pas.
"""

PITCH = 2.54

# ref PCB -> (x_rel de la 1re broche, signaux en x croissant)
BLOCS = {
    "JMP1": (0.0, ["(reserve)", "IOREF", "RESET", "3V3", "5V", "GND", "GND", "VIN"]),
    "JMA1": (22.86, [f"A{i}" for i in range(0, 8)]),
    "JMA2": (45.72, [f"A{i}" for i in range(8, 16)]),
    "JMD1": (-9.144, ["D21", "D20", "AREF", "GND", "D13", "D12", "D11", "D10", "D9", "D8"]),
    "JMD2": (17.78, [f"D{i}" for i in range(7, -1, -1)]),
    "JMD3": (40.64, [f"D{i}" for i in range(14, 22)]),
}

# Broche Mega -> nom de net du shield. None = signal non utilisé par le shield ;
# la pastille reste sans net mais touche quand même la broche physique.
NET = {
    "(reserve)": None, "IOREF": None, "RESET": None, "3V3": None,
    "5V": "+5V", "GND": "GND", "VIN": "VIN_RAW", "AREF": "AREF",
    "D0": "D0_RX", "D1": "D1_TX", "D20": "D20_SDA", "D21": "D21_SCL",
    **{f"D{i}": f"D{i}" for i in range(2, 20)},
    **{f"A{i}": f"A{i}" for i in range(16)},
}


def net_attendu(ref, x_rel, tol=0.02):
    """Net attendu sur la pastille du bloc `ref` située à `x_rel`.

    Lève ValueError si la position ne tombe pas sur la grille du bloc.
    """
    x0, signaux = BLOCS[ref]
    i = round((x_rel - x0) / PITCH)
    if not (0 <= i < len(signaux)) or abs(x0 + i * PITCH - x_rel) > tol:
        raise ValueError(f"{ref}: x_rel={x_rel:.3f} hors grille (x0={x0}, "
                         f"{len(signaux)} broches)")
    return signaux[i], NET[signaux[i]]
