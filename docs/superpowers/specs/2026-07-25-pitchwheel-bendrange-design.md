# Molette de pitch déportée + switch de bend range — design

**Date :** 2026-07-25
**Branche :** `pitchwheel-integration` (ex-`shield-mega-redesign`)
**Version matérielle visée :** shield Mega **v1.1**
**Firmware visé :** `KorgZ3_SysEx_08-05-2026/`

---

## 1. Objectif

Ajouter au contrôleur Korg Z3 une **molette de pitch** et un **switch 3 positions de bend
range**, tous deux montés **sur la guitare** (Stratocaster) et reliés au shield par **un seul
câble**.

Le Korg Z3 mappe en dur la plage MIDI `[0..16383]` sur ±12 demi-tons ; c'est figé dans son
firmware. Pour obtenir une amplitude musicale plus fine, on restreint la course MIDI émise
depuis l'Arduino. Le switch choisit cette restriction.

| Position du switch | Amplitude musicale | `BEND_SEMITONES` |
|---|---|---|
| 1 | ±1 ton | 2 |
| 2 | ±1 ton et demi | 3 |
| 3 | ±1 octave | 12 |

**Hors périmètre.** Les 3 boutons (SW6/SW7/SW8) et 3 LEDs (D7/D8/D9) présents sur le shield
v1.0 restent en réserve : broches déclarées, aucune fonction. Le reste du sketch (presets,
EEPROM, OLED, SysEx, 16 pots, 5 pages) n'est pas modifié au-delà des points de contact listés
en §5.4.

---

## 2. Contraintes qui ont façonné le design

**Les 16 entrées analogiques du Mega sont prises.** A0–A15 sont utilisées par les 16
potentiomètres de façade. Toute entrée supplémentaire passe obligatoirement par le
multiplexeur **CD74HC4067 (U2)** déjà présent depuis la v0.3, dont la sortie commune est
câblée sur **A15** et les 4 lignes de sélection sur **D2/D3/D4/D13**. Le pot #16, autrefois
sur A15 en direct, est déjà déplacé sur le canal 0 du mux.

**Un seul câble vers la guitare.** Le jack 6,35 stéréo actuel (J10) n'offre que 3 conducteurs,
tous consommés par la molette (curseur, +5 V, masse). Le switch en exige un quatrième.

**Le connecteur portera du +5 V.** Deux embases DIN 5 broches (MIDI IN / MIDI OUT) sont déjà
sur la carte. Un troisième DIN 5 non-MIDI transportant du 5 V créerait un risque réel : un
câble MIDI branché par erreur injecterait du 5 V dans la sortie MIDI d'un autre appareil. Le
connecteur retenu doit être **physiquement incompatible** avec du MIDI.

**Le PCB n'est pas encore commandé.** La v1.1 peut donc modifier le connecteur proprement,
sans fil volant ni contournement.

---

## 3. Matériel — shield v1.1

### 3.1 Modifications du PCB

| Réf | v1.0 | v1.1 |
|---|---|---|
| **J10** | Jack 6,35 stéréo Neutrik NRJ6HF-1 | **Mini-XLR 4 points Neutrik NC4MD-L-B-1** |
| **R31** | — | 470 kΩ, pull-down de présence sur le canal switch |
| **R32** | — | 1 kΩ, filtre RC série du canal switch |
| **C20** | — | 100 nF, filtre RC du canal switch vers GND |

Inchangé : mux 4067 et ses lignes de sélection, COM sur A15, les 16 pots et leurs filtres RC,
MIDI IN/OUT, OLED, alimentation, les 7 boutons et 7 LEDs.

### 3.2 Brochage du mini-XLR (J10)

| Broche | Signal | Destination sur la carte |
|---|---|---|
| 1 | GND | masse commune |
| 2 | +5 V | alimentation molette + échelle de résistances |
| 3 | curseur molette | filtre RC existant du jack v0.3 → **I1** du 4067 — **canal 1** |
| 4 | tension switch | filtre RC (R32 / C20) + pull-down R31 → **I2** du 4067 — **canal 2** |

Le filtre RC du canal 1 existe déjà en v1.0 (il servait au jack « pot déporté ») ; seules ses
références seront renumérotées si le re-routage l'impose.

### 3.3 Câblage dans la guitare

Une molette 10 kΩ à ressort de rappel, un switch **3 positions ON-ON-ON**, et **quatre
résistances de 2,2 kΩ** en série entre +5 V et GND. Le switch sélectionne l'un des trois points
intermédiaires du diviseur.

L'échelle vit dans la guitare, pas sur le PCB : c'est ce qui permet de tenir en 4 conducteurs.
Une échelle côté carte imposerait 3 fils rien que pour le switch.

Consommation de l'échelle : 5 V / 8,8 kΩ ≈ **568 µA**, négligeable.

| Position | Point du diviseur | Tension | Impédance de source (Thévenin) |
|---|---|---|---|
| 1 | ¼ Vcc | 1,25 V | 1,65 kΩ |
| 2 | ½ Vcc | 2,50 V | 2,20 kΩ |
| 3 | ¾ Vcc | 3,75 V | 1,65 kΩ |

Le pull-down R31 de 470 kΩ face à une impédance de source de 2,2 kΩ au pire introduit une
erreur de **12 mV**, sans effet sur la détection (les fenêtres font 0,4 V de large).

### 3.4 Détection de présence — pourquoi ¼ / ½ / ¾ et pas 0 / ½ / 1

Sans précaution, une entrée de mux en l'air prend une valeur aléatoire : **pitch bend fantôme**
dès que le câble n'est pas branché.

Aucune position valide ne produit 0 V. Câble débranché, R31 tire le canal à ~0 V, valeur qui
n'appartient à **aucune** fenêtre. Le canal du switch sert donc à la fois de sélecteur de plage
**et** de détection de présence, pour le prix d'une résistance.

### 3.5 Fenêtres de décision (ADC 10 bits, Vref = 5 V)

| Valeur ADC | Interprétation |
|---|---|
| 0 – 100 | **débranché** |
| 176 – 336 | position 1 — ±1 ton |
| 432 – 592 | position 2 — ±1 ton et demi |
| 688 – 848 | position 3 — ±1 octave |
| tout le reste | **invalide** (switch en cours de bascule) |

Valeurs nominales : 256, 512, 768. Marge de ±80 pas ADC, soit ±0,39 V. Des résistances à 5 %
déplacent les taps d'environ ±26 pas — largement dans la marge. Aucune fenêtre ne se chevauche.

### 3.6 Contrainte mécanique côté guitare

L'embase est encastrée dans le **pickguard** de la Stratocaster : la fiche mini-XLR au bout du
câble doit être **coudée**, sinon elle bute contre le corps de l'instrument. Référence exacte à
choisir au moment de la BOM.

---

## 4. Ce qui est repris du test validé en production

Le sketch `~/Desktop/PitchWheel_Test_Z3.ino`, éprouvé en conditions réelles sur le Z3, fournit
la logique. Elle est reprise **sans modification de comportement** :

- lissage IIR `filtre = (filtre * 3 + brut) / 4`
- zone morte à hystérésis : entrée à 40, sortie à 55
- stabilisation au centre : `PITCH_CENTER_SETTLE_MS = 80`
- cadence d'émission : 20 ms (~50 Hz)
- seuil de réémission : `PITCH_MIN_DELTA = 8`
- auto-calibration du centre sur 16 lectures
- restriction de course : `8192 ± BEND_SEMITONES × 8192 / 12`

**Ce qui est abandonné :** la LED témoin sur D12, qui servait aux essais. D12 reste affectée à
`LED_PREV` dans le sketch principal.

**Ce qui n'est pas repris :** `KorgZ3_PitchWheel(1).ino` et `KorgZ3_PitchWheel.ino` lisent la
molette sur `A16`, broche **inexistante sur un Mega 2560** (le core AVR ne définit que A0–A15,
et aucun `#define A16` n'apparaît dans ces fichiers). Ces deux brouillons n'ont jamais été
compilés. Ils n'entrent pas dans le dépôt.

---

## 5. Firmware

### 5.1 Structure

Le sketch principal fait déjà 981 lignes (presets, EEPROM, OLED, SysEx, 4 boutons). La molette
part donc dans deux modules séparés, en onglets de l'IDE Arduino :

```
KorgZ3_SysEx_08-05-2026/
├── KorgZ3_SysEx_08-05-2026.ino   ← seul .ino ; le dossier reste UN programme
├── mux.h / mux.cpp
└── pitch.h / pitch.cpp
```

Un seul `.ino` par dossier : deux fichiers `.ino` seraient concaténés par l'IDE, ce qui
brouillerait les frontières que l'on cherche justement à poser. Un seul téléversement,
inchangé pour l'utilisateur.

### 5.2 Module `mux`

Responsabilité unique : lire un canal du 4067.

```cpp
void muxBegin();              // D2/D3/D4/D13 en OUTPUT
int  muxRead(byte channel);   // canal 0..15 -> 0..1023
```

`muxRead()` positionne les 4 lignes de sélection, attend 10 µs, effectue une **lecture jetée**
puis la lecture utile. La lecture jetée laisse le condensateur d'échantillonnage de l'ADC se
recharger après le changement de canal ; sans elle la mesure traîne une trace du canal
précédent. Coût ≈ 100 µs, et le mux n'est lu que 3 fois par tour de boucle (canaux 0, 1, 2).

### 5.3 Module `pitch`

```cpp
void pitchBegin();
void pitchUpdate();           // appelé à chaque tour de loop()
int  pitchBendSemitones();    // 2, 3 ou 12 — pour l'affichage
bool pitchConnected();
```

Tout l'état est privé au module : centre calibré, valeur filtrée, dernière valeur émise,
drapeau « au centre », plage courante, présence du câble, chronos.

**Calibration à chaud.** Tant que le câble est absent, aucune calibration n'est possible. Dès
que le canal switch entre dans une fenêtre valide et s'y maintient, le module calibre le centre
sur 16 lectures du canal 1, puis passe en service. Débrancher/rebrancher recalibre. L'appareil
n'a plus besoin d'être allumé molette au repos.

**Anti-glissement du switch.** Sur un ON-ON-ON, le contact s'ouvre brièvement pendant la
bascule : le fil part en l'air, R31 le tire vers 0 V, ce qui ressemble à un débranchement. Tout
changement — position **comme** débranchement — est donc confirmé sur **300 ms de lectures
cohérentes** avant d'être appliqué. Le transitoire est absorbé.

**Changement de plage.** À position physique égale, la valeur MIDI dépend de la plage. Un
changement de plage **force une réémission immédiate**, sans quoi le Z3 resterait sur
l'ancienne valeur jusqu'au prochain mouvement de molette.

**Débranchement confirmé.** Le module émet **une fois** `8192` puis se tait, pour ne pas laisser
le Z3 désaccordé sur le dernier bend. `pitchUpdate()` devient un no-op tant que rien n'est
rebranché.

**Valeurs de butée attendues** (à vérifier par test unitaire) :

| Plage | `PITCH_MIDI_MIN` | Centre | `PITCH_MIDI_MAX` |
|---|---|---|---|
| ±1 ton (2) | 6827 | 8192 | 9557 |
| ±1 ton et demi (3) | 6144 | 8192 | 10240 |
| ±1 octave (12) | 0 | 8192 | 16384 → **écrêté à 16383** |

En ±1 octave, `8192 + 8192 × 12 / 12 = 16384` dépasse d'une unité le maximum 14 bits. L'écrêtage
final dans `sendPitchWheel()` le ramène à 16383 ; le test unitaire doit le confirmer plutôt que
de le supposer.

### 5.4 Points de contact avec le sketch principal

| Endroit | Changement |
|---|---|
| `setup()` | `muxBegin(); pitchBegin();` |
| `loop()` | `pitchUpdate();` en tête, **avant** le throttle SysEx |
| `updateFiltered()` (l. 348) et `initFilter()` (l. 379) | si `pot.pin == A15` → `muxRead(0)` au lieu de `analogRead(A15)` |
| affichage | message transitoire ~1,5 s au changement de plage et au branchement |

Le troisième point est **obligatoire, pas cosmétique** : A15 est la sortie commune du mux. Un
`analogRead(A15)` sans avoir positionné les lignes de sélection lirait un canal indéterminé. Le
pot #16 doit passer par `muxRead(0)`.

### 5.5 Limitation acceptée — pitch suspendu pendant un dump SysEx

`sendSysEx()` (l. 399) émet les 96 octets avec un `delay(2)` entre chaque, bloquant la boucle
**~192 ms** à chaque mouvement de potentiomètre. La molette n'est ni lue ni émise pendant ce
temps.

Il n'existe pas de contournement propre : la norme MIDI interdit d'insérer un pitch bend à
l'intérieur d'un SysEx en cours (seuls les messages temps réel y sont admis) ; l'y glisser
corromprait le dump aux yeux du Z3.

**Décision : accepter.** En jeu réel, on tourne un potentiomètre *ou* on bende, rarement les
deux dans le même dixième de seconde ; le filtre IIR rattrape la position à la reprise sans
saut audible. Deux pistes restent en réserve, à évaluer **une fois la carte en main** :

1. réduire le `delay(2)` — les 96 octets ne prennent que 31 ms à 31250 bauds, les 192 ms
   ajoutées sont une marge de sécurité pour le Z3 ; la réduire sans pouvoir tester risquerait
   de casser un envoi qui fonctionne ;
2. réécrire l'envoi en machine à états non bloquante — propre, mais touche au cœur de ce qui
   marche, pour un bénéfice non mesurable sans matériel.

---

## 6. Vérification

Ce firmware est écrit **sans carte pour le tester**. Les brouillons sur `A16` montrent le risque :
un fichier d'apparence correcte, jamais compilé, qui donne l'illusion d'exister.

### 6.1 Compilation réelle pour le Mega

`arduino-cli compile --fqbn arduino:avr:mega`. Prouve que le programme existe, attrape toute
erreur de la classe `A16`, et rapporte l'occupation flash et RAM. Le Mega 2560 n'a que 8 Ko de
RAM et le sketch y garde déjà 8 slots de dump de 108 octets : la marge doit être mesurée avant
d'ajouter un module, pas après.

**Mesure de référence du 2026-07-25** (sketch actuel, avant modification) :

| Ressource | Utilisé | Total | Marge |
|---|---|---|---|
| Flash | 25 660 o (10 %) | 253 952 o | très large |
| RAM statique | 4 454 o (54 %) | 8 192 o | 3 738 o pour pile et locales |

À retrancher de ces 3 738 o : le tampon d'affichage alloué au tas par `Adafruit_SSD1306` à
l'appel de `display.begin()`, soit 128 × 32 / 8 = **512 o**. Reste ≈ 3 200 o réellement libres à
l'exécution. Les deux modules ajoutent quelques dizaines d'octets d'état : la marge est
confortable, mais le chiffre est à re-mesurer après intégration.

**Prérequis de l'environnement de compilation** (macOS Apple Silicon) : Arduino ne publie pas de
toolchain AVR native ARM ; `avr-g++` est un binaire x86_64 et exige **Rosetta 2**
(`softwareupdate --install-rosetta`). Les bibliothèques `Adafruit SSD1306` et `Adafruit GFX`
sont absentes du sketchbook local et ont dû être installées via `arduino-cli lib install` — le
sketch n'a donc jamais été compilé sur ce Mac.

**Vérification du 2026-07-25 :** `KorgZ3_PitchWheel(1).ino` compilé pour `arduino:avr:mega`
échoue sur `error: 'A16' was not declared in this scope`, confirmant §4.

### 6.2 Tests unitaires de la logique, sur le Mac

Les calculs ne dépendent pas de l'Arduino. Deux fonctions pures sont isolées et compilées avec
`g++` :

```cpp
int  pitchValue14(int filtered, int center, int deadzoneOut, int semitones);
byte switchWindow(int raw);   // 0 = débranché, 1..3 = position, 255 = invalide
```

Ce qu'on prouve ainsi :

- les butées valent bien celles du tableau §5.3, notamment 6827 / 9557 en ±1 ton
- le centre renvoie exactement 8192, sans dérive d'arrondi
- aucune sortie hors de `[0..16383]`, y compris avec un centre calibré très décentré
- les fenêtres de §3.5 ne se chevauchent pas et la zone « débranché » n'en mord aucune

La partie chronométrée (hystérésis, confirmation 300 ms) se teste en injectant un temps simulé
plutôt qu'en appelant `millis()`.

### 6.3 Vérification croisée firmware ↔ KiCad

`shield_redesign/check_fw.py` est étendu pour relire le netlist v1.1 et confirmer que le
firmware est d'accord avec le cuivre : sélection sur D2/D3/D4/D13, COM sur A15, molette sur le
canal 1, switch sur le canal 2. C'est la classe d'erreur la plus coûteuse à découvrir une fois
la carte soudée.

Le script pointe aujourd'hui sur `hardware/firmware_CCSysEx_Patcher.ino` ; il est repointé sur
`KorgZ3_SysEx_08-05-2026.ino` (voir §7).

### 6.4 Contrôles KiCad du PCB v1.1

ERC sur le schéma, DRC sur le PCB, plus les scripts `check_*` existants (placement, nets,
en-têtes, template Mega) — le passage qui a validé la v1.0, rejoué après remplacement de J10 et
re-routage.

### 6.5 Angles morts assumés

Trois points ne se sauront qu'à la première mise sous tension :

- le comportement réel de l'ADC derrière le mux et les filtres RC (bruit, diaphonie)
- la valeur brute au repos de la molette, donc le réglage fin des zones mortes 40/55
- la tolérance du Z3 à recevoir nos messages autour des envois de dump

Première étape à réception : un **montage de test** — platine avec pot 10 kΩ, switch 3 positions
et les 4 résistances, câblée sur une fiche mini-XLR, soit la guitare simulée. On valide la
chaîne électrique avant de toucher au pickguard.

---

## 7. Rangement du dépôt

- **`hardware/firmware_CCSysEx_Patcher.ino` supprimé.** C'est le firmware de baritonomarchetto,
  sans rapport avec le Z3 ; sa présence à côté du sketch réel entretient la confusion sur
  « quel programme téléverser ».
- **`hardware/firmware_LICENSE.txt` supprimé** avec lui (licence MIT du fichier retiré).
- **`hardware/ATTRIBUTION.md` mis à jour** : la mention du firmware d'origine devient un lien
  vers le dépôt de baritonomarchetto. L'attribution du **matériel** (CC BY-NC-SA) reste
  intacte — c'est elle qui nous engage juridiquement, la carte étant une œuvre dérivée.
- **`shield_redesign/check_fw.py` repointé** sur `KorgZ3_SysEx_08-05-2026.ino`, sinon le script
  casse à la suppression ci-dessus.
- Les brouillons `KorgZ3_PitchWheel.ino` et `KorgZ3_PitchWheel(1).ino` **n'entrent pas** dans le
  dépôt (voir §4).
- Branche `shield-mega-redesign` **renommée `pitchwheel-integration`**, local et distant.

---

## 8. Décisions et leurs raisons

| Décision | Raison |
|---|---|
| Mini-XLR 4 points plutôt que DIN 5 | incompatible physiquement avec le MIDI ; évite d'injecter du +5 V dans un appareil tiers |
| Switch lu en analogique plutôt qu'en numérique | 1 conducteur au lieu de 2 ; pas de ligne logique le long d'un câble de guitare |
| Échelle de résistances dans la guitare | seule façon de tenir en 4 conducteurs |
| Taps à ¼ / ½ / ¾ Vcc | libère 0 V comme signature « débranché » — détection de présence gratuite |
| Confirmation sur 300 ms | absorbe l'ouverture de contact pendant la bascule du ON-ON-ON |
| Modules `.h`/`.cpp` plutôt qu'un fichier unique | code écrit à l'aveugle : des frontières nettes se relisent et se corrigent mieux qu'un bloc noyé dans 981 lignes |
| Pitch suspendu pendant un dump | contournement interdit par la norme MIDI ; gêne réelle non mesurable sans matériel |
