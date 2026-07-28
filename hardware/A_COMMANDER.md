# Liste d'achat — shield Korg Z3 SysEx v1.2

Quantités extraites du schéma et du PCB le 2026-07-28, pour **une carte**.
Les 16 potentiomètres ne sont pas comptés : ils sont déjà en stock.

Carte **100 % traversante** — aucun composant monté en surface.

---

## Résistances

Toutes axiales, 1/4 W, pas **10,16 mm** (boîtier DIN0207, montage horizontal).

| ☐ | Qté | Valeur | À quoi ça sert |
|---|---|---|---|
| ☐ | **25** | **1 kΩ** | filtres anti-bruit des 16 pots et du jack, limitation des 7 LEDs, série du canal switch |
| ☐ | 3 | 220 Ω | MIDI IN et MIDI OUT |
| ☐ | 1 | 330 Ω | section MIDI |
| ☐ | 1 | 10 kΩ | tirage de la sortie du 6N138 |
| ☐ | 1 | **470 kΩ** | **détection de présence du câble guitare** |

> Les 1 kΩ sont de loin les plus nombreuses : en prendre une trentaine évite de
> se retrouver bloqué pour une résistance.
>
> La 470 kΩ n'est pas un détail décoratif : sans elle, une entrée en l'air fait
> émettre au Z3 du pitch bend fantôme dès que la guitare est débranchée.

## Condensateurs

| ☐ | Qté | Valeur | Boîtier |
|---|---|---|---|
| ☐ | **19** | 100 nF céramique | disque Ø 5 mm, pas 5,00 mm |
| ☐ | 1 | 100 µF électrolytique | radial Ø 6,3 mm, pas 2,50 mm — **polarisé** |

## Semi-conducteurs

| ☐ | Qté | Référence | Boîtier |
|---|---|---|---|
| ☐ | 1 | **6N138** optocoupleur | DIP-8 |
| ☐ | 1 | **CD74HC4067** multiplexeur 16:1 | DIP-24 **large**, 15,24 mm |
| ☐ | 1 | 1N4148 | DO-35, pas 7,62 mm |
| ☐ | 1 | 1N4004 | DO-41, pas 10,16 mm |
| ☐ | **7** | LED 3 mm | au choix, avec leurs 1 kΩ ci-dessus |

> Prendre aussi **2 supports DIP** (8 et 24 broches larges) : ça évite de souder
> les circuits intégrés directement et permet de les remplacer.

## Connecteurs de la carte

| ☐ | Qté | Élément | Note |
|---|---|---|---|
| ☐ | 2 | Embase MIDI **DIN 5 broches, 180°** | montage circuit imprimé |
| ☐ | 1 | Jack d'alimentation **barrel horizontal** | |
| ☐ | 1 | **Mini-XLR 5 points Switchcraft TRAPC** | horizontal, vers la guitare |
| ☐ | 2 | Barrette mâle 1×4, pas 2,54 mm | écran OLED et alimentation batterie |

## Barrettes d'interface Arduino Mega

**Femelles**, pas 2,54 mm. Les prendre **à longues broches** (type empilable) si
tu veux garder la possibilité de superposer autre chose.

| ☐ | Qté | Format |
|---|---|---|
| ☐ | 5 | 1×8 |
| ☐ | 1 | 1×10 |

> Pas de barrette 2×18 : elle a été retirée en v1.2, sa position réelle sur le
> Mega traversant une colonne de potentiomètres.

## Commandes en façade

| ☐ | Qté | Élément |
|---|---|---|
| ☐ | **7** | Bouton poussoir **6 mm** |

> 5 pour les pages, 2 pour le dump et les presets.

## Visserie

| ☐ | Qté | Élément |
|---|---|---|
| ☐ | 4 | Vis M3 + entretoises (trous Ø 3,2 mm) |

## Modules

| ☐ | Qté | Élément |
|---|---|---|
| ☐ | 1 | **Arduino Mega 2560** |
| ☐ | 1 | Écran **OLED 128 × 32 I²C**, adresse 0x3C |

---

## Côté guitare

| ☐ | Qté | Élément |
|---|---|---|
| ☐ | 1 | Molette de pitch **10 kΩ linéaire, à ressort de rappel** |
| ☐ | 1 | Switch **3 positions ON-ON-ON** |
| ☐ | 4 | Résistance **2,2 kΩ** — l'échelle qui encode le switch |
| ☐ | 1 | Embase mini-XLR 5 points à encastrer dans le pickguard |
| ☐ | 1 | **Câble mini-XLR 5 points à fiche coudée** |

### Deux vérifications avant de payer

**Le genre des connecteurs.** L'embase de la carte, celle du pickguard et les
deux fiches du câble doivent être complémentaires. C'est l'erreur d'achat
classique sur ce type de liaison : vérifier sur la fiche technique Switchcraft
si l'empreinte TRAPC correspond à un mâle ou à une femelle, et commander le
câble en conséquence.

**La fiche coudée.** L'embase étant encastrée dans le pickguard d'une
Stratocaster, une fiche droite butera contre le corps. C'est le point
d'approvisionnement le plus incertain du projet.

---

## Le circuit imprimé

Fichier à envoyer : **`SysEx_Patcher_v1.2_JLCPCB.zip`**

| Paramètre | Valeur |
|---|---|
| Dimensions | 172 × 172 mm |
| Couches | 4 |
| Épaisseur | 1,6 mm |
| Perçage minimal | 0,30 mm |
| Piste et isolation minimales | 0,20 mm |
| Finition | HASL sans plomb, ou ENIG |

Ces valeurs restent dans les capacités standard : aucun supplément pour classe
de précision.
