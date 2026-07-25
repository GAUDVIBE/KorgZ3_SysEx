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
