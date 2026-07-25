# Molette de pitch + switch de bend range — plan d'implémentation

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Ajouter au contrôleur Korg Z3 une molette de pitch et un switch 3 positions de bend range, montés sur la guitare et reliés au shield par un unique câble de 4 conducteurs.

**Architecture:** La molette et le switch sont lus par le multiplexeur CD74HC4067 déjà présent (canaux 1 et 2, sortie commune sur A15). Le switch est encodé en analogique par une échelle de résistances dont les taps à ¼/½/¾ Vcc libèrent 0 V comme signature « débranché ». Le firmware isole la logique dans trois fichiers — `pitch_math.h` (maths pures, testables sur Mac), `mux.*` (accès au 4067), `pitch.*` (machine à états) — le sketch principal ne gardant que quatre points de contact.

**Tech Stack:** Arduino AVR (Mega 2560), `arduino-cli` 1.5.1, KiCad 10.0.3 (`kicad-cli`), Python 3 pour l'outillage s-expr, `g++` pour les tests unitaires natifs.

**Spec:** `docs/superpowers/specs/2026-07-25-pitchwheel-bendrange-design.md`

---

## Prérequis de l'environnement

Déjà installés et vérifiés le 2026-07-25 :

- `arduino-cli` 1.5.1, core `arduino:avr` 1.8.8, Rosetta 2 (le toolchain AVR est x86_64 seul)
- bibliothèques `Adafruit SSD1306` 2.5.x et `Adafruit GFX` 1.12.6 (installées via `arduino-cli lib install`)
- `kicad-cli` 10.0.3 dans `/opt/homebrew/bin/`
- `directories.user` d'arduino-cli pointé sur `/Users/gaudry/Documents/Arduino`

Mesure de référence à conserver comme point de comparaison : **flash 25 660 o (10 %)**, **RAM statique 4 454 o (54 %)**.

Toutes les commandes s'exécutent depuis la racine du dépôt `/Users/gaudry/Documents/KorgZ3_SysEx`, sauf mention contraire.

---

## Structure des fichiers

| Fichier | Responsabilité |
|---|---|
| `KorgZ3_SysEx_08-05-2026/pitch_math.h` | **Créé.** Maths pures : fenêtres du switch, conversion position → valeur 14 bits. Aucune dépendance Arduino, donc compilable et testable avec `g++`. |
| `KorgZ3_SysEx_08-05-2026/mux.h` / `mux.cpp` | **Créés.** Accès au CD74HC4067 : sélection de canal et lecture. |
| `KorgZ3_SysEx_08-05-2026/pitch.h` / `pitch.cpp` | **Créés.** Machine à états : calibration, hystérésis, confirmation du switch, émission MIDI, événements pour l'affichage. |
| `KorgZ3_SysEx_08-05-2026/KorgZ3_SysEx_08-05-2026.ino` | **Modifié.** Quatre points de contact (§5.4 de la spec) + déclaration en réserve des 3 boutons/3 LEDs. |
| `tests/test_pitch_math.cpp` | **Créé.** Tests unitaires natifs de `pitch_math.h`. Placé **hors** du dossier de sketch : tout `.cpp` qui s'y trouverait serait compilé par l'IDE Arduino. |
| `tests/run.sh` | **Créé.** Compile et exécute les tests. |
| `hardware/kicad_project/SysEx_Patcher.kicad_sch` | **Modifié.** J10 en `Conn_01x05`, ajout R31/R32/C20, câblage du canal 2. |
| `hardware/kicad_project/SysEx_Patcher.kicad_pcb` | **Modifié.** Empreinte mini-XLR, placement des 3 composants, re-routage. |
| `hardware/kicad_project/shield_redesign/check_fw.py` | **Modifié.** Repointé sur le vrai sketch + vérifications du mux. |
| `hardware/kicad_project/shield_redesign/check_sch_nets.py` | **Modifié.** Vérifie R31/R32/C20 et le nouveau J10. |
| `hardware/firmware_CCSysEx_Patcher.ino`, `hardware/firmware_LICENSE.txt` | **Supprimés.** |
| `hardware/ATTRIBUTION.md`, `hardware/RECONSTRUCTION_SCHEMA.md`, `README.md` | **Modifiés.** |

---

# Phase 1 — Maths pures et tests natifs

Cette phase ne touche à rien d'existant et est entièrement vérifiable sur le Mac, sans carte.

## Task 1 : Fenêtres de décision du switch

**Files:**
- Create: `tests/test_pitch_math.cpp`
- Create: `tests/run.sh`
- Create: `KorgZ3_SysEx_08-05-2026/pitch_math.h`

- [ ] **Step 1 : Écrire le test qui échoue**

Créer `tests/test_pitch_math.cpp` :

```cpp
// Tests natifs de pitch_math.h — aucune dépendance Arduino.
#include <cstdio>
#include <cstdlib>
#include "../KorgZ3_SysEx_08-05-2026/pitch_math.h"

static int failures = 0;

#define CHECK_EQ(actual, expected)                                            \
  do {                                                                        \
    long _a = (long)(actual), _e = (long)(expected);                          \
    if (_a != _e) {                                                           \
      printf("ECHEC %s:%d — %s vaut %ld, attendu %ld\n",                      \
             __FILE__, __LINE__, #actual, _a, _e);                            \
      failures++;                                                             \
    }                                                                         \
  } while (0)

static void test_switch_windows() {
  // Valeurs nominales des taps : 1/4, 1/2, 3/4 de 1023.
  CHECK_EQ(switchWindow(256), 1);
  CHECK_EQ(switchWindow(512), 2);
  CHECK_EQ(switchWindow(768), 3);

  // Rien branché : le pull-down tire à 0.
  CHECK_EQ(switchWindow(0), 0);
  CHECK_EQ(switchWindow(100), 0);

  // Bords de chaque fenêtre (marge de +/-80 pas).
  CHECK_EQ(switchWindow(176), 1);
  CHECK_EQ(switchWindow(336), 1);
  CHECK_EQ(switchWindow(432), 2);
  CHECK_EQ(switchWindow(592), 2);
  CHECK_EQ(switchWindow(688), 3);
  CHECK_EQ(switchWindow(848), 3);

  // Entre deux fenêtres : switch en cours de bascule.
  CHECK_EQ(switchWindow(101), PITCH_WINDOW_INVALID);
  CHECK_EQ(switchWindow(175), PITCH_WINDOW_INVALID);
  CHECK_EQ(switchWindow(337), PITCH_WINDOW_INVALID);
  CHECK_EQ(switchWindow(431), PITCH_WINDOW_INVALID);
  CHECK_EQ(switchWindow(593), PITCH_WINDOW_INVALID);
  CHECK_EQ(switchWindow(687), PITCH_WINDOW_INVALID);
  CHECK_EQ(switchWindow(849), PITCH_WINDOW_INVALID);
  CHECK_EQ(switchWindow(1023), PITCH_WINDOW_INVALID);

  // Aucun chevauchement : chaque valeur ADC appartient a au plus une fenetre.
  for (int raw = 0; raw <= 1023; raw++) {
    unsigned char w = switchWindow(raw);
    if (w != PITCH_WINDOW_INVALID && w > 3) {
      printf("ECHEC — switchWindow(%d) rend %u, hors 0..3\n", raw, w);
      failures++;
    }
  }
}

int main() {
  test_switch_windows();
  if (failures == 0) { printf("OK — tous les tests passent\n"); return 0; }
  printf("%d echec(s)\n", failures);
  return 1;
}
```

Créer `tests/run.sh` :

```bash
#!/bin/sh
# Compile et exécute les tests unitaires natifs de pitch_math.h.
set -e
cd "$(dirname "$0")/.."
g++ -std=c++11 -Wall -Wextra -o /tmp/test_pitch_math tests/test_pitch_math.cpp
/tmp/test_pitch_math
```

- [ ] **Step 2 : Rendre le script exécutable et vérifier que le test échoue**

Run :
```bash
chmod +x tests/run.sh && ./tests/run.sh
```
Expected : ÉCHEC de compilation, `fatal error: '../KorgZ3_SysEx_08-05-2026/pitch_math.h' file not found`.

- [ ] **Step 3 : Écrire l'implémentation minimale**

Créer `KorgZ3_SysEx_08-05-2026/pitch_math.h` :

```cpp
#ifndef PITCH_MATH_H
#define PITCH_MATH_H

// Maths pures de la molette de pitch.
// Aucune dépendance Arduino : ce fichier se compile aussi avec g++ sur un PC,
// ce qui permet de tester la logique sans carte (voir tests/test_pitch_math.cpp).

// Rendu par switchWindow() quand la tension lue n'appartient a aucune position
// valide — typiquement pendant la bascule du switch, contact ouvert.
const unsigned char PITCH_WINDOW_INVALID = 255;

// Position du switch de bend range d'apres la valeur ADC 10 bits du canal 2.
//   0 = rien branche (le pull-down tire la ligne a 0 V)
//   1 = +/-1 ton        2 = +/-1 ton et demi        3 = +/-1 octave
// Les taps nominaux valent 1/4, 1/2 et 3/4 de la pleine echelle, avec une
// marge de +/-80 pas : des resistances a 5 % les deplacent d'environ 26 pas.
inline unsigned char switchWindow(int raw) {
  if (raw >= 0   && raw <= 100) return 0;
  if (raw >= 176 && raw <= 336) return 1;
  if (raw >= 432 && raw <= 592) return 2;
  if (raw >= 688 && raw <= 848) return 3;
  return PITCH_WINDOW_INVALID;
}

#endif
```

- [ ] **Step 4 : Vérifier que le test passe**

Run :
```bash
./tests/run.sh
```
Expected : `OK — tous les tests passent`, code de sortie 0.

- [ ] **Step 5 : Commit**

```bash
git add tests/test_pitch_math.cpp tests/run.sh KorgZ3_SysEx_08-05-2026/pitch_math.h
git commit -m "pitch: fenêtres de décision du switch de bend range + tests natifs"
```

---

## Task 2 : Conversion position → valeur MIDI 14 bits

**Files:**
- Modify: `tests/test_pitch_math.cpp`
- Modify: `KorgZ3_SysEx_08-05-2026/pitch_math.h`

- [ ] **Step 1 : Écrire le test qui échoue**

Dans `tests/test_pitch_math.cpp`, ajouter cette fonction **avant** `main()` :

```cpp
static void test_pitch_value14() {
  const int CENTER = 512;
  const int DZ_OUT = 55;

  // Au centre : valeur MIDI centrale exacte, quelle que soit la plage.
  CHECK_EQ(pitchValue14(512, CENTER, DZ_OUT, 2,  true), 8192);
  CHECK_EQ(pitchValue14(0,   CENTER, DZ_OUT, 12, true), 8192);

  // +/-1 ton : offset = 8192 * 2 / 12 = 1365 -> butees 6827 et 9557.
  CHECK_EQ(pitchValue14(0,    CENTER, DZ_OUT, 2, false), 6827);
  CHECK_EQ(pitchValue14(1023, CENTER, DZ_OUT, 2, false), 9557);

  // +/-1 ton et demi : offset = 8192 * 3 / 12 = 2048 -> 6144 et 10240.
  CHECK_EQ(pitchValue14(0,    CENTER, DZ_OUT, 3, false), 6144);
  CHECK_EQ(pitchValue14(1023, CENTER, DZ_OUT, 3, false), 10240);

  // +/-1 octave : le calcul donne 16384, soit une unite au-dessus du maximum
  // 14 bits. L'ecretage doit ramener a 16383 — c'est la raison d'etre du test.
  CHECK_EQ(pitchValue14(0,    CENTER, DZ_OUT, 12, false), 0);
  CHECK_EQ(pitchValue14(1023, CENTER, DZ_OUT, 12, false), 16383);

  // Plage nulle (aucune position valide) : on reste au centre.
  CHECK_EQ(pitchValue14(1023, CENTER, DZ_OUT, 0, false), 8192);

  // Centre tres decentre : aucune sortie hors de [0..16383].
  for (int center = 20; center <= 1000; center += 20) {
    for (int raw = 0; raw <= 1023; raw += 7) {
      for (int st = 2; st <= 12; st += 5) {
        int v = pitchValue14(raw, center, DZ_OUT, st, false);
        if (v < 0 || v > 16383) {
          printf("ECHEC — pitchValue14(%d,%d,%d,%d) rend %d, hors bornes\n",
                 raw, center, DZ_OUT, st, v);
          failures++;
        }
      }
    }
  }
}
```

Puis remplacer `main()` par :

```cpp
int main() {
  test_switch_windows();
  test_pitch_value14();
  if (failures == 0) { printf("OK — tous les tests passent\n"); return 0; }
  printf("%d echec(s)\n", failures);
  return 1;
}
```

- [ ] **Step 2 : Vérifier que le test échoue**

Run :
```bash
./tests/run.sh
```
Expected : ÉCHEC de compilation, `error: use of undeclared identifier 'pitchValue14'`.

- [ ] **Step 3 : Écrire l'implémentation**

Dans `KorgZ3_SysEx_08-05-2026/pitch_math.h`, insérer **avant** le `#endif` :

```cpp
// Equivalent exact de la fonction map() d'Arduino (division entiere tronquee).
// Reimplementee ici pour que ce fichier reste compilable hors Arduino.
inline long pmMap(long x, long inMin, long inMax, long outMin, long outMax) {
  return (x - inMin) * (outMax - outMin) / (inMax - inMin) + outMin;
}

// Convertit la position filtree de la molette en valeur MIDI 14 bits.
//
// Le Korg Z3 mappe en dur [0..16383] sur +/-12 demi-tons. Pour obtenir une
// amplitude de +/-N demi-tons, on restreint la course emise a
// 8192 +/- N * 8192 / 12.
//
//   filtered    : valeur ADC lissee de la molette (0..1023)
//   center      : valeur ADC au repos, mesuree a la calibration
//   deadzoneOut : demi-largeur de la zone morte de sortie
//   semitones   : 2, 3 ou 12 ; 0 quand aucune position n'est valide
//   atCenter    : etat de la machine a hysteresis
inline int pitchValue14(int filtered, int center, int deadzoneOut,
                        int semitones, bool atCenter) {
  if (atCenter || semitones <= 0) return 8192;

  const long offset = (8192L * semitones) / 12;
  const long lo = 8192 - offset;
  const long hi = 8192 + offset;

  long v;
  if (filtered < center) {
    long inMax = (long)center - deadzoneOut - 1;
    // Centre si bas que la course descendante est nulle : butee basse.
    v = (inMax <= 0) ? lo : pmMap(filtered, 0, inMax, lo, 8191);
  } else {
    long inMin = (long)center + deadzoneOut + 1;
    // Centre si haut que la course montante est nulle : butee haute.
    v = (inMin >= 1023) ? hi : pmMap(filtered, inMin, 1023, 8193, hi);
  }

  if (v < lo) v = lo;
  if (v > hi) v = hi;
  // Ecretage 14 bits : en +/-1 octave, hi vaut 16384.
  if (v < 0) v = 0;
  if (v > 16383) v = 16383;
  return (int)v;
}
```

- [ ] **Step 4 : Vérifier que le test passe**

Run :
```bash
./tests/run.sh
```
Expected : `OK — tous les tests passent`, code de sortie 0.

- [ ] **Step 5 : Commit**

```bash
git add tests/test_pitch_math.cpp KorgZ3_SysEx_08-05-2026/pitch_math.h
git commit -m "pitch: conversion position -> valeur MIDI 14 bits avec plage restreinte"
```

---

# Phase 2 — Modules Arduino

## Task 3 : Module `mux`

**Files:**
- Create: `KorgZ3_SysEx_08-05-2026/mux.h`
- Create: `KorgZ3_SysEx_08-05-2026/mux.cpp`

- [ ] **Step 1 : Écrire l'en-tête**

Créer `KorgZ3_SysEx_08-05-2026/mux.h` :

```cpp
#ifndef MUX_H
#define MUX_H

#include <Arduino.h>

// Multiplexeur analogique CD74HC4067 (U2) du shield.
// Sortie commune COM -> A15 ; selection S0..S3 -> D2, D3, D4, D13 ;
// ~E force a la masse (toujours actif).
//
// Affectation des canaux :
//   0 = potentiometre #16 (autrefois cable en direct sur A15)
//   1 = curseur de la molette de pitch (broche 3 du mini-XLR)
//   2 = tension du switch de bend range (broche 4 du mini-XLR)
//   3..15 = libres, sortis sur le header J9

void muxBegin();
int  muxRead(byte channel);

#endif
```

- [ ] **Step 2 : Écrire l'implémentation**

Créer `KorgZ3_SysEx_08-05-2026/mux.cpp` :

```cpp
#include "mux.h"

static const byte MUX_SEL[4] = {2, 3, 4, 13};  // S0, S1, S2, S3
static const int  MUX_COM    = A15;            // sortie commune du 4067

void muxBegin() {
  for (byte i = 0; i < 4; i++) {
    pinMode(MUX_SEL[i], OUTPUT);
    digitalWrite(MUX_SEL[i], LOW);
  }
}

int muxRead(byte channel) {
  for (byte i = 0; i < 4; i++) {
    digitalWrite(MUX_SEL[i], (channel >> i) & 1);
  }
  delayMicroseconds(10);
  // Lecture jetee : laisse le condensateur d'echantillonnage de l'ADC se
  // recharger apres le changement de canal. Sans elle, la mesure traine une
  // trace du canal precedent.
  (void)analogRead(MUX_COM);
  return analogRead(MUX_COM);
}
```

- [ ] **Step 3 : Vérifier que le sketch compile toujours**

Run :
```bash
arduino-cli compile --fqbn arduino:avr:mega KorgZ3_SysEx_08-05-2026 2>&1 | tail -3
```
Expected : compilation réussie. Les chiffres bougent à peine — `mux.cpp` est compilé mais aucune fonction n'est encore appelée, l'éditeur de liens élimine le code mort.

- [ ] **Step 4 : Commit**

```bash
git add KorgZ3_SysEx_08-05-2026/mux.h KorgZ3_SysEx_08-05-2026/mux.cpp
git commit -m "mux: module d'accès au CD74HC4067 (canaux 0=pot16, 1=molette, 2=switch)"
```

---

## Task 4 : Module `pitch`

**Files:**
- Create: `KorgZ3_SysEx_08-05-2026/pitch.h`
- Create: `KorgZ3_SysEx_08-05-2026/pitch.cpp`

- [ ] **Step 1 : Écrire l'en-tête**

Créer `KorgZ3_SysEx_08-05-2026/pitch.h` :

```cpp
#ifndef PITCH_H
#define PITCH_H

#include <Arduino.h>

// Molette de pitch et switch de bend range, tous deux montes sur la guitare
// et relies au shield par le mini-XLR J10 (canaux 1 et 2 du multiplexeur).

enum PitchEvent {
  PITCH_EVENT_NONE = 0,
  PITCH_EVENT_CONNECTED,     // cable detecte : le centre vient d'etre calibre
  PITCH_EVENT_DISCONNECTED,  // cable retire : pitch fige au centre
  PITCH_EVENT_RANGE          // le switch a change de position
};

void pitchBegin();
void pitchUpdate();          // a appeler a chaque tour de loop()
int  pitchBendSemitones();   // 2, 3 ou 12 ; 0 si rien n'est branche
bool pitchConnected();

// Rend l'evenement en attente et le consomme. Destine a l'affichage.
PitchEvent pitchTakeEvent();

#endif
```

- [ ] **Step 2 : Écrire l'implémentation**

Créer `KorgZ3_SysEx_08-05-2026/pitch.cpp` :

```cpp
#include "pitch.h"
#include "mux.h"
#include "pitch_math.h"

static const byte CH_WHEEL  = 1;
static const byte CH_SWITCH = 2;

static const byte MIDI_CHANNEL = 0;   // 0 = canal MIDI 1, Basic Channel du Z3

// Reglages repris tels quels du test valide en production.
static const int DEADZONE_IN  = 40;
static const int DEADZONE_OUT = 55;
static const unsigned long CENTER_SETTLE_MS = 80;
static const unsigned long INTERVAL_MS      = 20;   // ~50 Hz
static const int MIN_DELTA = 8;

// Un ON-ON-ON ouvre brievement le contact pendant la bascule : la ligne part
// en l'air et le pull-down la tire vers 0 V, ce qui ressemble a un
// debranchement. On confirme donc tout changement sur cette duree.
static const unsigned long SWITCH_CONFIRM_MS = 300;
static const unsigned long SWITCH_POLL_MS    = 20;

static int  centerRaw   = 512;
static int  filteredRaw = 512;
static int  lastValue14 = 8192;
static bool atCenter    = true;
static unsigned long inZoneSince  = 0;
static unsigned long lastSendTime = 0;

static bool connected = false;
static byte position  = 0;            // 0 = rien branche, 1..3 = position
static byte pendingWindow = PITCH_WINDOW_INVALID;
static unsigned long pendingSince   = 0;
static unsigned long lastSwitchPoll = 0;
static bool forceSend = false;
static PitchEvent pendingEvent = PITCH_EVENT_NONE;

static void sendBend(int value14) {
  if (value14 < 0)     value14 = 0;
  if (value14 > 16383) value14 = 16383;
  Serial.write((byte)(0xE0 | (MIDI_CHANNEL & 0x0F)));
  Serial.write((byte)(value14 & 0x7F));
  Serial.write((byte)((value14 >> 7) & 0x7F));
}

// Mesure la position de repos de la molette. Appelee a chaque branchement,
// jamais au demarrage : tant que le cable est absent, il n'y a rien a mesurer.
static void calibrate() {
  long sum = 0;
  for (int i = 0; i < 16; i++) { sum += muxRead(CH_WHEEL); delay(2); }
  centerRaw   = (int)(sum / 16);
  filteredRaw = centerRaw;
  atCenter    = true;
  inZoneSince = 0;
  lastValue14 = 8192;
}

static void pollSwitch() {
  if (millis() - lastSwitchPoll < SWITCH_POLL_MS) return;
  lastSwitchPoll = millis();

  unsigned char w = switchWindow(muxRead(CH_SWITCH));

  // Entre deux fenetres : bascule en cours, on ne conclut rien.
  if (w == PITCH_WINDOW_INVALID) {
    pendingWindow = PITCH_WINDOW_INVALID;
    pendingSince  = 0;
    return;
  }

  byte current = connected ? position : 0;
  if (w == current) {
    pendingWindow = PITCH_WINDOW_INVALID;
    pendingSince  = 0;
    return;
  }

  if (w != pendingWindow) {
    pendingWindow = w;
    pendingSince  = millis();
    return;
  }
  if (millis() - pendingSince < SWITCH_CONFIRM_MS) return;

  // Changement confirme sur SWITCH_CONFIRM_MS de lectures coherentes.
  pendingWindow = PITCH_WINDOW_INVALID;
  pendingSince  = 0;

  if (w == 0) {
    // Debranchement : on recentre le Z3 une fois, puis on se tait.
    connected = false;
    position  = 0;
    sendBend(8192);
    lastValue14   = 8192;
    pendingEvent  = PITCH_EVENT_DISCONNECTED;
    return;
  }

  bool wasConnected = connected;
  position  = w;
  connected = true;
  if (!wasConnected) {
    calibrate();
    pendingEvent = PITCH_EVENT_CONNECTED;
  } else {
    // A position physique egale, la valeur MIDI depend de la plage : il faut
    // reemettre tout de suite, sinon le Z3 reste sur l'ancienne valeur.
    pendingEvent = PITCH_EVENT_RANGE;
  }
  forceSend = true;
}

void pitchBegin() {
  connected   = false;
  position    = 0;
  lastValue14 = 8192;
  atCenter    = true;
  forceSend   = false;
  pendingEvent   = PITCH_EVENT_NONE;
  pendingWindow  = PITCH_WINDOW_INVALID;
  pendingSince   = 0;
  lastSwitchPoll = 0;
  lastSendTime   = 0;
}

void pitchUpdate() {
  pollSwitch();
  if (!connected) return;

  if (millis() - lastSendTime < INTERVAL_MS) return;
  lastSendTime = millis();

  int raw = muxRead(CH_WHEEL);
  filteredRaw = (filteredRaw * 3 + raw) / 4;

  int distance = abs(filteredRaw - centerRaw);

  // Hysteresis : on entre au centre a DEADZONE_IN, on n'en sort qu'a
  // DEADZONE_OUT, et il faut y rester CENTER_SETTLE_MS pour figer la valeur.
  if (atCenter) {
    if (distance > DEADZONE_OUT) { atCenter = false; inZoneSince = 0; }
  } else {
    if (distance <= DEADZONE_IN) {
      if (inZoneSince == 0) inZoneSince = millis();
      if (millis() - inZoneSince >= CENTER_SETTLE_MS) atCenter = true;
    } else {
      inZoneSince = 0;
    }
  }

  int value14 = pitchValue14(filteredRaw, centerRaw, DEADZONE_OUT,
                             pitchBendSemitones(), atCenter);

  bool atCenterNow    = (value14 == 8192);
  bool wasAtCenter    = (lastValue14 == 8192);
  bool bigEnoughDelta = (abs(value14 - lastValue14) >= MIN_DELTA);

  if (forceSend || bigEnoughDelta || (atCenterNow && !wasAtCenter)) {
    sendBend(value14);
    lastValue14 = value14;
    forceSend   = false;
  }
}

int pitchBendSemitones() {
  switch (position) {
    case 1:  return 2;   // +/-1 ton
    case 2:  return 3;   // +/-1 ton et demi
    case 3:  return 12;  // +/-1 octave
    default: return 0;
  }
}

bool pitchConnected() { return connected; }

PitchEvent pitchTakeEvent() {
  PitchEvent e = pendingEvent;
  pendingEvent = PITCH_EVENT_NONE;
  return e;
}
```

- [ ] **Step 3 : Vérifier que le sketch compile**

Run :
```bash
arduino-cli compile --fqbn arduino:avr:mega KorgZ3_SysEx_08-05-2026 2>&1 | tail -3
```
Expected : compilation réussie, chiffres quasi inchangés (code non encore appelé).

- [ ] **Step 4 : Vérifier que les tests natifs passent toujours**

Run :
```bash
./tests/run.sh
```
Expected : `OK — tous les tests passent`.

- [ ] **Step 5 : Commit**

```bash
git add KorgZ3_SysEx_08-05-2026/pitch.h KorgZ3_SysEx_08-05-2026/pitch.cpp
git commit -m "pitch: machine à états — calibration à chaud, hystérésis, confirmation du switch"
```

---

## Task 5 : Intégration dans le sketch principal

**Files:**
- Modify: `KorgZ3_SysEx_08-05-2026/KorgZ3_SysEx_08-05-2026.ino`

- [ ] **Step 1 : Ajouter les inclusions et les broches en réserve**

Dans `KorgZ3_SysEx_08-05-2026.ino`, juste après la ligne `#include <EEPROM.h>` (l. 27, dernière des inclusions), ajouter :

```cpp
#include "mux.h"
#include "pitch.h"

// Boutons et LEDs supplementaires du shield v1.0, exposes par le bloc 2x18.
// Aucune fonction pour l'instant : les broches sont declarees pour que le
// materiel soit testable electriquement et reservees pour un usage futur.
const byte butLayout2[3] = {22, 24, 26};
const byte LEDLayout2[3] = {23, 25, 27};
```

- [ ] **Step 2 : Router le pot #16 par le multiplexeur**

Remplacer `updateFiltered()` (l. 348-355) par :

```cpp
void updateFiltered(PotConfig &pot) {
  // A15 est desormais la sortie commune du 4067 : le pot #16 s'y lit par le
  // canal 0. Un analogRead(A15) direct lirait un canal indetermine.
  int raw = (pot.pin == A15) ? muxRead(0) : analogRead(pot.pin);
  if (pot.maxVal >= 31) {
    pot.filteredRaw = (pot.filteredRaw * 7 + raw) / 8;
  } else {
    pot.filteredRaw = (pot.filteredRaw * 3 + raw) / 4;
  }
}
```

Remplacer `initFilter()` (l. 379-383) par :

```cpp
void initFilter(PotConfig &pot) {
  int sum = 0;
  for (int i = 0; i < 8; i++) {
    sum += (pot.pin == A15) ? muxRead(0) : analogRead(pot.pin);
    delay(1);
  }
  pot.filteredRaw = sum / 8;
}
```

- [ ] **Step 3 : Initialiser les modules dans `setup()`**

Dans `setup()`, juste après `delay(500);` (l. 693), ajouter. L'ordre importe : `initFilter()` est appelé plus bas (l. 757) sur les pots de la page 0, dont le pot #16 sur A15 — `muxBegin()` doit donc le précéder, sinon les lignes de sélection sont encore en entrée flottante au moment de la première lecture.

```cpp
  // Multiplexeur et molette : muxBegin() doit preceder tout initFilter(),
  // car le pot #16 se lit desormais par le canal 0.
  muxBegin();
  pitchBegin();

  // Boutons et LEDs en reserve du shield v1.0.
  for (byte i = 0; i < 3; i++) {
    pinMode(butLayout2[i], INPUT_PULLUP);
    pinMode(LEDLayout2[i], OUTPUT);
    digitalWrite(LEDLayout2[i], LOW);
  }
```

- [ ] **Step 4 : Appeler `pitchUpdate()` et afficher les événements dans `loop()`**

Dans `loop()`, juste après l'accolade ouvrante et **avant** `receiveMidi();`, ajouter :

```cpp
  // --- Molette de pitch : scannee a chaque tour, avant tout le reste ---
  pitchUpdate();

  switch (pitchTakeEvent()) {
    case PITCH_EVENT_CONNECTED:
      showMessage("Molette OK", "Centre calibre");
      delay(1000);
      updateDisplay();
      break;
    case PITCH_EVENT_DISCONNECTED:
      showMessage("Molette", "Debranchee");
      delay(1000);
      updateDisplay();
      break;
    case PITCH_EVENT_RANGE: {
      const char* label = "+/-1 octave";
      if (pitchBendSemitones() == 2) label = "+/-1 ton";
      else if (pitchBendSemitones() == 3) label = "+/-1 ton 1/2";
      showMessage("Bend range", label);
      delay(1000);
      updateDisplay();
      break;
    }
    default:
      break;
  }
```

- [ ] **Step 5 : Compiler et relever la consommation**

Run :
```bash
arduino-cli compile --fqbn arduino:avr:mega KorgZ3_SysEx_08-05-2026 2>&1 | tail -3
```
Expected : compilation réussie. Comparer aux chiffres de référence (flash 25 660 o, RAM statique 4 454 o). La RAM statique ne doit pas dépasser **4 700 o** ; au-delà, s'arrêter et signaler, car il faut aussi loger les 512 o du tampon SSD1306 alloués au tas.

- [ ] **Step 6 : Vérifier que les tests natifs passent toujours**

Run :
```bash
./tests/run.sh
```
Expected : `OK — tous les tests passent`.

- [ ] **Step 7 : Commit**

```bash
git add KorgZ3_SysEx_08-05-2026/KorgZ3_SysEx_08-05-2026.ino
git commit -m "sketch: intègre molette + bend range, pot #16 routé par le mux"
```

---

# Phase 3 — Rangement du dépôt

## Task 6 : Retirer le firmware d'origine et repointer les vérifications

**Files:**
- Delete: `hardware/firmware_CCSysEx_Patcher.ino`
- Delete: `hardware/firmware_LICENSE.txt`
- Modify: `hardware/ATTRIBUTION.md`
- Modify: `hardware/kicad_project/shield_redesign/check_fw.py`

- [ ] **Step 1 : Repointer `check_fw.py` sur le vrai sketch**

Remplacer l'intégralité de `hardware/kicad_project/shield_redesign/check_fw.py` par :

```python
import pathlib, re

# Verifie que le firmware reel est d'accord avec le cuivre de la carte.
# Chemin resolu quel que soit le cwd : shield_redesign -> kicad_project ->
# hardware -> racine du depot.
_here = pathlib.Path(__file__).resolve()
root = _here.parents[3]
fw_path = root / "KorgZ3_SysEx_08-05-2026" / "KorgZ3_SysEx_08-05-2026.ino"
mux_path = root / "KorgZ3_SysEx_08-05-2026" / "mux.cpp"
pitch_path = root / "KorgZ3_SysEx_08-05-2026" / "pitch.cpp"

fw = fw_path.read_text()
mux = mux_path.read_text()
pitch = pitch_path.read_text()

# --- Boutons et LEDs en reserve du bloc 2x18 ---
assert "butLayout2" in fw, "butLayout2 absent"
assert "LEDLayout2" in fw, "LEDLayout2 absent"
assert re.search(r'butLayout2\s*\[\s*3\s*\]\s*=\s*\{\s*22\s*,\s*24\s*,\s*26\s*\}', fw), \
    "butLayout2 valeurs incorrectes"
assert re.search(r'LEDLayout2\s*\[\s*3\s*\]\s*=\s*\{\s*23\s*,\s*25\s*,\s*27\s*\}', fw), \
    "LEDLayout2 valeurs incorrectes"
assert "INPUT_PULLUP" in fw, "pinMode pullup absent"

# --- Multiplexeur 4067 : selection D2/D3/D4/D13, commun sur A15 ---
assert re.search(r'MUX_SEL\s*\[\s*4\s*\]\s*=\s*\{\s*2\s*,\s*3\s*,\s*4\s*,\s*13\s*\}', mux), \
    "lignes de selection du mux incorrectes (attendu D2,D3,D4,D13)"
assert re.search(r'MUX_COM\s*=\s*A15', mux), "sortie commune du mux non cablee sur A15"

# --- Affectation des canaux ---
assert re.search(r'CH_WHEEL\s*=\s*1', pitch), "molette attendue sur le canal 1"
assert re.search(r'CH_SWITCH\s*=\s*2', pitch), "switch attendu sur le canal 2"

# --- Le pot #16 doit passer par le canal 0, pas par un analogRead(A15) direct ---
assert re.search(r'pot\.pin\s*==\s*A15\s*\)\s*\?\s*muxRead\(0\)', fw), \
    "le pot #16 ne passe pas par muxRead(0)"

print("check_fw OK — butLayout2{22,24,26} | LEDLayout2{23,25,27} | "
      "mux S0..S3={2,3,4,13} COM=A15 | molette ch1 | switch ch2 | pot16 ch0")
```

- [ ] **Step 2 : Exécuter la vérification**

Run :
```bash
python3 hardware/kicad_project/shield_redesign/check_fw.py
```
Expected : `check_fw OK — butLayout2{22,24,26} | LEDLayout2{23,25,27} | mux S0..S3={2,3,4,13} COM=A15 | molette ch1 | switch ch2 | pot16 ch0`

- [ ] **Step 3 : Supprimer le firmware d'origine**

Run :
```bash
git rm hardware/firmware_CCSysEx_Patcher.ino hardware/firmware_LICENSE.txt
```

- [ ] **Step 4 : Mettre à jour l'attribution**

Dans `hardware/ATTRIBUTION.md`, remplacer la puce :

```markdown
- `firmware_CCSysEx_Patcher.ino` is the original author's firmware, included for
  reference under its **MIT** license (see `firmware_LICENSE.txt`).
```

par :

```markdown
- The original author's firmware is **not redistributed here** — it targets a
  different synthesizer and its presence next to this project's own sketch was a
  source of confusion. It remains available under the MIT license in the
  [upstream repository](https://github.com/baritonomarchetto/arduino-SysEx-Patcher).
```

L'attribution du **matériel** (CC BY-NC-SA) reste inchangée : c'est elle qui engage le projet, la carte étant une œuvre dérivée.

- [ ] **Step 5 : Vérifier qu'aucune référence ne subsiste**

Run :
```bash
grep -rn "firmware_CCSysEx_Patcher\|firmware_LICENSE" --exclude-dir=.git . || echo "aucune référence résiduelle"
```
Expected : `aucune référence résiduelle`

- [ ] **Step 6 : Commit**

```bash
git add hardware/ATTRIBUTION.md hardware/kicad_project/shield_redesign/check_fw.py
git commit -m "hardware: retire le firmware d'origine, check_fw pointe sur le vrai sketch"
```

---

# Phase 4 — PCB v1.1

Cette phase se fait dans KiCad 10.0.3. Les scripts de `shield_redesign/` manipulent les fichiers s-expr directement ; `sexpr.py` fournit `find_blocks()`, `block_at()` et `prop()`.

## Task 7 : Schéma — mini-XLR 5 points et filtre du canal 2

**Files:**
- Modify: `hardware/kicad_project/SysEx_Patcher.kicad_sch`

- [ ] **Step 1 : Relever l'état actuel de J10**

Run :
```bash
grep -n -A5 '"Reference" "J10"' hardware/kicad_project/SysEx_Patcher.kicad_sch
```
Expected : symbole `Connector_Generic:Conn_01x04` à `(at 58.42 198.12 0)`, valeur `JACK6.35`, empreinte `Connector_Audio:Jack_6.35mm_Neutrik_NRJ6HF-1_Horizontal`.

- [ ] **Step 2 : Ouvrir le schéma dans KiCad et appliquer les modifications**

Run :
```bash
open -a KiCad hardware/kicad_project/SysEx_Patcher.kicad_pro
```

Dans l'éditeur de schéma :

1. Remplacer le symbole **J10** par `Connector_Generic:Conn_01x05`, valeur `MINI-XLR-5`, empreinte `Connector_Audio:MiniXLR-5_Switchcraft_TRAPC_Horizontal`.
2. Câbler les broches de J10 :
   - broche 1 → `GND`
   - broche 2 → `+5V`
   - broche 3 → filtre RC existant du jack → entrée `I1` du 4067 (câblage inchangé, seule l'origine change)
   - broche 4 → nouveau nœud `SW_BEND`
   - broche 5 → laissée libre, poser un symbole **No Connect** dessus pour que l'ERC ne la signale pas
3. Ajouter les trois composants du canal 2, entre la broche 4 et l'entrée `I2` du 4067 :
   - **R32** — 1 kΩ, en série depuis `SW_BEND` vers le nœud ADC
   - **C20** — 100 nF, du nœud ADC vers `GND`
   - **R31** — 470 kΩ, du nœud ADC vers `GND` (détection de présence)
4. Relier le nœud ADC à l'entrée **I2** du 4067 (U2).
5. Ajouter une étiquette de réseau `SW_BEND` sur le nœud, pour la lisibilité.

Sauvegarder.

- [ ] **Step 3 : Vérifier que les composants sont bien présents**

Run :
```bash
for ref in J10 R31 R32 C20; do
  printf "%-5s " "$ref"
  grep -c "\"Reference\" \"$ref\"" hardware/kicad_project/SysEx_Patcher.kicad_sch
done
grep -c 'Conn_01x05' hardware/kicad_project/SysEx_Patcher.kicad_sch
grep -c 'MiniXLR-5_Switchcraft_TRAPC_Horizontal' hardware/kicad_project/SysEx_Patcher.kicad_sch
```
Expected : `1` pour chacune des six lignes.

- [ ] **Step 4 : Lancer l'ERC**

Run :
```bash
/opt/homebrew/bin/kicad-cli sch erc --severity-error --exit-code-violations \
  -o /tmp/erc_v11.rpt hardware/kicad_project/SysEx_Patcher.kicad_sch; echo "code=$?"
```
Expected : `code=0`. Si des violations sortent, les lire dans `/tmp/erc_v11.rpt` et les corriger avant de continuer.

- [ ] **Step 5 : Commit**

```bash
git add hardware/kicad_project/SysEx_Patcher.kicad_sch
git commit -m "schéma v1.1: J10 en mini-XLR 5 points + filtre RC et pull-down du canal switch"
```

---

## Task 8 : Étendre la vérification du schéma

**Files:**
- Modify: `hardware/kicad_project/shield_redesign/check_sch_nets.py`

- [ ] **Step 1 : Ajouter les contrôles v1.1**

Dans `hardware/kicad_project/shield_redesign/check_sch_nets.py`, insérer **avant** la ligne `print(...)` finale :

```python
# --- v1.1 : mini-XLR 5 points et canal switch ---
assert re.search(r'Conn_01x05', sch), "J10 n'est pas un connecteur 5 points"
assert re.search(r'MiniXLR-5_Switchcraft_TRAPC_Horizontal', sch), \
    "empreinte mini-XLR absente"
for ref in ["R31", "R32", "C20"]:
    assert re.search(r'"Reference" "%s"' % ref, sch), \
        "%s absent (filtre / pull-down du canal switch)" % ref
assert re.search(r'\(label "SW_BEND"', sch), "net-label SW_BEND absent"
```

Puis remplacer la ligne `print(...)` finale par :

```python
print("check_sch_nets OK — D22..D27 | SW6/SW7/SW8 | R28/R29/R30 | D7/D8/D9 | "
      "J10 mini-XLR 5pts | R31/R32/C20 | SW_BEND")
```

- [ ] **Step 2 : Exécuter la vérification**

Run :
```bash
cd hardware/kicad_project && python3 shield_redesign/check_sch_nets.py; cd -
```
Expected : `check_sch_nets OK — D22..D27 | SW6/SW7/SW8 | R28/R29/R30 | D7/D8/D9 | J10 mini-XLR 5pts | R31/R32/C20 | SW_BEND`

- [ ] **Step 3 : Commit**

```bash
git add hardware/kicad_project/shield_redesign/check_sch_nets.py
git commit -m "check_sch_nets: vérifie le mini-XLR 5 points et le filtre du canal switch"
```

---

## Task 9 : PCB — empreinte et placement

**Files:**
- Modify: `hardware/kicad_project/SysEx_Patcher.kicad_pcb`

- [ ] **Step 1 : Importer les modifications du schéma dans le PCB**

Dans l'éditeur de PCB KiCad : **Outils → Mettre à jour le PCB depuis le schéma** (`F8`), en cochant le remplacement des empreintes modifiées. J10 prend l'empreinte mini-XLR ; R31, R32 et C20 apparaissent hors de la carte.

- [ ] **Step 2 : Placer les composants**

Placer les trois nouveaux composants **dans l'espace libre entre les deux rangées de headers Mega**, à côté du 4067 et du filtre du canal 1 — c'est là que la v1.0 a déjà logé le mux, précisément parce que la rangée analogique est trop dense.

Positionner J10 sur le **bord haut** de la carte, comme les autres entrées (MIDI IN/OUT, jack DC), nez du connecteur vers l'extérieur : on branche le câble par le haut.

Contrainte à respecter, héritée de la v1.0 : ne poser aucun composant haut au-dessus du coin où se trouvent les pièces hautes du Mega (USB, jack d'alimentation, condensateurs).

- [ ] **Step 3 : Vérifier le placement et l'empreinte**

Run :
```bash
grep -c 'MiniXLR-5_Switchcraft_TRAPC_Horizontal' hardware/kicad_project/SysEx_Patcher.kicad_pcb
for ref in R31 R32 C20; do
  printf "%-4s " "$ref"
  grep -c "\"Reference\" \"$ref\"" hardware/kicad_project/SysEx_Patcher.kicad_pcb
done
```
Expected : `1` pour chacune des quatre lignes.

- [ ] **Step 4 : Commit**

```bash
git add hardware/kicad_project/SysEx_Patcher.kicad_pcb
git commit -m "pcb v1.1: empreinte mini-XLR + placement du filtre et du pull-down"
```

---

## Task 10 : PCB — routage et DRC

**Files:**
- Modify: `hardware/kicad_project/SysEx_Patcher.kicad_pcb`

- [ ] **Step 1 : Router les nouvelles pistes**

Router les liaisons de J10, R31, R32 et C20. La v1.0 est routée à 100 % ; seules les pistes de l'ancien jack sont à reprendre, plus les trois nouveaux composants. Conserver les règles existantes : 4 couches, plan de masse, **100 % traversant**.

- [ ] **Step 2 : Vérifier qu'il ne reste aucune connexion manquante**

Run :
```bash
python3 hardware/kicad_project/shield_redesign/check_drc.py
```
Expected : **0 violation** et **0 connexion manquante**. Tant qu'il reste des connexions manquantes, le routage n'est pas fini — ne pas passer à la suite.

- [ ] **Step 3 : Regénérer le rendu de contrôle**

Run :
```bash
cd hardware/kicad_project && python3 shield_redesign/render.py; cd -
```
Expected : `pcb_shield_final_preview.png` régénéré. L'ouvrir et vérifier à l'œil que J10 est bien sur le bord haut et que rien ne chevauche les headers Mega.

- [ ] **Step 4 : Commit**

```bash
git add hardware/kicad_project/SysEx_Patcher.kicad_pcb hardware/kicad_project/pcb_shield_final_preview.png
git commit -m "pcb v1.1: routage complet du canal switch (0 connexion manquante)"
```

---

## Task 11 : Regénérer les Gerbers

**Files:**
- Modify: `hardware/fab/*`

- [ ] **Step 1 : Produire les Gerbers et le perçage**

Run :
```bash
/opt/homebrew/bin/kicad-cli pcb export gerbers \
  --output hardware/fab/ \
  hardware/kicad_project/SysEx_Patcher.kicad_pcb
/opt/homebrew/bin/kicad-cli pcb export drill \
  --output hardware/fab/ \
  hardware/kicad_project/SysEx_Patcher.kicad_pcb
```
Expected : les fichiers `hardware/fab/SysEx_Patcher-*.g*` et `SysEx_Patcher.drl` sont réécrits.

- [ ] **Step 2 : Reconstituer l'archive de fabrication**

Run :
```bash
cd hardware/fab && rm -f SysEx_Patcher_GERBERS.zip && \
  zip -q SysEx_Patcher_GERBERS.zip SysEx_Patcher-*.g* SysEx_Patcher.drl && \
  unzip -l SysEx_Patcher_GERBERS.zip | tail -3; cd -
```
Expected : l'archive contient les 9 couches plus le fichier de perçage.

- [ ] **Step 3 : Vérifier la date des fichiers**

Run :
```bash
ls -la hardware/fab/ | head -15
```
Expected : tous les fichiers portent la date du jour. Un fichier resté à une date antérieure signale un export incomplet — la v1.0 a déjà connu ce piège (commit `95d6e40`, zip périmé).

- [ ] **Step 4 : Commit**

```bash
git add hardware/fab/
git commit -m "fab: Gerbers v1.1 régénérés depuis le board routé (mini-XLR + canal switch)"
```

---

# Phase 5 — Documentation

## Task 12 : Documenter la v1.1

**Files:**
- Modify: `hardware/RECONSTRUCTION_SCHEMA.md`
- Modify: `README.md`

- [ ] **Step 1 : Ajouter la section v1.1 au document matériel**

Dans `hardware/RECONSTRUCTION_SCHEMA.md`, insérer après la section `## 0ter. MODIFICATION v1.0` :

```markdown
## 0quater. MODIFICATION v1.1 — molette de pitch et bend range déportés sur la guitare

- **J10** passe du jack 6,35 stéréo au **mini-XLR 5 points Switchcraft TRAPC**
  (empreinte `Connector_Audio:MiniXLR-5_Switchcraft_TRAPC_Horizontal`, traversante).
  Le mini-XLR **4 points** aurait suffi mais n'a pas d'empreinte dans la bibliothèque
  KiCad standard ; la 5ᵉ broche reste libre.
- **Brochage** : 1 = GND, 2 = +5 V, 3 = curseur molette → canal 1 du 4067,
  4 = tension du switch → canal 2 du 4067, 5 = réserve.
- **Nouveaux composants** : R32 (1 kΩ série) + C20 (100 nF vers GND) forment le filtre
  anti-bruit du canal 2 ; **R31 (470 kΩ vers GND)** assure la détection de présence.
- **Côté guitare** : molette 10 kΩ à ressort, switch **ON-ON-ON** 3 positions, et quatre
  résistances de 2,2 kΩ en série entre +5 V et GND. Le switch sélectionne un des trois
  points intermédiaires du diviseur.

| Position | Tension | Valeur ADC | Bend range |
|---|---|---|---|
| 1 | ¼ Vcc | ~256 | ±1 ton |
| 2 | ½ Vcc | ~512 | ±1 ton et demi |
| 3 | ¾ Vcc | ~768 | ±1 octave |
| *(rien branché)* | ~0 V | ~0 | molette ignorée, pitch figé à 8192 |

**Pourquoi des taps à ¼ / ½ / ¾ et non 0 / ½ / 1.** Aucune position valide ne produit 0 V.
Câble débranché, R31 tire la ligne à ~0 V, valeur qui n'appartient à aucune fenêtre : le
firmware sait qu'il n'y a rien de branché et gèle le pitch, au lieu de lire une entrée en
l'air et d'envoyer un bend fantôme.

**Contrainte mécanique** : l'embase côté guitare est encastrée dans le pickguard de la
Stratocaster, la fiche du câble doit donc être **coudée**.
```

- [ ] **Step 2 : Mettre à jour le README**

Dans `README.md`, ajouter à la fin de la section « Hardware », après le paragraphe sur le mux 4067 :

```markdown
### Molette de pitch (v1.1)

Une molette de pitch et un switch 3 positions de bend range (±1 ton, ±1 ton et demi,
±1 octave) se montent **sur la guitare** et se relient au shield par un unique câble
mini-XLR de 4 conducteurs. Le Korg Z3 mappe en dur sa plage de pitch bend sur ±12
demi-tons ; le firmware restreint la course MIDI émise pour obtenir une amplitude
musicale plus fine.

Le switch est lu en analogique par le multiplexeur : ses trois positions valent ¼, ½ et
¾ de Vcc, ce qui libère 0 V comme signature « câble débranché ». Rien de branché, la
molette est ignorée et le pitch reste au centre.
```

- [ ] **Step 3 : Vérifier les liens et la cohérence**

Run :
```bash
grep -n "firmware_CCSysEx_Patcher\|Jack_6.35mm_Neutrik" README.md hardware/RECONSTRUCTION_SCHEMA.md hardware/ATTRIBUTION.md || echo "aucune référence périmée"
```
Expected : `aucune référence périmée`

- [ ] **Step 4 : Lancer toutes les vérifications une dernière fois**

Run :
```bash
./tests/run.sh
arduino-cli compile --fqbn arduino:avr:mega KorgZ3_SysEx_08-05-2026 2>&1 | tail -2
python3 hardware/kicad_project/shield_redesign/check_fw.py
cd hardware/kicad_project && python3 shield_redesign/check_sch_nets.py && \
  python3 shield_redesign/check_drc.py; cd -
```
Expected : tests OK, compilation réussie, `check_fw OK`, `check_sch_nets OK`, DRC à 0 violation et 0 connexion manquante.

- [ ] **Step 5 : Commit**

```bash
git add README.md hardware/RECONSTRUCTION_SCHEMA.md
git commit -m "doc: décrit la v1.1 — molette et bend range déportés sur la guitare"
```

---

## À faire à réception du PCB

Hors périmètre de ce plan, mais à ne pas perdre :

1. **Monter le banc de test** — platine avec pot 10 kΩ, switch ON-ON-ON et les quatre
   résistances de 2,2 kΩ, câblée sur une fiche mini-XLR : la guitare simulée. Valider la
   chaîne électrique avant de toucher au pickguard.
2. **Relever la valeur brute au repos** de la molette réelle et réajuster `DEADZONE_IN` /
   `DEADZONE_OUT` (40 / 55) si le ressort oscille davantage que sur le montage d'essai.
3. **Mesurer la gêne du blocage SysEx** de 192 ms. Si elle s'entend, évaluer les deux
   pistes en réserve : réduire le `delay(2)` de `sendSysEx()`, ou passer l'envoi en machine
   à états non bloquante.
4. **Valider la référence de la fiche mini-XLR coudée** — c'est le point
   d'approvisionnement le plus incertain, à régler **avant** de commander le PCB.
