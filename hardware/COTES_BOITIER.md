# Cotes pour la conception du boîtier — shield v1.2

Toutes les cotes sont en millimètres, dans le repère du PCB.
**Origine (0,0) = coin supérieur gauche du fichier KiCad.**
La carte occupe **x 20 à 192** et **y 20 à 192**, soit **172 × 172 mm**.

Pour raisonner depuis un coin de carte, retire 20 à chaque valeur.

## Connecteurs du bord haut

| Réf | Connecteur | Centre en x | Corps (x) | Atteint y | Retrait du bord |
|---|---|---|---|---|---|
| `J1` | MIDI IN, DIN 5 | **50.0** | 39.8 à 60.2 | **20.4** | **0.4** |
| `J2` | MIDI OUT, DIN 5 | **87.0** | 76.8 à 97.2 | **20.4** | **0.4** |
| `J10` | Mini-XLR 5 pts | **110.8** | 104.0 à 117.7 | **20.0** | **0.0** |
| `J3` | Alimentation, barrel | **137.9** | 133.2 à 142.6 | **27.4** | **7.4** |

> Le bord haut de la carte est à **y = 20**. Les deux DIN MIDI et le mini-XLR
> l'atteignent désormais : la paroi du boîtier peut venir au ras de la carte.
>
> **Le jack d'alimentation J3 reste 7,4 mm en retrait.** C'est une limite de son
> empreinte, pas un choix : ses pastilles de 3,5 mm s'étendent en avant de son
> corps, et l'avancer davantage les fait sortir de la carte et entrer en conflit
> avec le plan de masse. Prévoir une découpe prolongée vers l'intérieur, ou un
> jack déporté par un court câble jusqu'à la paroi.

### Percer la paroi pour le mini-XLR `J10`

Relevé sur le plan Switchcraft `TRAPC_X - TRASM_X SERIES` rév. C, recoupé sur
l'empreinte : le connecteur occupe **y 20,125 à 40,191**, soit ses 20,07 mm hors
tout. Sa face avant est donc **0,125 mm en retrait** du bord de carte.

| Élément | Cote |
|---|---|
| Ø du fût du connecteur | 11,1 mm |
| **Ø de perçage conseillé** | **12,0 mm** |
| Jeu face avant ↔ paroi | 0,125 mm |
| Dépassement de la fiche branchée | **43,7 mm** vers l'extérieur |
| Ø de la fiche branchée | 10,5 mm |

Le fût **ne traverse pas la paroi** : c'est pourquoi on prend la version sans
filetage, `TRAPC5M1X`, dont l'écrou serait inutilisable. La contrepartie est que
rien ne retient le connecteur hors ses soudures.

> **Percer au plus juste** — 12,0 mm, pas davantage. C'est le trou lui-même qui
> reprendra l'effort latéral au branchement.
>
> La fiche femelle du câble doit traverser l'épaisseur de la paroi avant
> d'atteindre le connecteur. **Garder la paroi mince à cet endroit** (1,5 mm
> environ), ou la chanfreiner : trop épaisse, le verrou de la fiche
> n'encliquettera pas.

Prévoir enfin **43,7 mm de dégagement** devant la paroi, dans l'axe du
connecteur : c'est la longueur de la fiche `TA5FSH` une fois branchée.

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
