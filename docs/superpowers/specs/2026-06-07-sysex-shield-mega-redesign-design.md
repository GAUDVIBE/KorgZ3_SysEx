# Design — SysEx Patcher : passage en vrai shield Arduino Mega 2560 + 3 boutons / 3 LEDs

> Date : 2026-06-07 · Projet : `KorgZ3_SysEx/hardware/kicad_project/SysEx_Patcher` (rev actuelle v0.9)
> Statut : design validé, prêt pour plan d'implémentation.

## 1. Objectif & périmètre

Transformer le PCB `SysEx_Patcher` en **vrai shield enfichable Arduino Mega 2560** : replacer toutes
les embases Mega aux **positions physiques réelles** (issues du template officiel KiCad `Arduino_Mega`),
ajouter le **bloc 2×18 (D22–D53)**, et y câbler **3 nouveaux boutons + 3 nouvelles LEDs** (D22–D27).
On **garde l'allure actuelle** : outline ~172×172 mm, grille 4×4 de 16 pots, MIDI IN/OUT (DIN-5),
jack 6,35 déporté, OLED, jack DC, mux 4067 + filtres RC — tous conservés.

**Hors périmètre :** redesign compact, réduction de la carte à l'empreinte Mega, refonte du firmware
au-delà de l'ajout des 3 boutons/3 LEDs, ajout de trous de fixation Mega ou de stacking headers
(mécanique laissée « minimale », gérée par l'utilisateur).

### Pourquoi ce redesign

La carte v0.9 **ne s'enfiche pas** sur un Mega réel : l'ordre physique des broches sur ses embases ne
correspond pas à celui d'un Arduino Mega 2560 (vérifié contre le template officiel KiCad — ni en direct,
ni en miroir). Elle se reliait donc au Mega par fils. Ce redesign corrige cela en adoptant la géométrie
réelle du template, rendant la carte compatible broche-à-broche.

```
Analogique (x croissants) — vrai Mega : A7 A6 A5 A4 A3 A2 A1 A0 | A15 A14 … A9 A8
                              v0.9      : A0 A1 A2 A3 A4 A5 A6 A7 A8 … A15  (monotone, NON conforme)
```

## 2. Approche d'exécution : hybride (C)

1. **Calculer** la géométrie autoritaire depuis le template officiel KiCad
   `/Applications/KiCad/KiCad.app/Contents/SharedSupport/template/Arduino_Mega/Arduino_Mega.kicad_pcb`
   (positions des embases J1–J7, mapping pad→net) + les helpers du toolchain `/tmp/kgen`.
2. **Appliquer chirurgicalement** sur les fichiers v0.9 (`.kicad_sch` + `.kicad_pcb`) — pas de
   régénération from-scratch (le générateur `gen_shield2.py` est resté en v0.7 et a divergé).
3. **Re-router** (freerouting), **DRC**, **regénérer Gerbers**, **mettre à jour le firmware**.

## 3. Géométrie des embases Mega (référence : template officiel)

Le template définit les embases d'un shield Mega (vue de dessus du shield), espacement des deux rangées
longues = **48,26 mm** (1,9"), pas 2,54 mm :

| Réf template | Type | Broches |
|---|---|---|
| J1 | 1×08 | NC, IOREF, ~RESET, +3V3, +5V, GND, GND, VIN |
| J3 | 1×08 | A0, A1, A2, A3, A4, A5, A6, A7 |
| J5 | 1×08 | A8, A9, A10, A11, A12, A13, A14, A15 |
| J2 | 1×10 | SCL/21, SDA/20, AREF, GND, D13, D12, D11, D10, D9, D8 |
| J4 | 1×08 | D7, D6, D5, D4, D3, D2, TX0/D1, RX0/D0 |
| J6 | 1×08 | TX3/D14, RX3/D15, D16, D17, D18, D19, SDA/20, SCL/21 |
| **J7** | **2×18** | bloc digital étendu — voir ci-dessous |

**Bloc 2×18 (J7) — affectation des pads (template) :** colonne paire et impaire au pas 2,54 mm.
Pins utiles pour ce projet (origine template, rot 180) :

| Signal | pad | Signal | pad |
|---|---|---|---|
| GND | 1,2 | … | … |
| **D22** | 33 | **D23** | 34 |
| **D24** | 31 | **D25** | 32 |
| **D26** | 29 | **D27** | 30 |
| (D28…D53) | 3–28 | +5V | 35,36 |

**Placement dans notre carte :** le bloc J1–J7 est posé comme **unité rigide** (translation +
orientation/miroir vertical pour mettre la rangée A0–A15 côté pots et la rangée numérique de l'autre,
conformément à l'allure actuelle), positionné pour que **le 2×18 s'étende dans la zone libre côté
A8/A15**, sans collision avec les pots ni les colonnes de boutons. Vérification par **rendu PNG** du
board pendant l'implémentation.

**Mapping pad→net :** chaque pad reçoit son net réel (A0…A15, D0…D21, D22…D27, +5V/GND/VIN). Tous les
nets existants restent identiques — **seuls les pads physiques se déplacent**.

## 4. Boutons & LEDs (7 paires bouton+LED)

- **7 paires bouton+LED**, chaque LED **à côté** de son bouton.
- Répartition : **5 paires à droite des pots**, **2 paires à gauche**.
- **Existants conservés** : 4 boutons sur **D5, D6, D7, D8** ; 4 LEDs sur **D9, D10, D11, D12**.
- **Nouveaux** : 3 boutons sur **D22, D24, D26** ; 3 LEDs sur **D23, D25, D27** (via J7).

**Électrique (identique à l'existant) :**
- Bouton : un côté → broche Mega, l'autre → **GND**, mode `INPUT_PULLUP` (pas de R externe).
- LED : broche Mega → **1 kΩ** → anode LED → cathode → **GND**.

**Nouvelles références :** `SW5`, `SW6`, `SW7` (boutons) ; 3× `LED` ; `R28`, `R29`, `R30` (1 kΩ).
Empreintes alignées sur l'existant : `Button_Switch_THT:SW_PUSH_6mm`, `LED_THT:LED_D3.0mm`,
`Resistor_THT:R_Axial_DIN0207_...`.

## 5. Modifications schéma

- **Étendre la représentation Mega** pour exposer **D22–D27** (ajout des pins sur le symbole
  `Arduino_Mega2560`, ou symbole connecteur 2×18 dédié) afin que le netlist relie les nouveaux
  contrôles aux nets D22–D27.
- **Ajouter l'empreinte/symbole J7 (2×18)** : pads D22–D27 câblés aux nouveaux boutons/LEDs ;
  +5V/GND du bloc reliés aux rails ; **D28–D53 en no-connect** (présents pour le maté mécanique).
- **Ajouter** 3× `SW_Push` (→ GND), 3× `LED`, 3× `R 1k`, câblés comme au §4.
- Réordonner/replacer les empreintes des 7 embases Mega selon §3.

## 6. Routage, vérif & sortie

1. **Rip-up** du routage lié aux broches Mega déplacées ; conserver le reste (boucle MIDI, OLED, rails)
   autant que possible.
2. **Re-route** via freerouting (`.dsn` → `.ses`), plusieurs passes attendues (board 4 couches + plan
   GND, ~92 empreintes → ~98 après ajout).
3. **DRC propre** (0 violation) via `kicad-cli`.
4. **Regénérer les Gerbers** (zip fab).
5. **Firmware** (`firmware_CCSysEx_Patcher.ino`) :
   ```c
   const byte butLayout2[3] = {22, 24, 26};   // nouveaux boutons (INPUT_PULLUP)
   const byte LEDLayout2[3] = {23, 25, 27};    // nouvelles LEDs (OUTPUT)
   ```
   + `pinMode` et lecture/écriture alignés sur la logique des boutons/LEDs existants.

## 7. Risques & mitigations

| Risque | Mitigation |
|---|---|
| **Convergence du re-routage** (le gros morceau) | Itérations freerouting comme historiquement ; vérifier DRC à chaque passe ; ajuster placement si nets bloqués. |
| **Encombrement** 2×18 + colonnes boutons + pots sur 172×172 | Rendu PNG avant routage ; déplacer le 2×18 / colonnes si collision. |
| **Compatibilité fab** | Positions issues du template officiel ; **test-fit conseillé sur Mega réel avant commande**. |
| **Divergence générateur v0.7 vs fichiers v0.9** | Approche hybride : on n'utilise le générateur que pour le calcul, on édite les fichiers v0.9. |

## 8. Critères de succès

- La carte expose les embases Mega aux **positions réelles du template** (vérifiable : ordre des
  broches == ordre Mega, direct ou miroir selon montage).
- 2×18 présent, **D22–D27** reliés à 3 boutons + 3 LEDs fonctionnels.
- **7 boutons + 7 LEDs** placés (5 paires à droite, 2 à gauche), chaque LED près de son bouton.
- **DRC 0 erreur**, routage complet, **Gerbers regénérés**.
- Firmware mis à jour pour lire/piloter les 3 nouveaux contrôles.
- Tous les sous-systèmes existants (pots, mux, jack, MIDI, OLED) préservés et toujours connectés.
