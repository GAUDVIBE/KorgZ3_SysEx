# Boîtier pupitre — shield Korg Z3 SysEx v1.2

Modèle paramétrique OpenSCAD. Cinq pièces, aucune vis autre que du M3.

```
openscad -o fond.stl -D 'piece="fond"' boitier.scad
```

| Pièce | Nombre | Encombrement | Impression |
|---|---|---|---|
| `fond` | 1 | 182,5 × 189,6 × 78,6 mm | posé sur son dessous, sans support |
| `facade` | 1 | 182,5 × 149,5 × 10,0 mm | face visible **contre le plateau** |
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

**Deux niveaux.** L'axe court impose une façade à 8,5 mm de la carte, alors que
le mini-XLR mesure 13,7 mm de haut et les embases MIDI davantage. Les
connecteurs sont donc logés sous un **dosseret** surélevé à l'arrière.

**Les cotes incertaines sont sur une pièce sacrificielle.** Les hauteurs des
embases au-dessus de la carte sont les seules valeurs que le fichier KiCad ne
donne pas. Elles sont toutes portées par `plaque_arriere`, qui coûte vingt
minutes d'impression au lieu de dix heures.

## Ce sont les potentiomètres qui tiennent la façade

Leur canon fileté de 5 mm traverse la façade de 3 mm et reçoit son écrou. Seize
écrous répartis sur la surface donnent bien plus de rigidité que quatre vis
d'angle — c'est le montage de n'importe quel synthétiseur.

```
axe                       15 mm  au-dessus de l'épaulement du canon
façade                   − 3 mm
                         ───────
axe émergent               12 mm      ← de quoi tenir n'importe quel bouton

canon fileté               5 mm
façade                   − 3 mm
                         ───────
filetage pour l'écrou      2 mm      ← un écrou de pot en fait ~1,6
```

Ces deux valeurs sont **calculées et affichées à chaque compilation**, et une
assertion arrête le rendu si la façade devient trop épaisse pour l'écrou :

```
ECHO: "Axe emergent au-dessus de la facade : 12 mm"
ECHO: "Filetage restant pour l'ecrou       : 2 mm"
ECHO: "Lamage sous la facade pour boutons  : 1 mm"
```

**D'où la règle : `facade_ep` ne doit jamais dépasser `pot_filetage_h − 1,6`.**

### Les boutons dépassent l'épaulement

Le poussoir monte à 7,3 mm quand l'épaulement des potentiomètres est à 7,0. La
façade est donc **lamée par-dessous** de 1 mm, sur Ø 9, en regard de chaque
bouton. Le lamage sert aussi de logement à la collerette du capuchon, qui s'y
trouve captif : il ne peut ni tomber ni ressortir.

## ⚠️ Deux points à vérifier avant d'imprimer

### La seule cote encore manquante

`pot_corps_h` — **la hauteur du corps du potentiomètre au-dessus de la carte**,
jusqu'à l'épaulement du canon. C'est elle qui fixe toute la hauteur de la
façade, et le lamage des boutons en découle. Valeur provisoire : 7,0 mm.

### Le connecteur de batterie `J4`

`J4` est à `(116, 132)`, **sous la façade**. Une barrette mâle ordinaire monte à
environ 8,4 mm, contre 7,0 mm de garde : si tu la peuples, elle touchera.
Laisse-la nue, ou demande-moi d'ajouter un lamage à son emplacement.

### L'écran n'est pas en face de son connecteur

`J5` se trouve à **Y = 160**, c'est-à-dire **sous le dosseret**, alors que la
fenêtre est percée dans la façade à `OLED = [142, 128]`. L'écran demandera donc
quelques centimètres de fil et un support collé sous la façade.

C'est délibéré : je ne connais ni les dimensions du module ni son mode de
fixation. Dis-moi le modèle exact et je dessine un berceau à sa cote.

## Ce qu'il faut mesurer

En tête de `boitier.scad`, section « À MESURER ». Ce sont les seules valeurs
non déduites du PCB.

| Paramètre | Défaut | Comment le relever |
|---|---|---|
| `pot_corps_h` | 7,0 | ⚠️ **manquante** — carte → épaulement du canon |
| `pot_axe_h` | 15,0 | ✅ donnée : bout de l'axe depuis l'épaulement |
| `pot_filetage_h` | 5,0 | ✅ donnée : longueur du canon fileté |
| `pot_canon_d` | 7,0 | Ø extérieur du canon (M7 sur un pot 9 mm) |
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
6. Poser la `facade`, puis **serrer les 16 écrous de potentiomètre**. Ce sont
   eux qui la tiennent et la raidissent.
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
