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

#endif
