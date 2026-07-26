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
