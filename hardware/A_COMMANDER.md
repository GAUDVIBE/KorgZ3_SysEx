# Liste d'achat — shield Korg Z3 SysEx v1.2

Quantités extraites du schéma et du PCB, pour **une carte**.
Carte **100 % traversante** — aucun composant monté en surface.

**Confronté au panier Mouser du 28/07/2026 (84,07 €).**

---

## 1. Déjà au panier, et conforme

| ☑ | Qté | Référence Mouser | Composant |
|---|---|---|---|
| ☑ | 1 | `737-ICM-624-1-GT-HT` | Support DIP-24, entraxe 15,24 mm — pour le 4067 |
| ☑ | 3 | `437-1168730841012101` | Support DIP-8 Preci-dip — pour le 6N138 (1 suffit) |
| ☑ | 19 | `594-MDSA104K15XH5TAA` | 100 nF — **voir §2** |
| ☑ | 1 | `502-TA5FSH` | Mini-XLR 5 points **femelle, côté câble** |
| ☑ | 7 | `SWT0325-073016TSK` (GCT) | Bouton tactile 6 × 6 mm traversant — **voir §2** |

---

## 2. À vérifier avant de valider

### ⚠️ Le mini-XLR de la carte manque

`TA5FSH` est le connecteur **de câble**. La carte, elle, porte un connecteur
**mâle à souder** : **`TRAPC5M1X`** — Switchcraft, 5 contacts, traversant,
coudé pour circuit imprimé. C'est lui qui correspond à l'empreinte
`MiniXLR-5_Switchcraft_TRAPC_Horizontal`. Sans lui, rien à brancher sur la carte.

#### Décodage, sur le plan `TRAPC_X - TRASM_X SERIES`, rév. C

| Champ | Valeur | Signification |
|---|---|---|
| `T` | T | TINI QG — la famille mini-XLR |
| `RA` | RA | **Right angle**, coudé |
| `PC` | PC | **Traversant** (`SM` = monté en surface) |
| `5` | 5 | 5 contacts, en **double rangée** de terminaisons |
| `M` | M | mâle |
| `1` | 1 | **sans filetage** |
| `X` | X | RoHS |

#### Empreinte vérifiée — 7 cotes sur 7

Confrontation de `MiniXLR-5_Switchcraft_TRAPC_Horizontal` au plan, feuille 2 :

| Cote | Empreinte | Plan |
|---|---|---|
| Entraxe des trous de fixation | 11,938 mm = 0,4700" | 0,470" |
| Ø des trous de fixation | 1,778 mm = 0,0700" | 0,070" |
| Ø de perçage des signaux | 1,016 mm = 0,0400" | 0,040" |
| Pas dans une rangée | 2,540 mm = 0,1000" | 0,100" |
| Décalage entre rangées, en x | 1,270 mm = 0,0500" | 0,050" |
| Écart entre rangées, en y | 2,540 mm = 0,1000" | 0,100" |
| Fixation → rangée proche, en y | 2,794 mm = 0,1100" | 0,110" |

Le circuit imprimé commandé accepte donc bien cette référence.

#### Pourquoi *pas* la version filetée `TRAPC5MX`

Le plan liste `TRAPC5M1X` et `TRAPC5MX` **sous la même implantation** : la carte
accepte les deux. La version sans le `1` apporte un fût fileté 7/16-32 UN-2A
avec écrou et rondelle, ce qui ferait encaisser l'effort de branchement par la
paroi du boîtier plutôt que par les soudures. Séduisant — mais inapplicable ici.

L'empreinte occupe **y 20,125 → 40,191**, soit les 20,07 mm du plan, filetage
compris. Le bord de la carte est à **y = 20,0** : la face avant du connecteur
affleure à 0,125 mm près, et les 7,11 mm de filetage s'étendent vers
l'**intérieur**. Un écrou se vissant depuis l'extérieur de la paroi, il n'aurait
jamais de quoi mordre.

> Rien ne soutient donc ce connecteur hors ses 5 soudures et ses 2 ergots — et
> le filetage n'y aurait rien changé. La parade est côté boîtier : percer au
> plus juste autour du fût pour que la paroi reprenne l'effort latéral.

### ⚠️ Les 100 nF sont en rupture

`594-MDSA104K15XH5TAA` : **stock 0, en attente de réapprovisionnement**. Ça
bloquerait toute la commande. Prendre une autre référence disponible.

Deux points sur ce modèle : sa description indique **« Axial Lead »** alors que
notre empreinte est **radiale, pas de 5,00 mm** (`C_Disc_D5.0mm_W2.5mm`), et
c'est une version automobile 100 V, inutilement chère. Un 100 nF céramique
radial 50 V au pas de 5 mm convient parfaitement.

### ⚠️ Le 100 µF : tension et quantité

`667-ECE-A1AN101X` : **10 V** sur un rail 5 V, c'est deux fois la tension de
service — acceptable mais sans marge. **16 V ou 25 V** serait plus sûr.
C'est aussi une version **bipolaire**, qui fonctionne mais coûte plus cher qu'un
polarisé ordinaire. Vérifier le **pas de 2,50 mm** et le **diamètre 6,3 mm**.

**Une seule pièce suffit** (C19), le panier en compte 10.

### ⚠️ Les boutons : écartement non documenté, et hauteur à arrêter

`SWT0325` (GCT) — **traversant**, corps 6,0 × 6,0 mm, SPST normalement ouvert,
100 000 cycles. Conforme sur tous les points vérifiables.

Mais **la fiche ne donne aucun plan d'implantation coté** : l'écartement des
broches n'y figure pas, et l'historique des révisions montre que le fabricant a
retiré ces schémas le 17/07/2025. Impossible donc de confirmer formellement les
**6,50 × 4,50 mm** de notre empreinte `SW_PUSH_6mm`.

Le risque reste faible : un tactile 6 × 6 traversant à 4 broches est toujours sur
cette trame. La pièce coûte quelques centimes, et en prendre une dizaine permet
d'essayer sans conséquence.

**Hauteur hors tout : 7,3 mm** — c'est le champ `0730` de la référence. Le
poussoir dépassera de 7,3 mm au-dessus du circuit. Si la façade du boîtier est
plus épaisse, le bouton sera inaccessible : c'est **le seul composant dont le
choix dépend du boîtier**, et il peut attendre que son épaisseur soit arrêtée.

### ⚠️ Les potentiomètres

16 × `652-PTV09A2020SB103` au panier, alors que tu m'as dit les avoir déjà.

Si c'est un rachat volontaire : notre empreinte est une **Alps RK09K**, broches
en ligne au pas de **2,5 mm** avec deux pattes de fixation. Vérifier que la
Bourns PTV09A a bien la même implantation avant de valider — les deux sont des
9 mm, mais l'écartement des pattes diffère selon les modèles.

### ⚠️ Les embases MIDI

2 × `806-KCDX-5S-N2` (Kycon). Notre empreinte `MIDI_DIN5_180deg` est
**sur mesure**, relevée sur le perçage de la carte d'origine :

- 5 broches signal Ø 1,4 mm en (−7,3 ; 0), (0 ; 0), (7,3 ; 0), (5 ; 2,5), (−5 ; 2,5)
- **2 trous de fixation Ø 1,4 mm en (±5 ; −9,7)**

Comparer avec le plan Kycon, en particulier la **position des ergots de
fixation** : c'est là que les modèles diffèrent le plus.

### ⚠️ Le jack d'alimentation

2 × `474-PRT-00119` (SparkFun). Notre empreinte `BarrelJack_Horizontal` a
**3 pastilles** en (0 ; 0), (−6 ; 0) et (−3 ; 4,7). Vérifier l'écartement des
broches sur le plan du fabricant.

---

## 3. Absent du panier

### Résistances — axiales 1/4 W, pas 10,16 mm

| ☐ | Qté | Valeur | Rôle |
|---|---|---|---|
| ☐ | **25** | **1 kΩ** | filtres RC des 16 pots et du jack, limitation des 7 LEDs, série du canal switch |
| ☐ | 3 | 220 Ω | MIDI IN et OUT |
| ☐ | 1 | 330 Ω | section MIDI |
| ☐ | 1 | 10 kΩ | tirage du 6N138 |
| ☐ | 1 | **470 kΩ** | **détection de présence du câble guitare** |

> Sans la 470 kΩ, le Z3 reçoit du pitch bend fantôme dès que la guitare est
> débranchée. Une seule pièce, facile à oublier.

### Semi-conducteurs

| ☐ | Qté | Référence |
|---|---|---|
| ☐ | 1 | **6N138** optocoupleur, DIP-8 |
| ☐ | 1 | 1N4148 (DO-35, pas 7,62 mm) |
| ☐ | 1 | 1N4004 (DO-41, pas 10,16 mm) |
| ☐ | **7** | LED 3 mm |

> Le **CD74HC4067** est fourni par un ami — vérifier à réception que
> l'écartement des rangées fait bien **15,24 mm** et non 7,62.

### Connecteurs et commandes

| ☐ | Qté | Élément |
|---|---|---|
| ☐ | 1 | **`TRAPC5M1X`** — mini-XLR mâle pour la carte |

### Barrettes d'interface Arduino Mega

**MÂLES**, simple rangée, pas 2,54 mm, broches droites. Nos empreintes sont des
`PinHeader` : sur un shield, la carte est au-dessus et le Mega s'enfiche
dessous, donc les broches pointent **vers le bas** pour entrer dans les
connecteurs femelles du Mega.

| ☐ | Qté | Format | Pour |
|---|---|---|---|
| ☐ | 5 | 1×8 | JMA1, JMA2, JMD2, JMD3, JMP1 |
| ☐ | 1 | 1×10 | JMD1 |
| ☐ | 2 | 1×4 | écran OLED (J5) et alimentation batterie (J4) |

> **Le plus simple : acheter des barrettes sécables de 40 positions** et les
> couper à la longueur. Deux ou trois réglettes couvrent tout, pour quelques
> euros, et il en reste toujours pour la suite.
>
> Une **barrette empilable** (femelle au-dessus, longues broches mâles en
> dessous) conviendrait aussi, à la même empreinte, si tu veux garder la
> possibilité de superposer une carte. Ce n'est pas nécessaire ici.

> Pas de barrette 2×18 : supprimée en v1.2, sa position réelle sur le Mega
> traversant une colonne de potentiomètres.

### Visserie et modules

| ☐ | Qté | Élément |
|---|---|---|
| ☐ | 4 | Vis M3 + entretoises (trous Ø 3,2 mm) |
| ☐ | 1 | **Arduino Mega 2560** |
| ☐ | 1 | Écran **OLED 128 × 32 I²C**, adresse 0x3C |

---

## 4. Côté guitare

| ☐ | Qté | Élément |
|---|---|---|
| ☐ | 1 | Molette de pitch **10 kΩ linéaire, à ressort de rappel** |
| ☑ | 1 | **`46311LDRX`** — Switchcraft, glissière 3 positions **SPTT** |
| ☐ | 4 | Résistance **2,2 kΩ** — l'échelle qui encode le switch |
| ☐ | 1 | Embase mini-XLR 5 points **femelle**, à encastrer dans le pickguard |
| ☐ | 1 | **Câble à fiche coudée** côté guitare |

### Le sélecteur de plage : `46311LDRX`

Switchcraft série 46300. **SPTT** — un pôle, trois directions — c'est exactement
la configuration attendue : le commun va vers le canal 2 du multiplexeur, les
trois directions vers les trois prises de l'échelle de 2,2 kΩ.

| Caractéristique | Valeur |
|---|---|
| Circuit | SPTT (= SP3T) |
| Positions | 3, **cran positif** à chacune |
| Courant admissible | 3 A alternatif, 0,5 A continu |
| Contacts | alliage de cuivre argenté |
| Boîtier | acier zingué, volet anti-poussière |

Le cran positif est ce qui compte le plus ici : il interdit les positions
intermédiaires, donc les tensions flottantes que le firmware interpréterait
comme un câble débranché.

**Choisi pour sa fermeté**, précisément parce qu'une glissière ne se déplace
pas par accident sous la main droite pendant le jeu — contrairement à un
sélecteur à lame de guitare.

> ⚠️ **Découpe rectangulaire dans le pickguard**, pas un perçage rond. Environ
> 19,5 × 35,7 mm hors tout d'après la fiche — **à confirmer sur le plan coté
> avant de tracer**. C'est irréversible : faire l'essai sur une chute de
> plastique d'abord.

Le sens de rotation n'est pas une contrainte : si la position haute ne tombe
pas sur la plage voulue, il suffit d'échanger deux fils, ou de permuter les
fenêtres dans `pitch_math.h`.

### La chaîne des genres, vérifiée sur la fiche Switchcraft

| Élément | Genre |
|---|---|
| Carte, `TRAPC5M1X` | **mâle** |
| Câble, extrémité carte | **femelle** — c'est le `TA5FSH` du panier |
| Câble, extrémité guitare | **mâle** |
| Embase du pickguard | **femelle** |

Pour la fiche coudée, Cable Techniques (série LPS, sortie orientable sur 60°) et
Remote Audio (TA5F-RA) sont les deux fournisseurs sérieux. Mesurer le diamètre
de perçage nécessaire **avant** de percer le pickguard.

#### `TA5FSH` — vérifié sur la fiche `TA_FSH SERIES`, rév. A

Décodage de la référence : `TA` = **TINI QG**, le nom Switchcraft du mini-XLR —
c'est ce champ qui garantit la compatibilité avec le `TRAPC` de la carte.
`5` = 5 contacts, `F` = femelle, `SH` = poignée blindée. Contacts argentés
(standard ; le suffixe `AU` donnerait de l'or, inutile ici).

| Cote | Valeur | Conséquence |
|---|---|---|
| Ouverture arrière | **Ø 7,4 mm** | **diamètre extérieur maximal du câble** |
| Longueur hors tout | 43,7 mm | dégagement devant la face du boîtier |
| Ø du corps | 10,5 mm | |

> ⚠️ **Brochage donné vue de face côté accouplement.** Une femelle vue de face
> est le miroir d'un mâle vu de face : recopier un plan sur l'autre inverse les
> broches 1 et 3. Contrôler la **continuité au multimètre** fil par fil avant
> le premier branchement — une inversion enverrait le +5 V sur la sortie de la
> molette.

La borne de masse du serre-câble est prévue pour recevoir le drain du blindage :
s'en servir, le câble longe des micros de guitare électrique.

---

## 5. Le circuit imprimé

**Commandé le 28/07/2026** — fichier `SysEx_Patcher_v1.2_JLCPCB.zip`.

| Paramètre | Valeur |
|---|---|
| Dimensions | 172 × 172 mm |
| **Couches** | **4** |
| Épaisseur | 1,6 mm |
| Perçage minimal | 0,30 mm |
| Piste et isolation minimales | 0,20 mm |
