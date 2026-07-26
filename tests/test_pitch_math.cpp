// Tests natifs de pitch_math.h — aucune dépendance Arduino.
#include <cstdio>
#include <cstdlib>
#include "../KorgZ3_SysEx_26-07-2026/pitch_math.h"

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

int main() {
  test_switch_windows();
  test_pitch_value14();
  if (failures == 0) { printf("OK — tous les tests passent\n"); return 0; }
  printf("%d echec(s)\n", failures);
  return 1;
}
