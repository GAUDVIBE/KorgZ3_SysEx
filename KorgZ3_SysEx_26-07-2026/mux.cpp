#include "mux.h"
#include "board_map.h"

// Numeros du schema du shield : S0-S3 = D2/D3/D4/D13, COM = A15. Traduits vers
// les broches reelles du Mega (5/4/3/12 et A8) — voir board_map.h.
static const byte MUX_SEL[4] = {megaDigital(2), megaDigital(3),
                                megaDigital(4), megaDigital(13)};
static const int  MUX_COM    = megaAnalog(A15);

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
