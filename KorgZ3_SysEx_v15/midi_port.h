#pragma once
#include <Arduino.h>

// ============================================================================
//  PORT MIDI — carte v1.5
// ============================================================================
//
// Le shield cable le MIDI sur ses nets D0_RX / D1_TX. Sur la v1.5 ces nets
// tombent sur les vraies broches 0 et 1 du Mega, c'est-a-dire sur l'UART
// MATERIEL Serial0 — ce que le schema d'origine avait toujours voulu dire.
//
// MIDI OUT : fonctionne.
// MIDI IN  : fonctionne de nouveau. Sur la v1.2 il etait perdu parce que le net
//            D0_RX atterrissait sur la broche 7, qui n'est pas une broche
//            d'UART ; le port logiciel de secours ne pouvait pas y recevoir
//            (SoftwareSerial exige une broche a interruption de changement
//            d'etat, et la broche 0 n'en est pas une non plus). Aucun port
//            logiciel ne pouvait donc s'en sortir : il fallait la carte droite.
//
// ⚠️ CONTREPARTIE — Serial0 est aussi le port USB. Le MIDI et le debogage ne
// peuvent plus coexister : toute impression de texte sur Serial injecterait des
// octets parasites dans le flux MIDI, jusqu'a faire deraper le Z3. C'est
// pourquoi les traces de debogage passent par DBG (voir debug_usb.h) et sont
// desactivees par defaut dans ce dossier.
//
// Pour deboguer malgre tout : mettre DEBUG_USB a 1 dans debug_usb.h, en sachant
// que le MIDI est alors inutilisable. Le port de debogage separe qu'offrait la
// v1.2 etait un effet de bord de son defaut, pas une fonction a conserver.

#define MIDI_PORT Serial

constexpr unsigned long MIDI_BAUD = 31250;
