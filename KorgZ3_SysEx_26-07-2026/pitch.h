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
