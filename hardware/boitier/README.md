# Boîtier pupitre — shield Korg Z3 SysEx v1.2

Modèle paramétrique OpenSCAD. Cinq pièces, aucune vis autre que du M3.

```
openscad -o fond.stl -D 'piece="fond"' boitier.scad
```

| Pièce | Nombre | Encombrement | Impression |
|---|---|---|---|
| `fond` | 1 | 182,5 × 189,6 × 78,6 mm | posé sur son dessous, sans support |
| `facade` | 1 | 182,5 × 149,5 × 12,5 mm | face visible **contre le plateau** |
| `capot_arriere` | 1 | 176,1 × 25,0 × 29,5 mm | couché sur son tablier |
| `plaque_arriere` | 1 | 135,6 × 30,6 × 3,0 mm | à plat |
| `capuchon` | **7** | 8,4 × 8,4 × 4,7 mm | à plat |

Aucune pièce ne demande de support. Toutes reposent sur `z = 0`.

## Le principe

**La carte est inclinée à 15°, pas seulement la façade.** Avec un axe de
potentiomètre de 15 mm, une carte à plat sous un couvercle en pente verrait
l'écart passer de quelques millimètres à l'avant à plus de 46 mm à l'arrière :
les axes du fond ne sortiraient jamais. Carte, façade et parois avant et
arrière forment donc un seul bloc incliné ; seul le dessous est aplani.

**Deux niveaux.** La façade s'appuie sur la face d'appui des potentiomètres, à
10 mm de la carte, alors que le mini-XLR mesure 13,7 mm de haut et les embases
MIDI davantage. Les connecteurs sont donc logés sous un **dosseret** surélevé
à l'arrière.

**Les cotes incertaines sont sur une pièce sacrificielle.** Les hauteurs des
embases au-dessus de la carte sont les seules valeurs que le fichier KiCad ne
donne pas. Elles sont toutes portées par `plaque_arriere`, qui coûte vingt
minutes d'impression au lieu de dix heures.

## Ce sont les potentiomètres qui tiennent la façade

Leur canon fileté de 5 mm traverse la façade de 2,5 mm et reçoit son écrou. Seize
écrous répartis sur la surface donnent bien plus de rigidité que quatre vis
d'angle — c'est le montage de n'importe quel synthétiseur.

```
axe                      15,0 mm  au-dessus de la face d'appui
façade                  − 2,5 mm
                        ─────────
axe émergent              12,5 mm    ← de quoi tenir n'importe quel bouton

canon fileté              5,0 mm
façade                  − 2,5 mm
                        ─────────
filetage pour l'écrou      2,5 mm    ← l'écrou Alpha en fait exactement 2,0
```

> **C'est pour cela que la façade fait 2,5 mm et non 3.** À 3 mm il restait
> 2,0 mm de filetage pour un écrou de 2,0 mm : l'écrou arrivait pile en bout de
> filet, sans rien pour la rondelle. Une assertion arrête désormais le rendu si
> `facade_ep` dépasse `pot_filetage_h − pot_ecrou_h`.

Ces valeurs sont **calculées et affichées à chaque compilation** :

```
ECHO: "Axe emergent au-dessus de la facade : 12.5 mm"
ECHO: "Filetage restant pour l'ecrou       : 2.5 mm"
ECHO: "Lamage sous la facade pour boutons  : 0 mm"
```

**La règle : `facade_ep` ne doit jamais dépasser `pot_filetage_h − pot_ecrou_h`.**

### Les boutons

Le poussoir monte à 7,3 mm, la façade est à 10,0 : **2,7 mm de garde**, aucun
lamage nécessaire. La collerette du capuchon, plus large que le perçage, le rend
captif entre le poussoir et la façade — il ne peut ni tomber ni ressortir.

Le connecteur de batterie `J4`, à `(116, 132)`, monte à environ 8,4 mm s'il est
peuplé : il passe désormais, avec 1,6 mm de reste.

## Les potentiomètres : compatibilité établie

Vérifié **contre la carte d'origine**, où ces mêmes potentiomètres sont déjà
soudés. Mesure faite dans son fichier de perçage
`SysEx_Programmer_BIG_display.TXT` : 48 trous de Ø 1,100 mm (16 pots × 3
broches) et 32 trous de Ø 1,600 mm (16 × 2 pattes), les 16 pots reconstitués.

| | Décalage / broches | Entraxe des pattes | Perçage des pattes |
|---|---|---|---|
| Carte d'origine | **7,00 mm** | **9,00 mm** | Ø 1,60 rond |
| Notre empreinte | **7,00 mm** | **8,80 mm** | oblong **2,10 × 1,80** |

Le décalage est identique. L'entraxe diffère de 0,20 mm, soit 0,10 mm par
patte, mais nos trous sont **oblongs et plus larges que ceux d'origine dans les
deux directions** : le jeu l'absorbe.

L'empreinte `Potentiometer_Alps_RK09K_Single_Vertical` n'a **pas changé d'un
micron** entre la reconstruction initiale (`ae7454e`) et aujourd'hui — vérifié
sur les trois révisions majeures du circuit.

> Nos trous de broches font Ø 1,00 mm, exactement la préconisation du plan
> Alpha (*3-Ø1.0 +0.1/-0*). La carte d'origine était simplement plus généreuse
> à Ø 1,10.

### La valeur : 10 kΩ, pas 100

La fiche que tu m'as transmise est celle du **`B100K`**. Il nous faut le
**`B10K`** — c'est la valeur du montage, et 100 kΩ tripleraient l'impédance de
source vue par le convertisseur. Même référence, même mécanique, autre suffixe.

## L'écran : module Velleman VMA438

Dalle **Univision UG-2864HSWEG01**, fiche `SAS1-9046-B`.

| | |
|---|---|
| Résolution | **128 × 64** |
| Zone active | **21,744 × 10,864 mm** |
| Dalle | 26,70 × 19,26 × 1,45 mm |
| Fenêtre percée | 23,34 × 12,46 (zone active + 0,8 de débord) |

### ⚠️ Le brochage est inversé sur les deux premières broches

| Broche | `J5` sur la carte | VMA438 |
|---|---|---|
| 1 | **GND** | **VCC** |
| 2 | **+5 V** | **GND** |
| 3 | SCL | SCL |
| 4 | SDA | SDA |

**Enfiché en direct, le +5 V arriverait sur la masse du module.** Comme il doit
de toute façon être relié par fils — `J5` est sous le dosseret, l'écran en
façade — il suffit de croiser les deux premiers. C'est même une chance que la
liaison soit câblée plutôt qu'enfichée.

### ⚠️ Le firmware est déclaré en 128 × 32

`SCREEN_HEIGHT 32` dans le `.ino`, alors que la dalle fait 64. Le SSD1306
serait initialisé en multiplex 1/32 sur un panneau 1/64 : image écrasée, une
ligne sur deux. **Passer la constante à 64** — l'affichage y gagne le double
de surface.

### Le berceau : deux rails ouverts

Le décalage entre la dalle et le bord de la carte du module m'est inconnu. Plutôt
que de le deviner, le berceau est fait de **deux rails parallèles** qui ne
contraignent que la largeur : le module s'y glisse et **coulisse** jusqu'à ce que
la dalle tombe en face de la fenêtre, puis se fixe à la colle ou au ruban
double face. Débattement disponible : **± 3,8 mm**.

Seule cote critique, donc : la **largeur** de la carte du module, `oled_pcb_l`,
à confirmer au pied à coulisse (27,0 mm par défaut).

### Position

`(75,5 ; 128,5)`, centré au-dessus de la grille de potentiomètres.

L'emplacement provisoire précédent — près de `J5`, à `(142 ; 128)` — **entrait
en collision avec le bouton `SW6`**. La bande libre entre les corps de
potentiomètres (Y ≤ 111,1) et la marche du dosseret (Y = 146) ne fait que
34,9 mm ; le module y tient avec 3,7 mm de marge de chaque côté.

## Ce qu'il faut mesurer

En tête de `boitier.scad`, section « À MESURER ». Ce sont les seules valeurs
non déduites du PCB.

| Paramètre | Défaut | Comment le relever |
|---|---|---|
| `pot_corps_h` | 10,0 | ✅ plan Alpha `SLH-211-414` |
| `pot_axe_h` | 15,0 | ✅ idem — axe Ø 6,35, pas 9 |
| `pot_filetage_h` | 5,0 | ✅ idem — canon M7 × 0,75 |
| `pot_canon_d` | 7,0 | ✅ idem |
| `pot_ecrou_h` | 2,0 | ✅ idem — écrou 10 mm sur plats |
| `bouton_h` | 7,3 | donnée par la référence GCT `SWT0325-**0730**16TSK` |
| `midi_z` | 11 | hauteur de l'axe de l'embase MIDI |
| `midi_d` | 23 | Ø d'une fiche DIN 5 |
| `xlr_z` | 12 | hauteur de l'axe du `TB5M` |
| `xlr_percage` | 11,28 | ✅ relevé sur la fiche `TB_M SERIES` rév. S |
| `xlr_meplat` | 10,72 | ✅ idem |
| `alim_z` | 6 | hauteur de l'axe du jack |

## Le mini-XLR `TB5M`

C'est une embase **de panneau**, pas de circuit imprimé : elle ne se soude pas
dans l'empreinte `J10`, elle se visse sur `plaque_arriere` et se câble jusqu'aux
cinq pastilles de `J10` par de courts fils.

C'est un gain, pas une contrainte. L'embase de carte `TRAPC5M1X` n'était retenue
que par ses cinq soudures ; ici la paroi encaisse tout l'effort de branchement.
Et comme elle n'est plus liée à l'empreinte, elle est **libre en X comme en Z** :
son axe est placé à 12 mm au-dessus de la carte pour que son corps, qui plonge
vers l'intérieur, passe au-dessus du circuit sans le toucher.

### Le perçage n'est pas rond

Relevé sur la fiche `TB_M SERIES` rév. S, section *RECOMMENDED "D" MOUNTING HOLE* :

| Cote | Pouces | mm |
|---|---|---|
| Diamètre | 0,444 <sup>+0,004</sup> | **11,28** |
| Méplat, au bord opposé | 0,422 <sup>+0,004</sup> | **10,72** |
| Filetage | 7/16-32 UNS-2A | — |
| Écrou six pans | 0,56 sur plats × 0,09 | 14,2 × 2,3 |
| Rondelle plate | Ø 0,63 × 0,02 | Ø 16,0 × 0,5 |
| **Épaisseur de paroi maximale** | **0,250** | **6,35** |

Le **méplat de 0,56 mm** est ce qui empêche l'embase de tourner quand on
dévisse une fiche récalcitrante. Il est reproduit dans le modèle.

La plaque fait 3 mm : largement sous les 6,35 mm admissibles. Le dégagement
autour du trou a été vérifié — la rondelle de Ø 16 demande 8 mm libres, le
voisin le plus proche est le MIDI OUT à 23,8 mm.

> La fiche note que *« solder terminals may be soldered to a PC board »* et donne
> des implantations. **Elles ne correspondent pas à notre empreinte `J10`** :
> le `TB5M` reste à câbler par fils.

## Assemblage

1. Visser la carte sur les quatre entretoises du `fond`, M3 × 8.
2. Glisser `plaque_arriere` par le haut dans sa feuillure.
3. Câbler le `TB5M` aux pastilles de `J10`.
4. Poser `capot_arriere`, deux M3 × 12 dans les parois latérales.
5. Poser les 7 `capuchon` sur les boutons.
6. Poser la `facade`, puis **serrer les 16 écrous de potentiomètre** (M7 × 0,75,
   clé de 10). Ce sont eux qui la tiennent et la raidissent.
7. Quatre M3 × 20 aux angles, à travers les bossages, dans les entretoises du
   fond : les mêmes vis tiennent la façade **et** la carte.

Le flanc gauche est volontairement ouvert sur 60 mm : le Mega y déborde de
2,8 mm, et c'est par là que sortent son USB et son jack d'alimentation.

## Réglages courants

| Envie | Paramètre |
|---|---|
| Pupitre moins haut | `pente` — 10° ramène le dosseret de 78 à ~63 mm |
| Boîtier plus rigide | `paroi` de 3 à 4 mm |
| Plus de garde sous la carte | `z_avant`, 10 mm par défaut |

## Vérification

```
python3 check_boitier.py
```

Recalcule depuis `SysEx_Patcher.kicad_pcb` les axes des 16 potentiomètres, les
centres des 7 boutons et des 7 LEDs et les 4 trous M3, et les compare au bloc
« relevé sur le PCB » de `boitier.scad`.

> Le piège que ce script existe pour attraper : **l'ancre d'une empreinte n'est
> pas le centre du composant.** L'axe d'un RK09K est à (7,5 ; 2,5) de son ancre,
> le centre d'un `SW_PUSH_6mm` à (3,25 ; 2,25), celui d'une LED 3 mm à (1,27 ; 0).
> Percer sur les ancres décalerait tout de plusieurs millimètres.
