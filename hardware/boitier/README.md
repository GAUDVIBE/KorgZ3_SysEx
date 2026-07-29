# Boîtier pupitre — shield Korg Z3 SysEx v1.2

Modèle paramétrique OpenSCAD. Cinq pièces, aucune vis autre que du M3.

```
openscad -o fond.stl -D 'piece="fond"' boitier.scad
```

| Pièce | Nombre | Encombrement | Impression |
|---|---|---|---|
| `fond` | 1 | 182,5 × 190,0 × 78,6 mm | posé sur son dessous, sans support |
| `facade` | 1 | 182,5 × 149,5 × 11,5 mm | face visible **contre le plateau** |
| `capot_arriere` | 1 | 176,1 × 25,0 × 29,5 mm | couché sur son tablier |
| `plaque_arriere` | 1 | 135,6 × 30,6 × 3,0 mm | à plat |
| `capuchon` | **7** | 8,5 × 8,5 × 6,2 mm | à plat |

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

## ⚠️ Deux points à trancher avant d'imprimer

### L'axe des potentiomètres ne dépasse que de 3,5 mm

```
axe                 15,0 mm au-dessus de la carte
façade, dessous    − 8,5 mm
façade, épaisseur  − 3,0 mm
                   ─────────
émergent             3,5 mm
```

C'est trop peu pour la plupart des boutons. Trois façons de gagner :

| Levier | Gain | Ce que ça coûte |
|---|---|---|
| `facade_ep` à 2,0 mm | +1,0 mm | façade plus souple |
| `ecart_facade` à 7,5 mm | +1,0 mm | ne marche que si le corps du pot fait ≤ 6,5 mm |
| Boutons à faible alésage | — | rien, mais choix restreint |

**Mesure d'abord ton axe réel.** Le « 15 mm » est une hypothèse : si tes
potentiomètres en donnent 18 ou 20, tout ce paragraphe tombe.

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
| `pot_corps_h` | 7,0 | hauteur du corps du pot au-dessus de la carte |
| `pot_axe_h` | 15,0 | bout de l'axe, depuis la surface de la carte |
| `pot_percage` | 7,5 | Ø du canon fileté, ou de l'axe s'il n'y en a pas |
| `bouton_h` | 7,3 | donnée par la référence GCT `SWT0325-**0730**16TSK` |
| `midi_z` | 11 | hauteur de l'axe de l'embase MIDI |
| `midi_d` | 23 | Ø d'une fiche DIN 5 |
| `xlr_z` | 12 | hauteur de l'axe du `TB5M` |
| `xlr_percage` | **12,5** | ⚠️ à confirmer sur la fiche `TB5M` |
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

## Assemblage

1. Visser la carte sur les quatre entretoises du `fond`, M3 × 8.
2. Glisser `plaque_arriere` par le haut dans sa feuillure.
3. Câbler le `TB5M` aux pastilles de `J10`.
4. Poser `capot_arriere`, deux M3 × 12 dans les parois latérales.
5. Poser les 7 `capuchon` sur les boutons.
6. Poser la `facade` : ses quatre bossages font entretoise jusqu'à la carte.
   Quatre M3 × 20 la traversent et se vissent dans les entretoises du fond.

Les quatre mêmes vis tiennent donc la façade **et** la carte.

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
