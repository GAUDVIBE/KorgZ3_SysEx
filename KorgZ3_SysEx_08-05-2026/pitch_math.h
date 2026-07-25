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
