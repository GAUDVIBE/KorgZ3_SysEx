#pragma once
#include <SoftwareSerial.h>

// ============================================================================
//  PORT MIDI — carte v1.2
// ============================================================================
//
// Le MIDI etait cable sur Serial0 (D0/D1), partage avec l'USB. Avec l'ordre des
// broches inverse (voir board_map.h), les deux signaux MIDI du shield tombent
// sur les broches 7 et 6 du Mega, qui ne sont pas des broches d'UART. On passe
// donc par un port serie logiciel.
//
// Effet de bord bienvenu : Serial0 est desormais libre pour l'USB seul, on peut
// deboguer a 115200 sans melanger du texte avec des octets SysEx.
//
// MIDI OUT : fonctionne. La broche 6 du Mega porte bien le net D1_TX du shield,
//            donc l'optocoupleur de sortie.
//
// MIDI IN  : NE FONCTIONNE PAS sur cette revision de carte. La sortie de
//            l'optocoupleur d'entree arrive sur la broche 7 du Mega, et
//            SoftwareSerial ne sait recevoir que sur une broche a interruption
//            de changement d'etat (10-15, 50-53, A8-A15 sur un Mega). La broche
//            de reception declaree ci-dessous est donc une broche libre du bloc
//            2x18, sur laquelle rien n'arrivera jamais : le Dump Request part,
//            mais la reponse du Z3 n'est pas lue.
//            Pour le retablir : un fil de la sortie de l'optocoupleur vers une
//            broche a interruption, ou vers D19 (RX1) pour un vrai UART.
constexpr uint8_t MIDI_TX_PIN = 6;   // net D1_TX du shield
constexpr uint8_t MIDI_RX_PIN = 53;  // broche libre, reception inoperante

extern SoftwareSerial MIDI_PORT;
