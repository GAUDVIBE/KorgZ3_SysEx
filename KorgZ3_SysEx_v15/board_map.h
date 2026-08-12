#pragma once
#include <Arduino.h>

// ============================================================================
//  CARTE v1.5 — AUCUNE TRADUCTION : LE SHIELD EST CABLE DROIT
// ============================================================================
//
// La v1.2 fabriquee avait l'ordre des broches INVERSE a l'interieur de chacun
// des 6 blocs d'embases Mega. Son board_map.h (dossier KorgZ3_SysEx_26-07-2026)
// rattrapait ce decalage en logiciel, au prix de plusieurs fonctions perdues.
//
// La v1.5 corrige le defaut dans le cuivre : chaque pastille porte le net de la
// broche Mega qu'elle touche reellement (hardware/kicad_project, verifie par
// shield_redesign/check_pinorder.py). Les deux fonctions de traduction sont donc
// l'IDENTITE, exactement comme l'annoncait le fichier de la v1.2.
//
// ⚠️ NE PAS TELEVERSER CE FIRMWARE SUR UNE CARTE v1.2 : les numeros de broches
// y designeraient les mauvaises broches, et la page V mettrait le plan de masse
// en sortie a l'etat haut (court-circuit franc du +5V, broche detruite).
//
// ----------------------------------------------------------------------------
//  ANALOGIQUE / NUMERIQUE : identite
// ----------------------------------------------------------------------------
constexpr uint8_t megaAnalog(uint8_t shieldPin) { return shieldPin; }
constexpr uint8_t megaDigital(uint8_t d) { return d; }

// ----------------------------------------------------------------------------
//  ECRAN OLED
// ----------------------------------------------------------------------------
// Les nets D20_SDA / D21_SCL du shield tombent maintenant sur les vraies broches
// 20 et 21 du Mega, c'est-a-dire sur son I2C MATERIEL. On conserve neanmoins le
// bus logiciel de la v1.2 (voir oled.h) : il fonctionne sur n'importe quelle
// paire de broches, il est deja eprouve sur cette carte, et l'ecran n'a aucun
// besoin de debit. Passer a Wire est possible plus tard, ce n'est pas un prealable.
constexpr uint8_t OLED_SDA_PIN = 20;
constexpr uint8_t OLED_SCL_PIN = 21;
constexpr bool    OLED_UTILISABLE = true;

// ----------------------------------------------------------------------------
//  PLUS AUCUNE BROCHE DE SECOURS, PLUS AUCUNE BROCHE INTERDITE
// ----------------------------------------------------------------------------
// La v1.2 renvoyait vers le bloc 2x18 (D22-D53) les fonctions dont la broche
// reelle etait inexploitable — D10 tombait sur AREF, D11 sur une masse. Ces deux
// signaux sont rendus a leur usage, les cinq pages retrouvent leur bouton et leur
// LED, et le Dump Request retrouve les siens. Les constantes PIN_LIBRE_* n'ont
// donc plus de raison d'exister : leur absence est volontaire, et toute
// reapparition dans ce dossier signalerait un reste de contournement v1.2.
//
// De meme, la broche 11 n'est plus le plan de masse du shield mais un D11 normal.
static_assert(megaDigital(10) == 10, "v1.5 : D10 doit etre exploitable");
static_assert(megaDigital(11) == 11, "v1.5 : D11 doit etre exploitable");
static_assert(megaAnalog(A0) == A0, "v1.5 : la rangee analogique doit etre droite");
static_assert(megaAnalog(A15) == A15, "v1.5 : la rangee analogique doit etre droite");
