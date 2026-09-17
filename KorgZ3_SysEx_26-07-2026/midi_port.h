#pragma once
#include <SoftwareSerial.h>

// ============================================================================
//  ⚠️ OPTION DE COMPILATION OBLIGATOIRE : -D_SS_MAX_RX_BUFF=192
// ============================================================================
//
// Le tampon de reception de SoftwareSerial vaut 64 octets par defaut, soit
// 20 ms de flux a 31250 bauds. Un dump du Korg Z3 fait 95 octets et dure 30 ms :
// il DEBORDE systematiquement, et la carte n'en recoit que 64 a 66 (mesure du
// 17/09/2026). Le dump tronque etait alors renvoye au Z3, qui affichait « err. ».
//
// Compiler ainsi (le fichier build_opt.h N'EST PAS honore par cette version
// d'arduino-cli — verifie : la RAM reste a 5432 octets au lieu de 5560) :
//
//   arduino-cli compile --fqbn arduino:avr:mega \
//     --build-property "compiler.cpp.extra_flags=-D_SS_MAX_RX_BUFF=192" \
//     KorgZ3_SysEx_26-07-2026
//
// CONTROLE : la RAM annoncee doit valoir 5560 octets, pas 5432. L'ecart de 128
// octets est exactement l'agrandissement du tampon. Compile depuis l'IDE sans
// cette option, le firmware perdra de nouveau des dumps en silence.

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
