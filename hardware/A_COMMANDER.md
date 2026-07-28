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
| ☐ | 2 | Barrette **mâle 1×4**, pas 2,54 mm — écran OLED et alimentation batterie |

### Barrettes d'interface Arduino Mega

**Femelles**, pas 2,54 mm, à longues broches si tu veux pouvoir empiler.

| ☐ | Qté | Format |
|---|---|---|
| ☐ | 5 | 1×8 |
| ☐ | 1 | 1×10 |

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
| ☐ | 1 | Switch **3 positions ON-ON-ON** |
| ☐ | 4 | Résistance **2,2 kΩ** — l'échelle qui encode le switch |
| ☐ | 1 | Embase mini-XLR 5 points **femelle**, à encastrer dans le pickguard |
| ☐ | 1 | **Câble à fiche coudée** côté guitare |

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
