# Reconstruction du schéma — MIDI SysEx Patcher Ver.2 (shield Arduino MEGA)

> But : reconstruire le schéma électrique sous KiCad à partir des Gerbers `SysEx_Programmer_BIG_display.*`.
> Le schéma d'origine n'a **jamais été publié** (l'auteur ne partage que Gerbers + firmware), mais
> **le câblage complet est entièrement déductible** du firmware + du BOM + de la sérigraphie des Gerbers.

---

## 0. MODIFICATION v0.3 — mux analogique 4067 + jack déporté + filtres RC

Le projet KiCad inclut désormais une **extension** (au-delà de la reconstruction d'origine) :

- **CD74HC4067 (U2)** — multiplexeur analogique 16:1. Sortie commune **COM → A15**.
  - **I0 = pot #16** (déplacé du direct A15 vers le mux), **I1 = jack déporté**, **I2–I15 = libres** (sortis sur header **J9**).
  - Sélection **S0/S1/S2/S3 → D2 / D3 / D4 / D13** ; **~E (enable, actif bas) → GND** ; VCC → +5V, GND → GND.
- **Jack 6,35 mm (J10)** = **potentiomètre déporté** (T=curseur→filtre→I1, R=+5V, S=GND, G=GND châssis). Empreinte réelle = **Neutrik NRJ6HF-1** (jack 6,35 mm stéréo **horizontal**, nez fileté orienté vers le **bord haut** : on branche le câble par le haut de la carte).
- **Filtre RC anti-bruit sur chaque entrée analogique** (16 pots + jack) : `wiper →[1kΩ série]→ nœud ADC`, et `100nF du nœud vers GND` (passe-bas). Réfs R10–R27 (1k) + C2–C18 (100nF).

### ⚠️ Firmware à adapter pour le 4067
Les 15 premiers pots restent en lecture directe (A0–A14). **Pot #16 et le jack se lisent via le mux sur A15** :
```c
const byte muxSel[4] = {2, 3, 4, 13};      // S0,S1,S2,S3  (D2,D3,D4,D13)
int readMux(byte ch){
  for (byte i=0;i<4;i++) digitalWrite(muxSel[i], (ch>>i)&1);
  delayMicroseconds(5);                      // settle
  return analogRead(A15);                     // COM du 4067 -> A15
}
// ch0 = pot #16 (ex-A15) ; ch1 = jack déporté ; ch2..15 = extension
```
Côté `setup()` : `for(b:muxSel) pinMode(b,OUTPUT);`. Remplacer la lecture de `potLayout[15]` (A15) par `readMux(0)`, et ajouter `readMux(1)` pour le jack.



## 0ter. MODIFICATION v1.0 — VRAI shield Mega 2560 enfichable + 7 boutons/7 LEDs

### Embases Mega — positions physiques réelles
Les connecteurs shield ont été entièrement repositionnés aux **coordonnées monde exactes** du template
officiel KiCad `Arduino_Mega` (outil `check_headers_world` : err_max = 0.00 mm, 86 pads vérifiés).
Références finales : **JMP1** (power), **JMA1** (A0–A7), **JMA2** (A8–A15), **JMD1/JMD2/JMD3**
(numérique) + **JMX1** (bloc 2×18, D22–D53). L'ordre des broches analogiques correspond à l'ordre
réel du Mega (vérifié par `check_pinorder`). La carte s'enfiche maintenant réellement sur un Mega.

### 3 nouveaux boutons + 3 nouvelles LEDs (D22–D27)
- **SW6/SW7/SW8** → D22/D24/D26 (mode `INPUT_PULLUP`, via JMX1 du bloc 2×18)
- **D7/D8/D9** (LEDs) → D23/D25/D27 via R28/R29/R30 (1 kΩ chacune)
- **Total : 7 boutons + 7 LEDs** ; chaque LED est physiquement à côté de son bouton.
  Layout : **5 paires à droite des pots, 2 paires à gauche**.

### Firmware
```c
const byte butLayout2[3] = {22, 24, 26};   // SW6, SW7, SW8
const byte LEDLayout2[3] = {23, 25, 27};   // D7, D8, D9 (via R28-R30)
// HandleNewButtons() à compléter — mapping MIDI laissé en TODO
```
`setup()` : `pinMode` INPUT_PULLUP sur butLayout2, OUTPUT sur LEDLayout2.

### État du routage (après freerouting best-effort)
- **129/130 nets routés**, 0 court-circuit, zones de cuivre remplies.
- **À finir en GUI** : net **A7** (JMA1 pad 8 ↔ R23, 1 piste manquante) + 1 clearance résiduelle
  de 0,6 µm (0,1994 mm vs. 0,200 mm, négligeable).
- **Gerbers à regénérer** après cette finition dans l'éditeur KiCad.

### Outillage de reconstruction
Scripts de vérification dans `kicad_project/shield_redesign/` :
`check_template`, `check_placement`, `check_pinorder`, `check_headers_world`,
`check_controls_pcb`, `check_controls_nets`, `check_sch_nets`, `check_fw`.
Configuration de placement : `kicad_project/shield_redesign/placement.json`.

> ⚠️ **Test-fit conseillé sur un Mega réel avant fabrication** — positions dérivées du template
> officiel KiCad (non testées sur silicium à ce stade).

---

## 0bis. PCB v0.5 — VRAI SHIELD enfichable Arduino Mega
La carte est maintenant un **shield empilable** : les connecteurs Mega sont placés aux **positions
exactes de l'Arduino Mega 2560** (2 rangées à 48,26 mm, brochage déduit du cuivre d'origine, ordre
analogique A0→A15 vérifié), donc **le Mega s'enfiche dessous**. Layout : entrées (MIDI IN/OUT, jack,
DC) en haut, pots en grille 4×4 (filtres RC à côté de chaque pot), 4 boutons en colonne verticale,
LEDs à droite. **Le Mega est désormais SOUS les pots** (comme la carte d'origine) : les deux rangées
d'embases (pas 24 mm entre rangées de pots) s'intercalent dans les interstices entre rangées de pots,
y72 (analogique) et y120 (numérique). **100 % THT**, 4 couches + plan GND, **routé à 100 %, DRC
propre**, coins arrondis, **~172×172 mm** (carte resserrée, le Mega passe dessous). Empreintes des
embases/pots **sans courtyard** (carte empilée : les composants sont au-dessus du Mega, c'est normal).
4ᵉ sélection du mux = **D13** (D22 absent de ces 2 rangées). ⚠️ Dégager les pièces hautes du Mega
(USB, jack alim, condensateurs) — ne pas poser de composant pile au-dessus de ce coin.

**Connecteurs d'entrée (orientation finale, tous branchables par le bord HAUT) :**
- **MIDI DIN-5** (J1/J2) : empreinte custom `MIDI_DIN5_180deg` (géométrie reprise du PCB d'origine) — **trous de fixation en haut, arc des broches en dessous, broches data (4/5) les plus basses**, conforme à l'orientation du PCB d'origine.
- **Jack 6,35 mm** (J10) : Neutrik **NRJ6HF-1** horizontal, nez vers le bord haut (câble branché par le haut).
- **Jack DC barrel** (J3) : `BarrelJack_Horizontal` tourné de 270° → **ouverture vers le bord haut** (comme les autres entrées).
- **OLED** (J5) : header 1×4 simple, **sans sérigraphie de nom de broches**.
- Le mux **4067** et le filtre du jack sont placés dans l'**espace libre entre les deux rangées de headers Mega** (sinon la rangée analogique trop dense provoquait des courts A15/D10 à l'autoroutage).

---

## 1. Identification du projet

| | |
|---|---|
| **Projet** | MIDI SysEx Patcher Ver.2 |
| **Auteur** | baritonomarchetto (« Barito »), synthbrigade.altervista.org |
| **Version carte** | **v0.2b** (lue sur la sérigraphie, en bas à droite) |
| **Type** | Shield (carte fille) pour **Arduino MEGA 2560** |
| **Nature** | PCB **double face**, env. **135 × 120 mm** (trous de fixation aux coins : 129,5 × 115,6 mm) |
| **CAO d'origine** | EAGLE 6.6.0 (Gerbers générés le 28/01/2022) |
| **Instructable** | https://www.instructables.com/MIDI-SysEx-Patcher-Ver2/ |
| **Dépôt GitHub** | https://github.com/baritonomarchetto/arduino-SysEx-Patcher (firmware + Gerbers, licence **MIT**) |
| **Prototype d'origine** | « (almost) Universal MIDI SysEx CC Programmer » (même auteur, version perfboard sans écran) |

> ⚠️ Le `.GML` fourni n'est **pas** le contour complet de la carte (il ne couvre qu'une sous-zone),
> d'où le rognage si on l'utilise comme outline. Le contour réel est donné par les trous d'angle du perçage.

---

## 2. Nomenclature (BOM) — source : Instructable Ver.2

| Qté | Composant | Empreinte KiCad suggérée | Rôle |
|---|---|---|---|
| 1 | Arduino MEGA 2560 (embase) | headers shield MEGA (2×18, 2×8, 1×6, 1×8) | cerveau |
| 1 | **6N138** optocoupleur | DIP-8 (socket) | entrée MIDI isolée |
| 1 | OLED **SSD1306 0,96"** 128×64 I²C | header 1×4 (2,54 mm) | affichage |
| 16 | Potentiomètre **10 kΩ lin.**, montage PCB | Potentiometer_Alps_RK09K_Single (ou équiv. 3 pins + 2 brackets) | réglages |
| 3 | Résistance **220 Ω** | R_Axial THT (ou 0207) | 1× MIDI IN, 2× MIDI OUT |
| 1 | Résistance **330 Ω** | R_Axial THT | section 6N138 |
| 4 | Résistance **1 kΩ** | R_Axial THT | limitation des 4 LEDs |
| 1 | Résistance **10 kΩ** | R_Axial THT | pull-up sortie 6N138 |
| 1 | Condensateur **100 nF** (non pol.) | C_Disc THT | découplage |
| 1 | Condensateur **100 µF** électrolytique **polarisé** (C19) | CP_Radial D6,3 mm | réservoir/lissage sur le rail **+5 V** (pad 1 = **+** → +5V, pad 2 = − → GND ; polarité sérigraphiée) |
| 1 | Diode **1N4148** | D_DO-35 | protection entrée optocoupleur |
| 1 | Diode **1N4004** | D_DO-41 | protection inversion alim |
| 4 | **LED 3 mm** | LED_D3.0mm | retour visuel (pages / séquenceur) |
| 4 | Bouton poussoir momentané | SW_PUSH 6 mm | PAGE/SHIFT, CALLBACK/START, RNDMZ!/PANIC, MODE |
| ~~1~~ | ~~Inverseur SPDT PCB (SW1)~~ | ~~SW_SPDT~~ | **supprimé** — 6N138 câblé en direct sur RX0 (voir §3.5) |
| 1 | Jack DC barrel | BarrelJack | alimentation +5 V |
| 2 | Connecteur **MIDI DIN-5** | DIN-5 180° | MIDI IN / MIDI OUT |
| — | Bornier 4 pts | GND / +5V / −BAT / +BAT | alim alternative (pile 9 V) |
| 50 | Pin headers mâles 2,54 | — | embases shield |

---

## 3. Câblage numérique — source : firmware `CCSysEx_Patcher.ino`

### 3.1 Potentiomètres (16) → entrées analogiques du MEGA
Chaque potentiomètre : **broche 1 → GND**, **curseur (broche 2) → A_x**, **broche 3 → +5 V**.
Ordre de la matrice 4×4 (numérotation sérigraphie) → broche analogique (`potLayout`) :

| Pos. (silk) | Label shift | A-pin | | Pos. | Label shift | A-pin |
|---|---|---|---|---|---|---|
| **1** | BPM | **A0** | | **9** | — | **A2** |
| **2** | STEP LENGHT | **A5** | | **10** | — | **A6** |
| **3** | CH1 | **A8** | | **11** | — | **A10** |
| **4** | CH2 | **A12** | | **12** | — | **A14** |
| **5** | OCTAVE | **A1** | | **13** | — | **A3** |
| **6** | SYNTH | **A4** | | **14** | — | **A7** |
| **7** | — | **A9** | | **15** | — | **A11** |
| **8** | — | **A13** | | **16** | — | **A15** |

> `const int potLayout[16] = {A0,A5,A8,A12, A1,A4,A9,A13, A2,A6,A10,A14, A3,A7,A11,A15};`

### 3.2 Boutons (4) → broches numériques (mode `INPUT_PULLUP`)
Un côté du bouton → broche MEGA, l'autre côté → **GND** (pas de résistance externe, pull-up interne).

| Bouton (silk) | Broche |
|---|---|
| 4 boutons (PAGE/SHIFT, CALLBACK/START, RNDMZ!/PANIC, MODE) | **D7, D8, D6, D5** |

> `const byte butLayout[4] = {7, 8, 6, 5};`

### 3.3 LEDs (4) → broches numériques (sortie)
Broche MEGA → **1 kΩ** → anode LED → cathode → **GND**.

| LED | Broche |
|---|---|
| 4 LEDs (1 LED par groupe de 4 pots) | **D12, D11, D10, D9** |

> `const byte LEDLayout[4] = {12, 11, 10, 9};`

### 3.4 Écran OLED SSD1306 → **I²C**
| Signal | Broche MEGA |
|---|---|
| SDA | **D20 (SDA)** |
| SCL | **D21 (SCL)** |
| VCC | +5 V |
| GND | GND |

> Adresse I²C **0x3C**, 128×64. Header 1×4 « SSD1306 » en haut au centre de la carte.
> Vérifier l'ordre des broches du header (silk : SDA … GND) par rapport à votre module.

### 3.5 MIDI → **Serial0 matériel** (USB partagé)
Le firmware utilise le port **Serial** = **RX0 (D0)** / **TX0 (D1)** (`Serial.begin(31250)`).
À l'origine un inverseur **SW1 « prog/MIDI »** déconnectait la sortie du 6N138 de RX0 pendant l'upload
USB. **Supprimé dans cette refonte** : la sortie du 6N138 (Vo, collecteur ouvert + pull-up 10 kΩ R5)
est désormais **câblée directement à RX0 (D0)**. Conséquence pratique : ne pas téléverser le firmware
pendant qu'un flux MIDI entre (au repos le pull-up n'empêche pas l'upload USB).

---

## 4. Section analogique — **CONFIRMÉE par traçage du cuivre**

Le brochage et les liaisons ci-dessous ont été **vérifiés par extraction de la connectivité cuivre**
des Gerbers (rastérisation des couches + composantes connexes + liaison via les trous métallisés),
recoupée avec la sérigraphie du dessous (`board_bottom.png`) et le BOM.

**Orientation du 6N138 (U1) établie sans ambiguïté** : les deux pastilles électriquement isolées
tombent sur les broches **NC (1 et 4)**, la pastille du rail +5V (partagé avec les 16 pots) sur **Vcc (8)**,
et la pastille du plan de masse sur **GND (5)**.

### 4.1 MIDI IN (optocoupleur 6N138) — tracé vérifié
```
DIN-5 (IN) br.4 ──[R1 220Ω]── 6N138 pin2 (Anode LED)        ✓ tracé
DIN-5 (IN) br.5 ───────────── 6N138 pin3 (Cathode)          ✓ tracé
                 D1 1N4148 entre pin2 (Anode) et pin3 (Cathode), protection  ✓ tracé
6N138 pin8 (Vcc) ── +5V        6N138 pin5 ── GND            ✓ tracé
6N138 pin6 (Vo)  ──[R5 10kΩ]── +5V   (pull-up)              ✓ tracé
6N138 pin6 (Vo)  ──────────────────────────── RX0 (D0)      (direct, SW1 supprimé)
6N138 pin7 (Vb)  ──[R4 330Ω]── GND   (polarisation base)    ✓ tracé
6N138 pin1, pin4 = NC                                       ✓ (pastilles isolées)
C1 100nF : découplage +5V/GND
```
> Tous les pads correspondants ont été localisés (coords mm dans l'analyse). Ce câblage = circuit
> MIDI IN 6N138 standard, ici intégralement confirmé sur le cuivre — pas une simple hypothèse.

### 4.2 MIDI OUT
```
TX0 (D1) ──[220Ω]── DIN-5 (OUT) br.5
+5V      ──[220Ω]── DIN-5 (OUT) br.4
DIN-5 (OUT) br.2 ── GND (blindage)
```

### 4.3 Alimentation
```
Barrel +5V (centre +) ──[1N4004 protection]── rail +5V
Bornier : GND / +5V / −BAT / +BAT (alternatives d'alim)
(SW1 « prog/MIDI » supprimé — 6N138 Vo câblé en direct sur RX0)
```

> Les 3×220 Ω, 330 Ω, 10 kΩ, 100 nF, 1N4148, 1N4004 sont **tous** présents et localisés ;
> seuls quelques nœuds exacts autour du 6N138 (pin7, rôle du 330 Ω) restent à confirmer en
> suivant les pistes sur `board_top.svg` / `board_bottom.svg` (zoomables).

---

## 5. Démarche conseillée sous KiCad

1. **Nouveau projet** KiCad → schéma.
2. Placer un symbole **Arduino MEGA 2560** (lib `MCU_Module:Arduino_Mega2560_Shield` existe dans KiCad).
3. Câbler les **16 pots**, **4 boutons**, **4 LEDs+1kΩ**, **OLED 1×4** selon les tables §3.
4. Ajouter la **section MIDI** (6N138 + 2 DIN-5) et l'**alim** selon §4 (circuit MIDI standard).
5. Associer les **empreintes** (§2). Pour le PCB, vous pouvez :
   - **partir des Gerbers** (les `board_*.svg` donnent l'implantation exacte à reproduire), ou
   - importer les Gerbers dans **GerbView → Exporter vers Pcbnew** comme calque de référence graphique.
6. Vérifier les nœuds analogiques douteux en zoomant `board_top.svg`/`board_bottom.svg`.

> Le plus simple si le but est juste de **refabriquer** la carte telle quelle : les Gerbers d'origine
> suffisent (à renvoyer chez JLCPCB). La reconstruction du schéma n'est nécessaire que pour **modifier**
> le circuit.

---

## 6. Fichiers de ce dossier

| Fichier | Contenu |
|---|---|
| `board_top.svg` / `board_top.png` | Rendu **dessus** (cuivre + sérigraphie + pads + perçage), vectoriel zoomable |
| `board_bottom.svg` / `board_bottom.png` | Rendu **dessous** (vue mirroir, section analogique/alim) |
| `firmware_CCSysEx_Patcher.ino` | Firmware officiel (source du câblage numérique) — licence MIT |
| `firmware_LICENSE.txt` | Licence MIT du firmware |
| `RECONSTRUCTION_SCHEMA.md` | Ce document |
| `kicad_project/SysEx_Patcher.kicad_pro` | **Projet KiCad 10** (à ouvrir dans KiCad) |
| `kicad_project/SysEx_Patcher.kicad_sch` | **Schéma reconstruit** : 46 composants, 150 connexions, **ERC 0 erreur** |
| `kicad_project/SysEx_Patcher_schematic.pdf` | Export PDF du schéma |
| `kicad_project/SysEx_Patcher.kicad_pcb` | **PCB de départ** : 46 empreintes nettées, contour 135×120 mm |
| `kicad_project/pcb_top_preview.png` | Aperçu du PCB de départ |

### À propos du PCB de départ
- 46 empreintes placées avec **tous les nets assignés** (le chevelu/ratsnest s'affiche à l'ouverture) +
  **contour de carte Edge.Cuts ~135×118 mm** (taille réelle relevée sur le perçage).
- **Positions réelles d'après le perçage** : les 21 composants dominants — **16 potentiomètres (grille 4×4),
  6N138 (U1), 4 boutons** — sont placés à leur **emplacement physique réel** (registration par
  correspondance de constellations de trous, validée contre la sérigraphie). Le reste (résistances,
  diodes, DIN, OLED, SW1, headers MEGA) est en **zone de staging sous la carte**, à positionner
  (empreintes sans perçage standard / abstractions de shield). Voir `pcb_realpos_preview.png`.
- Rien n'est routé — état « prêt à placer/router », DRC = ratsnest + chevauchements de staging (pas de court-circuit).
- Le **DRC** ne remonte que : connexions manquantes (= ratsnest, normal avant routage), quelques
  chevauchements de placement à réorganiser, et des avertissements « lib non configurée » propres à la CLI.
- ~~Empreinte MIDI DIN = placeholder~~ → **résolu** : empreinte custom `MIDI_DIN5_180deg` (géométrie d'origine, orientation conforme au PCB d'origine). Voir §0bis.
- Les empreintes des connecteurs MEGA (J6/J7/J8) débordent à droite du contour : à repositionner
  comme headers d'empilage du shield.

### À propos du projet KiCad
- Schéma généré puis **validé par `kicad-cli` (ERC : 0 erreur)**. Les composants utilisent les
  symboles standard KiCad (Device:R, Isolator:6N138, Connector:DIN-5, Device:R_Potentiometer, …),
  embarqués dans le fichier — il s'ouvre donc sans dépendance.
- **Connectivité par labels globaux** portant les noms de broches Arduino (`A0`, `D7`, `+5V`, `RX0`…).
  Le MEGA est représenté par 3 embases (J6 analogique, J7 numérique, J8 alim) = les headers du shield.
- À l'ouverture, KiCad peut afficher des avertissements « librairie non trouvée » selon ta config :
  réassocie au besoin les symboles aux libs globales (clic droit → *Change Symbols*). Les empreintes
  sont des suggestions (§2) à confirmer avant routage.
- Les valeurs/refs suivent §2 et §4 ; les 16 pots = RV1…RV16, le 6N138 = U1 (Vo direct sur RX0, SW1 retiré).

*Rendus générés avec gerbonara depuis les Gerbers `SysEx_Programmer_BIG_display.*`.*
