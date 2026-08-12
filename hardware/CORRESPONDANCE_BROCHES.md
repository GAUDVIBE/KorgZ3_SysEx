# Correspondance broches shield ↔ Arduino Mega 2560 réel

> ## ✅ CORRIGÉ EN v1.5 — 11/08/2026
>
> Le défaut décrit ci-dessous **n'existe plus dans les fichiers de conception**. Les 50
> pastilles des 6 blocs ont été réaffectées à la broche Mega qu'elles touchent réellement,
> **sans qu'aucune ne soit déplacée** (`shield_redesign/fix_mega_pinorder.py`, idempotent),
> puis la carte a été intégralement re-routée : **0 connexion manquante, 0 erreur DRC**.
> Gerbers dans `hardware/fab_v15/`.
>
> La table qui suit décrit donc la **carte v1.2 que tu as physiquement entre les mains**.
> Elle reste valable pour la réparation au fil de cette carte-là, et comme trace du défaut.
>
> **Conséquence firmware :** le remap de compensation ajouté pour la v1.2 (permutations
> D/A, contournement des pages sur D10/D11) doit être **retiré** avant d'utiliser une carte
> v1.5 — sinon les deux inversions s'annulent à l'envers. Ce retrait n'est pas fait.
>
> **Pourquoi le défaut avait survécu aux tests :** `check_pinorder.py` attendait lui aussi
> l'ordre inversé — écrit à partir de la même croyance que le générateur, il passait au vert
> sur une carte fausse. Le brochage réel vit désormais dans `shield_redesign/mega_pinout.py`,
> importé à la fois par le correctif et par le test, qui couvre les 6 blocs et non plus la
> seule rangée analogique.

**⚠️ DÉFAUT DE CONCEPTION v1.2 (carte fabriquée) : l'ordre des broches est INVERSÉ à
l'intérieur de chacun des 6 blocs d'embases.** Les blocs sont bien placés (géométrie
validée contre `mega2560_reel.json`, la carte s'enfiche), mais l'affectation des nets
aux pastilles est faite de la dernière broche vers la première.

Origine `x_rel = 0` = 1re broche du connecteur POWER, côté USB (convention de
`mega2560_reel.json`). Le vrai Mega, en x croissant depuis cette origine :

- rangée alim/analogique : `(réservé) IOREF RESET 3V3 5V GND GND VIN | A0…A7 | A8…A15`
- rangée numérique : `SCL SDA AREF GND D13…D8 | D7…D0 | D14…D21`

Conséquences majeures :

- le **plan de masse du shield est câblé sur RESET et IOREF** → relier ce plan à la
  vraie masse maintient le Mega en reset (symptôme « le Mega s'éteint », 100 % réversible) ;
- le **+5 V du shield est câblé sur la sortie 3V3** → le shield n'a jamais été alimenté
  (écran OLED noir, potentiomètres inertes, mux muet) ;
- les 4 pastilles marquées `NC` de JMP1 sont posées sur les **vraies broches 5V et GND** :
  ce sont elles qui servent à la réparation.

| embase | x_rel | net du SHIELD | broche MEGA réellement touchée | verdict |
|---|---:|---|---|---|
| JMD1 | -9.144 | `D8` | **D21_SCL** | permutation (rattrapable en firmware) |
| JMD1 | -6.604 | `D9` | **D20_SDA** | permutation (rattrapable en firmware) |
| JMD1 | -4.064 | `D10` | **AREF** | 🚨 **inutilisable** |
| JMD1 | -1.524 | `D11` | **GND** | 🚨 **inutilisable** |
| JMD1 | 1.016 | `D12` | **D13** | permutation (rattrapable en firmware) |
| JMD1 | 3.556 | `D13` | **D12** | permutation (rattrapable en firmware) |
| JMD1 | 6.096 | `GND` | **D11** | permutation (rattrapable en firmware) |
| JMD1 | 8.636 | `AREF` | **D10** | permutation (rattrapable en firmware) |
| JMD1 | 11.176 | `D20_SDA` | **D9** | permutation (rattrapable en firmware) |
| JMD1 | 13.716 | `D21_SCL` | **D8** | permutation (rattrapable en firmware) |
| JMD2 | 17.780 | `D0_RX` | **D7** | permutation (rattrapable en firmware) |
| JMD2 | 20.320 | `D1_TX` | **D6** | permutation (rattrapable en firmware) |
| JMD2 | 22.860 | `D2` | **D5** | permutation (rattrapable en firmware) |
| JMD2 | 25.400 | `D3` | **D4** | permutation (rattrapable en firmware) |
| JMD2 | 27.940 | `D4` | **D3** | permutation (rattrapable en firmware) |
| JMD2 | 30.480 | `D5` | **D2** | permutation (rattrapable en firmware) |
| JMD2 | 33.020 | `D6` | **D1_TX** | permutation (rattrapable en firmware) |
| JMD2 | 35.560 | `D7` | **D0_RX** | permutation (rattrapable en firmware) |
| JMD3 | 40.640 | `D21_SCL` | **D14** | permutation (rattrapable en firmware) |
| JMD3 | 43.180 | `D20_SDA` | **D15** | permutation (rattrapable en firmware) |
| JMD3 | 45.720 | `D19` | **D16** | permutation (rattrapable en firmware) |
| JMD3 | 48.260 | `D18` | **D17** | permutation (rattrapable en firmware) |
| JMD3 | 50.800 | `D17` | **D18** | permutation (rattrapable en firmware) |
| JMD3 | 53.340 | `D16` | **D19** | permutation (rattrapable en firmware) |
| JMD3 | 55.880 | `D15` | **D20_SDA** | permutation (rattrapable en firmware) |
| JMD3 | 58.420 | `D14` | **D21_SCL** | permutation (rattrapable en firmware) |
| JMP1 | 0.000 | `VIN_RAW` | **(reserve)** | 🚨 **inutilisable** |
| JMP1 | 2.540 | `GND` | **IOREF** | 🚨 **inutilisable** |
| JMP1 | 5.080 | `GND` | **RESET** | 🚨 **inutilisable** |
| JMP1 | 7.620 | `+5V` | **3V3** | 🚨 **inutilisable** |
| JMP1 | 10.160 | `+5V` | **5V** | ⚠️ rectifié le 11/08 : cette pastille **porte déjà le net `+5V`** (vérifié dans le PCB, identique dans HEAD). Le `+5V` du shield touche donc à la fois le vrai 5 V *et* la sortie 3V3 — ce qui explique mieux le symptôme qu'une absence d'alimentation |
| JMP1 | 12.700 | `NC` | **GND** | pastille morte posée sur une vraie broche — **à exploiter pour la réparation** |
| JMP1 | 15.240 | `NC` | **GND** | pastille morte posée sur une vraie broche — **à exploiter pour la réparation** |
| JMP1 | 17.780 | `NC` | **VIN** | pastille morte |
| JMA1 | 22.860 | `A7` | **A0** | permutation (rattrapable en firmware) |
| JMA1 | 25.400 | `A6` | **A1** | permutation (rattrapable en firmware) |
| JMA1 | 27.940 | `A5` | **A2** | permutation (rattrapable en firmware) |
| JMA1 | 30.480 | `A4` | **A3** | permutation (rattrapable en firmware) |
| JMA1 | 33.020 | `A3` | **A4** | permutation (rattrapable en firmware) |
| JMA1 | 35.560 | `A2` | **A5** | permutation (rattrapable en firmware) |
| JMA1 | 38.100 | `A1` | **A6** | permutation (rattrapable en firmware) |
| JMA1 | 40.640 | `A0` | **A7** | permutation (rattrapable en firmware) |
| JMA2 | 45.720 | `A15` | **A8** | permutation (rattrapable en firmware) |
| JMA2 | 48.260 | `A14` | **A9** | permutation (rattrapable en firmware) |
| JMA2 | 50.800 | `A13` | **A10** | permutation (rattrapable en firmware) |
| JMA2 | 53.340 | `A12` | **A11** | permutation (rattrapable en firmware) |
| JMA2 | 55.880 | `A11` | **A12** | permutation (rattrapable en firmware) |
| JMA2 | 58.420 | `A10` | **A13** | permutation (rattrapable en firmware) |
| JMA2 | 60.960 | `A9` | **A14** | permutation (rattrapable en firmware) |
| JMA2 | 63.500 | `A8` | **A15** | permutation (rattrapable en firmware) |
