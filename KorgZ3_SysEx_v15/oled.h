#pragma once
#include <U8g2lib.h>
#include "board_map.h"

// ============================================================================
//  ECRAN OLED — I2C MATERIEL (carte v1.5)
// ============================================================================
//
// Sur la v1.5, les nets D20_SDA / D21_SCL du shield tombent sur les vraies
// broches 20 et 21 du Mega, c'est-a-dire sur son I2C materiel. On l'utilise :
// a 400 kHz, le transfert des 512 octets du tampon revient a ~13 ms, contre
// 288 ms avec le bus logiciel de la v1.2 — mesure du 12/08/2026 sur carte.
//
// ⚠️ NON VERIFIE SUR MATERIEL : la carte v1.5 n'est pas fabriquee a ce jour.
// Le reste de ce fichier (decoupage + pompe) est, lui, mesure et valide sur la
// v1.2 ; seul le passage a Wire attend une carte pour etre confirme.
//
// Cette classe expose l'interface d'Adafruit_SSD1306 utilisee par le firmware,
// pour que le code d'affichage existant reste inchange.

static_assert(OLED_SDA_PIN == 20 && OLED_SCL_PIN == 21,
              "v1.5 : l'ecran doit etre sur l'I2C materiel (20/21)");

#define SSD1306_WHITE 1

// ----------------------------------------------------------------------------
//  POMPE : rendre la main pendant les transferts longs
// ----------------------------------------------------------------------------
// loop() est cooperative et pitchUpdate() n'est appelee qu'une fois par tour :
// tout traitement long est un trou dans la molette de pitch, servie toutes les
// 12 ms. Sur la v1.2, un rafraichissement d'ecran l'affamait 330 ms — le pitch
// bend sautait audiblement a chaque defilement automatique.
//
// L'I2C etant pilote par le maitre, on a le droit de decouper le transfert et
// de rendre la main entre les tranches. Definie dans le .ino, cette pompe sert
// la molette. Mesure sur v1.2 : famine ramenee de 330 ms a 20 ms.
//
// Le dessin des polices coute aussi ~43 ms : il est pompe de la meme facon,
// depuis updateDisplay(). Le transfert materiel de la v1.5 etant bien plus
// rapide, c'est meme lui qui domine ici.
void ecranPompe();

class Ecran : public Print {
public:
  Ecran() : u8g2(U8G2_R2, U8X8_PIN_NONE) {}

  bool begin() {
    bool ok = u8g2.begin();
    u8g2.setBusClock(400000);        // I2C rapide : sans objet en bus logiciel
    u8g2.setFont(u8g2_font_6x10_tf);
    u8g2.setFontPosBaseline();
    return ok;
  }

  void setRotation(uint8_t) {}                  // fixee a la construction (R2)
  void setTextColor(uint16_t) {}                // monochrome : sans objet
  void setTextSize(uint8_t) { u8g2.setFont(u8g2_font_6x10_tf); }
  void clearDisplay()       { u8g2.clearBuffer(); }

  // Envoi par tranches, avec la main rendue entre chacune. L'ecran fait 16x4
  // tuiles ; 8 tranches de 8x1 garantissent qu'aucune ne monopolise la boucle,
  // quelle que soit la vitesse du bus.
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
  U8G2_SSD1306_128X32_UNIVISION_F_HW_I2C u8g2;
};
