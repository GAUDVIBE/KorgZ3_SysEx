#ifndef MUX_H
#define MUX_H

#include <Arduino.h>

// Multiplexeur analogique CD74HC4067 (U2) du shield.
// Sortie commune COM -> A15 ; selection S0..S3 -> D2, D3, D4, D13 ;
// ~E force a la masse (toujours actif).
//
// ATTENTION : S3 occupe D13, qui est aussi la LED integree du Mega. Elle
// clignotera au rythme des selections de canal (sans consequence), mais tout
// code de debug qui ecrirait sur D13 casserait silencieusement le mux.
//
// Affectation des canaux :
//   0 = potentiometre #16 (autrefois cable en direct sur A15)
//   1 = curseur de la molette de pitch (broche 3 du mini-XLR)
//   2 = tension du switch de bend range (broche 4 du mini-XLR)
//   3..15 = libres, sortis sur le header J9

void muxBegin();
int  muxRead(byte channel);

#endif
