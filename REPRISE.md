# Reprise — Korg Z3 SysEx

État au **13/08/2026**. Ce fichier est écrit pour être lu **seul**, sans le fil de
discussion qui l'a produit. Il dit où en est le projet, ce qui reste à faire, et
les pièges qui ont déjà coûté du temps.

---

## 1. Où en est le dépôt

- Branche **`pitchwheel-integration`**, **poussée** sur `github.com:GAUDVIBE/KorgZ3_SysEx`.
- **Non fusionnée dans `main`** — c'était déjà le cas avant, rien d'anormal.
- Arbre de travail propre, rien en attente.

Les quatre derniers commits :

| commit | contenu |
|---|---|
| `9103150` | **pcb v1.5** — brochage Mega corrigé, re-routé, Gerbers `hardware/fab_v15/` |
| `c28b07c` | **firmware v1.2** — compensation du brochage, latence molette corrigée, traces retirées |
| `c6d0e35` | **firmware v1.5** — le jumeau, pour la carte corrigée |
| `2fc26b5` | divers — M4L, `tools/diag_mux`, nomenclature, migration KiCad 10 du schéma |

**La carte physique en ta possession est une v1.2** et elle tourne le firmware
`KorgZ3_SysEx_26-07-2026/`, nettoyé de ses traces de debug.

---

## 2. Les deux cartes, les deux firmwares

### Carte v1.2 (fabriquée, entre tes mains)

Défaut de conception : **l'ordre des broches est inversé à l'intérieur de chacun
des 6 blocs d'embases Mega**. Les blocs sont aux bonnes positions — elle
s'enfiche — mais les nets sont affectés de la dernière broche vers la première.
Conséquences : masse du shield sur RESET et IOREF, +5 V sur la sortie 3V3, D10
sur AREF, D11 sur une masse. Table complète dans `hardware/CORRESPONDANCE_BROCHES.md`.

Réparation matérielle déjà faite : broches mâles 2, 3 et 4 de JMP1 retirées,
remplacées par deux cavaliers 3→6 (masse) et 4→5 (+5 V).

Le firmware `KorgZ3_SysEx_26-07-2026/` compense tout ça dans `board_map.h`.

### Carte v1.5 (corrigée, **pas encore fabriquée**)

`shield_redesign/fix_mega_pinorder.py` a réaffecté les **50 pastilles** à la
broche qu'elles touchent réellement, **sans en déplacer aucune**. Re-routage
complet : **0 connexion manquante, 0 erreur DRC**. Gerbers prêts dans
`hardware/fab_v15/`.

Le firmware `KorgZ3_SysEx_v15/` lui correspond : `board_map.h` y est l'identité,
les 5 boutons et 5 LEDs de page sont rendus, le Dump Request aussi, et le MIDI IN
refonctionne (les nets `D0_RX`/`D1_TX` retombent sur l'UART0 matériel).

> ⚠️ **Ne jamais téléverser le firmware v1.5 sur une carte v1.2** : la page V
> mettrait le plan de masse en sortie à l'état haut et détruirait la broche.
> Les en-têtes des deux croquis se signalent mutuellement.

---

## 3. À FAIRE, par ordre d'importance

### 🔴 1. Trancher les broches 4/5 des embases DIN — bloquant avant toute commande

Sur `J1` (MIDI_IN) et `J2` (MIDI_OUT), les pastilles se lisent en x croissant
`1 5 2 4 3`. Un DIN-5 idéal impose `1 4 2 5 3`.

**Mais cette comparaison ne prouve rien** : l'empreinte ne suit pas un demi-cercle
de 7 mm, elle suit le plan **Kycon KCDX-5S-N** (rangée 3-2-1 à 14,60 mm, rangée
5-4 à 10,00 mm — validé dans `hardware/A_COMMANDER.md`). J'ai d'abord affirmé la
transposition en me fondant sur le DIN idéal ; c'était mal fondé et je l'ai
retiré. La question reste **ouverte**.

Ce qui trancherait, du plus rapide au plus lent :

1. **Ohmmètre**, deux minutes : embase Kycon en main, continuité entre la broche 4
   du connecteur et le trou intérieur **gauche** de l'empreinte (local x = −5).
   Si ça passe, l'empreinte est bonne, il n'y a rien à corriger.
2. **Plan Kycon** `Pub_Eng_Draw/KCDX-5S-N.pdf`, section *Recommended PCB Layout* :
   il numérote les trous.
3. Ce qui a été constaté au fer à souder le 10/08 au soir.

**Élément qui va CONTRE la transposition** : `midi_port.h` notait « MIDI OUT
fonctionne » le 10/08 à 16:10, donc avant tout croisement de fils — or le MIDI OUT
passe précisément par les broches 4 et 5.

Si correction il y a : permuter deux nets par embase, puis re-router (recette § 5).

### 2. `sendSysEx()` bloque 223 ms à chaque mouvement de potentiomètre

Même classe de bug que la latence déjà corrigée. Dans
`KorgZ3_SysEx_26-07-2026.ino` :

```c
for (int i = 0; i < sendBufferSize; i++) { MIDI_PORT.write(sendBuffer[i]); delay(2); }
```

95 octets × (2 ms + ~0,32 ms de SoftwareSerial) ≈ **223 ms**, plus le
rafraîchissement d'écran qui suit. Le correctif est immédiat : appeler
`ecranPompe()` dans cette boucle au lieu du `delay(2)` sec — la pompe existe déjà
et sert la molette. Non fait parce que tu joues à la molette seule, donc tu ne
l'entends pas.

### 3. Relire le schéma après sa migration KiCad 10

`SysEx_Patcher.kicad_sch` a été réécrit du format `20231120` (généré par
l'outillage maison) au format KiCad 10 lors d'une ouverture précédente. Écart de
**21 694 lignes**, **préexistant** à la session qui l'a commité, **jamais relu**.
Ouvrir le schéma et vérifier avant de s'y fier.

### 4. Piège théorique du recentrage automatique (`pitch.cpp`)

```c
if (abs(filteredRaw - refStable) > 3) { refStable = filteredRaw; stableSince = millis(); }
else if (millis() - stableSince >= 4000 && abs(refStable - centerRaw) > DEADZONE_OUT) {
    centerRaw = refStable;   // adopte la position tenue comme nouveau centre
}
```

Le commentaire du code juge « improbable » de tenir un bend figé plusieurs
secondes. **En butée mécanique, c'est automatique** : la molette est bloquée, la
lecture ne bouge plus d'un LSB. Tenir un bend à fond 4 s ferait donc adopter la
butée comme centre. **Jamais observé en capture**, mais la justification écrite
est fausse. À vérifier à la main : tenir un bend à fond 5 s et regarder si le
pitch retombe seul.

---

## 4. La latence de la molette — ce qui a été fait et pourquoi

**Cause racine :** `loop()` est coopérative et `pitchUpdate()` n'est appelée
qu'une fois par tour. **Tout traitement long est donc un trou dans la molette**,
qui attend d'être servie toutes les 12 ms. Le défilement automatique de la rangée
de paramètres (`SCROLL_INTERVAL = 3000`) déclenchait `updateDisplay()`, qui
bloquait **329 ms**.

Tout ci-dessous est **mesuré sur la carte**, pas estimé :

| étape | pire famine de la molette |
|---|---|
| départ | **330 ms** |
| pilotage I2C par accès direct `DDRH`/`PORTH` (le bus tournait à 16 kHz) | 131 ms |
| transfert découpé en 8 tranches + pompe entre chacune | 55 ms |
| pompe pendant le dessin aussi (43 ms non pompés) | **20 ms** |

Décomposition initiale des 329 ms : **288 ms de transfert I2C, 41 ms de dessin**.
Le bus logiciel tournait à 16 kHz (62 µs/bit) parce qu'U8g2 appelle `pinMode()`
**puis** `digitalWrite()` à chaque front, deux fonctions lentes sur AVR.

Le découpage est légitime : **l'I2C est piloté par le maître**, rien n'interdit de
suspendre entre deux transactions.

Validé à l'oreille : « je n'entends plus rien ».

---

## 5. Recettes qui marchent

### Firmware

```bash
cd ~/Documents/GitHub/KorgZ3_SysEx
arduino-cli compile --fqbn arduino:avr:mega KorgZ3_SysEx_26-07-2026
arduino-cli upload -p /dev/cu.usbmodem1201 --fqbn arduino:avr:mega KorgZ3_SysEx_26-07-2026
```

Le port varie ; le retrouver avec `arduino-cli board list`.

### Capture série — trois pièges

```bash
( arduino-cli monitor -p /dev/cu.usbmodem1201 -c baudrate=115200 > /tmp/cap.log 2>&1 ) & M=$!
sleep 18; kill $M
grep -a "MOTIF" /tmp/cap.log
```

1. **Ouvrir `/dev/cu.*` ne redémarre PAS la carte.** Tout ce qui n'est imprimé que
   dans `setup()` est déjà parti : republier en boucle pour le capturer.
2. **`grep` a besoin de `-a`** — le log contient du binaire de démarrage, sinon
   grep le traite comme binaire et n'affiche rien (m'a fait croire trois fois que
   la carte était plantée).
3. **L'attache du moniteur rate la fenêtre une fois sur deux.** Prévoir 16–20 s et
   réessayer avant de conclure que le firmware est cassé.

> Le firmware v1.2 est désormais **silencieux** (0 octet en 14 s, vérifié). Pour
> retrouver des traces, en réintroduire ponctuellement. Le firmware v1.5, lui,
> garde les siennes derrière `debug_usb.h` : passer `DEBUG_USB` à 1 — **au prix du
> MIDI**, puisque Serial0 le porte sur cette carte.

### Carte

```bash
cd hardware/kicad_project
python3 shield_redesign/check_pinorder.py     # 50 pastilles, les 6 blocs
kicad-cli pcb drc --format json --output /tmp/drc.json --severity-error --severity-warning SysEx_Patcher.kicad_pcb
```

Re-routage complet :

```bash
KPY=/Applications/KiCad/KiCad.app/Contents/Frameworks/Python.framework/Versions/Current/bin/python3
python3 shield_redesign/ripup_routing.py
$KPY shield_redesign/export_dsn.py SysEx_Patcher.kicad_pcb /tmp/b.dsn
/opt/homebrew/opt/openjdk/bin/java -Djava.awt.headless=true \
  -jar ~/tools/freerouting/freerouting-2.2.4.jar -de /tmp/b.dsn -do /tmp/b.ses -mp 100
$KPY shield_redesign/import_ses.py SysEx_Patcher.kicad_pcb /tmp/b.ses SysEx_Patcher.kicad_pcb
```

**Astuce qui évite l'effleurement d'isolation** : router avec la règle à
**0,25 mm** (dans `SysEx_Patcher.kicad_pro`, classe `Default`) puis **la remettre
à 0,20** avant le DRC. Freerouting laisse sinon une violation à 0,1994 mm.

Gerbers :

```bash
kicad-cli pcb export gerbers --output ../fab_v15/ \
  --layers F.Cu,In1.Cu,In2.Cu,B.Cu,F.Mask,B.Mask,F.Silkscreen,B.Silkscreen,Edge.Cuts \
  SysEx_Patcher.kicad_pcb
kicad-cli pcb export drill --output ../fab_v15/ --format excellon --excellon-separate-th SysEx_Patcher.kicad_pcb
```

Sans `--layers`, l'export sort le mauvais jeu de couches.

---

## 6. Pièges qui ont déjà coûté du temps

**Un test peut partager la croyance fausse du code qu'il vérifie.**
`check_pinorder.py` attendait l'ordre analogique inversé — écrit à partir de la
même erreur que le générateur, il passait au vert sur une carte dont la masse
atterrissait sur RESET. Le brochage réel vit maintenant dans **un seul endroit**,
`shield_redesign/mega_pinout.py`, importé à la fois par le correctif et par le
test. Vérifier qu'un test **échoue** sur le cas défectueux, pas seulement qu'il
passe sur le cas sain.

**`constexpr` n'est pas une macro.** `#if OLED_SCL_PIN == 8` est évalué par le
préprocesseur, qui remplace l'identifiant inconnu par **0** et désactive le code
**en silence, sans warning**. Seule la mesure l'a rattrapé. Utiliser un
`constexpr bool` évalué par le compilateur.

**Ne jamais calculer une position de pastille à la main.** Les transformations de
rotation se sont trompées deux fois (signe de y inversé → composants en miroir →
faux diagnostics de collision). Prendre les positions via l'API `pcbnew` et
laisser le DRC arbitrer.

**Ne JAMAIS « Update PCB from schematic ».** Le PCB porte 11 empreintes absentes
du schéma (`JMP1`, `JMA1/2`, `JMD1/2/3`, `JMX1`, `MH1`-`MH4`). Une synchro les
supprimerait et détruirait le shield. Ajouter des composants au PCB en injectant
du s-expr, comme le fait `shield_redesign/apply_controls.py`.

**Mesurer avant d'optimiser.** Mon estimation du coût de l'écran était fausse d'un
facteur 7 (30–50 ms annoncés, 329 ms réels), et j'ai ensuite annoncé « 45 ms après
correction » pour un résultat de 90 ms. Les deux fois, c'est la mesure sur la
carte qui a corrigé le tir.

---

## 7. Références

- `hardware/CORRESPONDANCE_BROCHES.md` — table shield ↔ Mega, défaut v1.2 et sa réparation
- `hardware/A_COMMANDER.md` — liste d'achat, preuve du choix Kycon `KCDX-5S-N`
- `hardware/COTES_BOITIER.md` — cotes pour le boîtier
- `hardware/BOM.md` — nomenclature
- `docs/superpowers/{specs,plans}/` — specs et plans des chantiers précédents
