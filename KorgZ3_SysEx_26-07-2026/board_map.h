#pragma once
#include <Arduino.h>

// ============================================================================
//  CARTE v1.2 — TRADUCTION VERS LES BROCHES REELLES DU MEGA
// ============================================================================
//
// Defaut de conception de la carte fabriquee : l'ordre des broches est INVERSE
// a l'interieur de chacun des 6 blocs d'embases Mega. Les blocs sont bien
// places (la carte s'enfiche), mais l'affectation des nets aux pastilles est
// faite de la derniere broche vers la premiere.
//
// Table complete : hardware/CORRESPONDANCE_BROCHES.md
//
// Le reste du firmware continue de raisonner en numeros du SCHEMA du shield.
// Ce fichier est le seul endroit qui connait le decalage. Une v1.5 de la carte,
// avec l'ordre corrige, n'aura qu'a rendre ces deux fonctions identitaires.
//
// Reparation materielle associee (10/08/2026) : les broches males 2, 3 et 4 de
// JMP1 ont ete retirees et remplacees par deux cavaliers 3->6 (masse) et
// 4->5 (+5V). Sans elle la carte n'est pas alimentee et son plan de masse
// tient la broche RESET du Mega.
//
// ----------------------------------------------------------------------------
//  ANALOGIQUE : shield An -> Mega A(7-n) si n < 8, A(23-n) sinon
// ----------------------------------------------------------------------------
constexpr uint8_t megaAnalog(uint8_t shieldPin) {
  return (shieldPin < A0 || shieldPin > A15)
           ? shieldPin
           : (uint8_t)(A0 + ((shieldPin - A0) < 8 ? 7 - (shieldPin - A0)
                                                  : 23 - (shieldPin - A0)));
}

// ----------------------------------------------------------------------------
//  NUMERIQUE : shield Dn -> broche Mega reellement touchee
// ----------------------------------------------------------------------------
// D10 tombe sur AREF et D11 sur une broche de masse : ces deux signaux du
// shield sont electriquement inexploitables. D14/D8 et D15/D9 tombent deux par
// deux sur la meme broche (le Mega expose SDA/SCL a deux endroits).
constexpr uint8_t megaDigital(uint8_t d) {
  return d == 0  ? 7  : d == 1  ? 6  : d == 2  ? 5  : d == 3  ? 4  :
         d == 4  ? 3  : d == 5  ? 2  : d == 6  ? 1  : d == 7  ? 0  :
         d == 8  ? 21 : d == 9  ? 20 : d == 12 ? 13 : d == 13 ? 12 :
         d == 14 ? 21 : d == 15 ? 20 : d == 16 ? 19 : d == 17 ? 18 :
         d == 18 ? 17 : d == 19 ? 16 : 255;   // 255 = inexploitable (D10, D11)
}

// ----------------------------------------------------------------------------
//  BROCHES LIBRES DE SECOURS
// ----------------------------------------------------------------------------
// Le bloc 2x18 du Mega (D22-D53) n'est pas cable sur le shield : ces broches ne
// touchent rien. On y renvoie les fonctions dont la broche reelle est
// inutilisable, plutot que de disperser des tests dans tout le code. Une LED
// ainsi renvoyee ne s'allume simplement jamais ; un bouton lit toujours HIGH,
// c'est-a-dire relache.
// Lignes I2C de l'ecran : les nets D20_SDA / D21_SCL du shield tombent sur les
// broches 9 et 8 du Mega (et aussi sur 15 et 14, le Mega exposant SDA/SCL a
// deux endroits ; ces deux-la restent en entree, sans effet). Pas d'I2C
// materiel dessus, donc bus logiciel — voir oled.h. Aucun fil a ajouter.
constexpr uint8_t OLED_SDA_PIN = 9;
constexpr uint8_t OLED_SCL_PIN = 8;
constexpr bool    OLED_UTILISABLE = true;

constexpr uint8_t PIN_LIBRE_1 = 30;
constexpr uint8_t PIN_LIBRE_2 = 31;
constexpr uint8_t PIN_LIBRE_3 = 32;
constexpr uint8_t PIN_LIBRE_4 = 33;
constexpr uint8_t PIN_LIBRE_5 = 34;
constexpr uint8_t PIN_LIBRE_6 = 35;
constexpr uint8_t PIN_LIBRE_7 = 36;
constexpr uint8_t PIN_LIBRE_8 = 37;
constexpr uint8_t PIN_LIBRE_9 = 38;
constexpr uint8_t PIN_LIBRE_10 = 39;

// ----------------------------------------------------------------------------
//  INTERDIT ABSOLU
// ----------------------------------------------------------------------------
// La broche 11 du Mega porte le PLAN DE MASSE du shield. La configurer en
// sortie et la mettre a l'etat haut met le +5V en court-circuit franc sur la
// masse a travers la broche du microcontroleur, qui n'y survit pas.
// Aucune constante de ce firmware ne doit valoir 11.
static_assert(megaDigital(10) == 255, "D10 doit rester marque inexploitable");
static_assert(megaDigital(11) == 255, "D11 doit rester marque inexploitable");
