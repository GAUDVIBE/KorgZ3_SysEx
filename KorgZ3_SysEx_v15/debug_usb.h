#pragma once
#include <Arduino.h>

// ============================================================================
//  TRACES DE DEBOGAGE — carte v1.5
// ============================================================================
//
// Sur cette carte, Serial0 porte le MIDI (voir midi_port.h) : ecrire du texte
// dessus corromprait le flux SysEx. Les traces passent donc par DBG, qui part
// dans le vide tant que DEBUG_USB vaut 0.
//
// Mettre DEBUG_USB a 1 rend les traces sur le port USB — et rend du meme coup
// le MIDI inutilisable tant que le firmware n'est pas recompile a 0. C'est un
// choix de mise au point, jamais un reglage de jeu.
//
// Le puits est resolu a la compilation : a 0, chaque appel DBG.print(...) se
// reduit a rien et n'occupe ni octet de programme ni cycle.

#define DEBUG_USB 0

#if DEBUG_USB

using FluxDebug = decltype(Serial);
#define DBG Serial
#define DEBUG_USB_BEGIN() Serial.begin(115200)

#else

struct PuitsDebug {
  template <typename... T> void print(T...) {}
  template <typename... T> void println(T...) {}
  template <typename... T> void write(T...) {}
  void flush() {}
  explicit operator bool() const { return false; }
};
extern PuitsDebug DBG;
#define DEBUG_USB_BEGIN() do {} while (0)

#endif
