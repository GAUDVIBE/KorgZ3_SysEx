# Cotes pour la conception du boîtier — shield v1.2

Toutes les cotes sont en millimètres, dans le repère du PCB.
**Origine (0,0) = coin supérieur gauche du fichier KiCad.**
La carte occupe **x 20 à 192** et **y 20 à 192**, soit **172 × 172 mm**.

Pour raisonner depuis un coin de carte, retire 20 à chaque valeur.

## Connecteurs du bord haut

| Réf | Connecteur | Centre en x | Corps (x) | Atteint y | Retrait du bord |
|---|---|---|---|---|---|
| `J1` | MIDI IN, DIN 5 | **50.0** | 39.8 à 60.2 | **26.9** | **6.9** |
| `J2` | MIDI OUT, DIN 5 | **87.0** | 76.8 à 97.2 | **26.9** | **6.9** |
| `J10` | Mini-XLR 5 pts | **110.8** | 104.0 à 117.7 | **24.0** | **4.0** |
| `J3` | Alimentation, barrel | **137.9** | 133.2 à 142.6 | **27.4** | **7.4** |

> Le bord haut de la carte est à **y = 20**. Aucun connecteur ne l'atteint :
> la paroi du boîtier doit donc venir au contact du connecteur, ou les
> découpes être prolongées vers l'intérieur.

## Trous de fixation M3

| Réf | Position |
|---|---|
| `MH1` | x 28.0, y 28.0 |
| `MH2` | x 184.0, y 28.0 |
| `MH3` | x 28.0, y 184.0 |
| `MH4` | x 184.0, y 184.0 |

Perçage Ø 3,2 mm, aux quatre coins, en retrait de 8 mm des bords.

## Hauteur

La carte est **100 % traversante**. Le Mega 2560 s'enfiche **dessous** :
prévoir sa hauteur sous le PCB, connecteur USB et jack d'alimentation compris.
Au-dessus dépassent les 16 potentiomètres, les 7 boutons, les 7 LEDs et
l'écran OLED sur son connecteur.

## Commandes en façade

| Élément | Position |
|---|---|
| `RV1` (coin de la grille) | x 50.0, y 80.0 |
| `RV4` (coin de la grille) | x 146.0, y 80.0 |
| `RV13` (coin de la grille) | x 50.0, y 152.0 |
| `RV16` (coin de la grille) | x 146.0, y 152.0 |

Les 16 potentiomètres forment une grille 4×4 régulière, colonnes espacées
de 32 mm et rangées de 24 mm.

Les 7 boutons et leurs LEDs : **5 paires à droite** (x ≈ 168 et 178,
y = 60, 84, 108, 132, 156) et **2 paires à gauche** (x ≈ 26 et 35, y = 100 et 160).
