#pragma once
#include <U8g2lib.h>
#include "board_map.h"

// ============================================================================
//  ECRAN OLED — I2C LOGICIEL (carte v1.2)
// ============================================================================
//
// Les nets D20_SDA / D21_SCL du shield ne tombent pas sur les broches d'I2C
// materiel du Mega mais sur les broches 9 et 8 (voir board_map.h). Plutot que
// d'ajouter des fils, on pilote le bus en logiciel : U8g2 le fait sur
// n'importe quelle broche, et l'ecran, lui, ne voit aucune difference.
//
// Cette classe expose l'interface d'Adafruit_SSD1306 utilisee par le firmware,
// pour que le code d'affichage existant reste inchange. Le jour ou une carte au
// brochage correct ramene l'ecran sur D20/D21, il suffira de repasser a
// Adafruit_SSD1306 — ou de ne rien faire, l'I2C logiciel marche aussi bien.

#define SSD1306_WHITE 1

// ----------------------------------------------------------------------------
//  PILOTAGE RAPIDE DU BUS I2C LOGICIEL
// ----------------------------------------------------------------------------
// U8g2 emule le drain ouvert en appelant pinMode() PUIS digitalWrite() a chaque
// front. Sur AVR ces deux fonctions passent par des tables en PROGMEM et un
// test de PWM : le bus plafonne a 16 kHz et le transfert des 512 octets du
// tampon dure 288 ms — pendant lesquelles loop() est bloquee, donc la molette
// de pitch sourde (elle est servie une fois par tour, toutes les 12 ms).
//
// On remplace le callback GPIO d'U8g2 par un acces direct aux registres.
// Mesure du 12/08/2026 sur la carte : 283,6 ms -> 90,1 ms, soit x3,15, rendu
// verifie net a l'ecran. Ce qui reste est le cout propre d'U8g2 par bit
// (~19 us), pas le cablage.
//
// Relacher une ligne = entree + pull-up ; la tirer = sortie a 0. On met PORT a
// 0 AVANT de passer en sortie : jamais de niveau haut impose sur le bus.
// ⚠️ Le test porte sur des constexpr, PAS sur des macros : il doit donc etre
// evalue par le COMPILATEUR (constexpr bool), jamais par le preprocesseur.
// Un #if OLED_SCL_PIN == 8 remplacerait l'identifiant inconnu par 0 et
// desactiverait ce pilotage en silence — erreur commise puis mesuree le
// 12/08/2026 : le transfert restait a 288 ms sans le moindre avertissement.
constexpr bool OLED_I2C_RAPIDE = (OLED_SCL_PIN == 8 && OLED_SDA_PIN == 9);
constexpr uint8_t OLED_SCL_BIT = (1 << 5);   // D8 = PH5 sur un Mega 2560
constexpr uint8_t OLED_SDA_BIT = (1 << 6);   // D9 = PH6

inline uint8_t oledGpioRapide(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr) {
  (void)u8x8; (void)arg_ptr;
  switch (msg) {
    case U8X8_MSG_GPIO_AND_DELAY_INIT:
      DDRH  &= ~(OLED_SCL_BIT | OLED_SDA_BIT);
      PORTH |=  (OLED_SCL_BIT | OLED_SDA_BIT);
      break;
    case U8X8_MSG_GPIO_I2C_CLOCK:
      if (arg_int) { PORTH |= OLED_SCL_BIT;  DDRH &= ~OLED_SCL_BIT; }
      else         { PORTH &= ~OLED_SCL_BIT; DDRH |=  OLED_SCL_BIT; }
      break;
    case U8X8_MSG_GPIO_I2C_DATA:
      if (arg_int) { PORTH |= OLED_SDA_BIT;  DDRH &= ~OLED_SDA_BIT; }
      else         { PORTH &= ~OLED_SDA_BIT; DDRH |=  OLED_SDA_BIT; }
      break;
    case U8X8_MSG_DELAY_MILLI:   delay(arg_int); break;
    case U8X8_MSG_DELAY_10MICRO: delayMicroseconds(10 * arg_int); break;
    case U8X8_MSG_DELAY_I2C:     break;   // aucune attente : mesure a l'appui
    case U8X8_MSG_DELAY_NANO:
    case U8X8_MSG_DELAY_100NANO: break;
    default: return 0;
  }
  return 1;
}
// Sur un autre brochage (la v1.5 ramene l'ecran sur 20/21), OLED_I2C_RAPIDE
// vaut false et U8g2 garde son pilotage d'origine : correct, seulement lent.

// ----------------------------------------------------------------------------
//  POMPE : rendre la main pendant les transferts longs
// ----------------------------------------------------------------------------
// loop() est cooperative et pitchUpdate() n'est appelee qu'une fois par tour :
// tout traitement long est donc un trou dans la molette de pitch, qui attend
// d'etre servie toutes les 12 ms. Meme accelere, l'envoi du tampon dure 90 ms.
//
// L'I2C etant pilote par le maitre, rien n'interdit de le decouper : on envoie
// le tampon par tranches et on appelle cette pompe entre chacune. Definie dans
// le .ino, elle sert la molette. Le bus reste coherent, chaque tranche etant
// une transaction complete.
void ecranPompe();

class Ecran : public Print {
public:
  Ecran() : u8g2(U8G2_R2, OLED_SCL_PIN, OLED_SDA_PIN, U8X8_PIN_NONE) {}

  bool begin() {
    // A poser AVANT begin() : c'est lui qui declenche l'init des broches.
    if (OLED_I2C_RAPIDE) u8g2.getU8x8()->gpio_and_delay_cb = oledGpioRapide;
    bool ok = u8g2.begin();
    u8g2.setFont(u8g2_font_6x10_tf);
    u8g2.setFontPosBaseline();
    return ok;
  }

  void setRotation(uint8_t) {}                  // fixee a la construction (R2)
  void setTextColor(uint16_t) {}                // monochrome : sans objet
  void setTextSize(uint8_t) { u8g2.setFont(u8g2_font_6x10_tf); }
  void clearDisplay()       { u8g2.clearBuffer(); }

  // Envoi par tranches, avec la main rendue entre chacune. L'ecran fait 16x4
  // tuiles ; 8 tranches de 8x1 ramenent chaque blocage a ~13 ms, sous la
  // periode de 12 ms de la molette, au lieu d'un bloc unique de 90 ms.
  void display() {
    for (uint8_t ty = 0; ty < 4; ty++) {
      for (uint8_t tx = 0; tx < 16; tx += 8) {
        u8g2.updateDisplayArea(tx, ty, 8, 1);
        ecranPompe();
      }
    }
  }

  // Envoi d'un seul bloc, sans rendre la main. Reserve aux cas ou la molette
  // est de toute facon hors jeu (ecrans de demarrage suivis d'un delay).
  void displayBloquant()    { u8g2.sendBuffer(); }

  // Adafruit compte y depuis le HAUT du caractere, U8g2 depuis sa ligne de
  // base : on decale de la hauteur de la police pour garder les memes
  // coordonnees dans le code appelant.
  void setCursor(int16_t x, int16_t y) { u8g2.setCursor(x, y + 8); }

  size_t write(uint8_t c) override { return u8g2.write(c); }

private:
  U8G2_SSD1306_128X32_UNIVISION_F_SW_I2C u8g2;
};
