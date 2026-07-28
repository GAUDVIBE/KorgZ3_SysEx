# Nomenclature — shield SysEx Patcher v1.2

Extraite du schéma et du PCB le 2026-07-28 (`kicad-cli sch export bom` + relevé des
empreintes présentes uniquement sur le PCB). Carte **172 × 172 mm**, 4 couches,
**100 % traversant** — aucun composant monté en surface.

> Les **16 potentiomètres RV1–RV16** sont déjà en possession du constructeur et ne
> figurent donc pas dans les quantités à commander. Ils sont rappelés en fin de document
> pour mémoire.

---

## 1. Composants de la carte

### Résistances — toutes axiales 1/4 W, pas 10,16 mm

| Réf | Valeur | Qté | Rôle |
|---|---|---|---|
| R1–R3 | 220 Ω | 3 | limitation MIDI OUT / MIDI IN |
| R4 | 330 Ω | 1 | MIDI |
| R5 | 10 kΩ | 1 | tirage du 6N138 (collecteur ouvert) |
| R6–R25, R27–R30, R32 | **1 kΩ** | **25** | filtres RC analogiques (16 pots + jack), limitation des LED, série du canal switch |
| R31 | **470 kΩ** | 1 | **détection de présence du câble guitare** — ne pas omettre |

> R26 n'existe pas : simple trou dans la numérotation, hérité de la reconstruction.

### Condensateurs

| Réf | Valeur | Qté | Empreinte |
|---|---|---|---|
| C1–C18, C20 | 100 nF céramique | **19** | disque Ø5 mm, pas 5,00 mm |
| C19 | 100 µF électrolytique | 1 | radial Ø6,3 mm, pas 2,50 mm — **polarisé, repère + sérigraphié** |

### Semi-conducteurs

| Réf | Composant | Qté | Boîtier |
|---|---|---|---|
| U1 | **6N138** optocoupleur | 1 | DIP-8 |
| U2 | **CD74HC4067** multiplexeur analogique 16:1 | 1 | DIP-24 large (15,24 mm) |
| D1 | 1N4148 | 1 | DO-35, horizontal, pas 7,62 mm |
| D2D | 1N4004 | 1 | DO-41, horizontal, pas 10,16 mm |
| D3–D9 | LED 3 mm | **7** | LED_D3.0mm |

### Connecteurs et commandes

| Réf | Composant | Qté | Note |
|---|---|---|---|
| J1, J2 | Embase MIDI DIN 5 broches, 180° | 2 | corps **20,5 × 15,0 mm** ; entraxe des deux embases : 37 mm |
| J3 | Jack d'alimentation barrel horizontal | 1 | entrée continue de la carte |
| J4 | Barrette 1×4 mâle 2,54 mm | 1 | alimentation batterie (GND / +5V / GND / VIN) |
| J5 | Barrette 1×4 mâle 2,54 mm | 1 | OLED (GND / VCC / SCL / SDA) |
| **J10** | **Mini-XLR 5 points Switchcraft TRAPC**, montage horizontal sur circuit | 1 | vers la guitare — 4 broches câblées, la 5ᵉ en réserve |
| SW2–SW8 | Bouton poussoir 6 mm | **7** | il n'y a pas de SW1 (supprimé en v0.7) |

### Connecteurs d'interface Arduino Mega

Ils n'apparaissent pas dans la nomenclature du schéma : sur un shield, ce sont les
connecteurs du PCB qui **sont** l'interface Mega, ils n'ont pas de symbole associé.

| Réf | Composant | Qté |
|---|---|---|
| JMA1, JMA2, JMD2, JMD3, JMP1 | Barrette **1×8** femelle 2,54 mm | **5** |
| JMD1 | Barrette **1×10** femelle 2,54 mm | 1 |

> **Pas de barrette 2×18.** Elle figurait en v1.1 pour exposer D22 à D53, mais sa
> position réelle sur le Mega traverse la colonne de potentiomètres
> RV3/RV7/RV11/RV15 — courts-circuits garantis. Elle n'apportait que 6 signaux
> utiles sur 36 ; ceux-ci sont reportés sur **D14 à D19**, libres sur `JMD3`.

> Prendre des barrettes **femelles à longues broches** (type « stackable header ») si tu
> veux pouvoir empiler autre chose au-dessus du shield.

### Visserie

| Réf | Élément | Qté |
|---|---|---|
| MH1–MH4 | Vis M3 + entretoises, trous Ø 3,2 mm aux quatre coins | 4 jeux |

### Hors carte

| Élément | Qté | Note |
|---|---|---|
| **Arduino Mega 2560** | 1 | la carte s'enfiche dessus |
| Écran **OLED 128 × 32 I²C** | 1 | adresse 0x3C, se branche sur J5 |

---

## 2. Côté guitare

Monté sur la Stratocaster, relié au shield par un unique câble de 4 conducteurs.

| Élément | Qté | Note |
|---|---|---|
| Molette de pitch **10 kΩ** à ressort de rappel | 1 | linéaire |
| Switch **3 positions ON-ON-ON** | 1 | sélection du bend range |
| Résistances **2,2 kΩ** | **4** | en série entre +5 V et GND ; le switch pioche les taps à ¼, ½ et ¾ Vcc |
| Embase mini-XLR 5 points à encastrer | 1 | dans le pickguard |
| Câble mini-XLR 5 points, **fiche coudée côté guitare** | 1 | le coude évite que le câble pointe perpendiculairement au corps |

### Deux points à vérifier avant de commander

**Le genre des connecteurs.** L'embase de la carte et celle du pickguard doivent être
complémentaires des deux fiches du câble. C'est l'erreur d'achat classique sur ce type de
liaison : vérifier sur la fiche technique Switchcraft si l'empreinte TRAPC correspond à un
mâle ou à une femelle, et commander le câble en conséquence.

**La fiche coudée.** C'est le point d'approvisionnement le plus incertain du projet, et il
conditionne le montage sur le pickguard.

### Pourquoi quatre résistances et pas deux

Les trois positions du switch valent ¼, ½ et ¾ de Vcc, **jamais 0 V**. Câble débranché,
R31 (470 kΩ sur la carte) tire la ligne à ~0 V, valeur qui n'appartient à aucune position
valide : le firmware sait qu'il n'y a rien de branché, gèle le pitch au centre et ignore la
molette, au lieu de lire une entrée en l'air et d'émettre un pitch bend fantôme. Un
diviseur à deux résistances ne permettrait pas cette signature.

---

## 3. Pour mémoire — déjà en possession

| Réf | Composant | Qté |
|---|---|---|
| RV1–RV16 | Potentiomètre **10 kΩ** linéaire, Alps RK09K vertical | 16 |

---

## Récapitulatif des quantités à commander

| Catégorie | Détail |
|---|---|
| Résistances | 3 × 220 Ω, 1 × 330 Ω, 1 × 10 kΩ, **25 × 1 kΩ**, 1 × 470 kΩ, **4 × 2,2 kΩ** (guitare) |
| Condensateurs | **19 × 100 nF**, 1 × 100 µF |
| Semi-conducteurs | 1 × 6N138, 1 × CD74HC4067, 1 × 1N4148, 1 × 1N4004, **7 × LED 3 mm** |
| Connecteurs carte | 2 × DIN 5, 1 × barrel, 2 × barrette 1×4, 1 × mini-XLR 5 |
| Barrettes Mega | 5 × 1×8, 1 × 1×10 (femelles) |
| Commandes | **7 × bouton 6 mm** |
| Visserie | 4 × M3 + entretoises |
| Guitare | 1 molette 10 kΩ, 1 switch ON-ON-ON, 1 embase mini-XLR, 1 câble à fiche coudée |
| Modules | 1 × Arduino Mega 2560, 1 × OLED 128×32 I²C |
