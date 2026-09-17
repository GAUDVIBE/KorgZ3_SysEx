// diag_ecran — le shield est-il alimente, et l'ecran repond-il ?
//
// Carte v1.2 : l'ecran est cable sur les nets D20_SDA / D21_SCL du shield, qui
// tombent sur les broches 9 et 8 du Mega. Pas d'I2C materiel dessus, donc bus
// logiciel — on le refait ici a la main, juste assez pour envoyer une adresse
// et lire l'accuse de reception (ACK). Un ecran vivant repond ; un ecran non
// alimente, mal connecte ou mort ne repond pas.
//
// Les 16 potentiometres sont alimentes par le +5V du shield : s'il manque, tous
// les curseurs sont a la masse et les lectures tombent a zero. C'est notre
// temoin d'alimentation, sans multimetre.
//
// SECURITE : ce croquis ne pilote QUE les broches 8 et 9 (lignes de l'ecran).
// La broche 11, sur laquelle repose le plan de masse du shield, n'est jamais
// touchee, ni aucune broche de LED ou de bouton.

const uint8_t SCL_PIN = 8;   // net D21_SCL du shield
const uint8_t SDA_PIN = 9;   // net D20_SDA du shield

// Bus volontairement LENT (~25 kHz) : les lignes ne remontent que par les
// resistances de tirage de l'ecran, et l'ecran n'en a pas forcement sur son
// horloge. Trop rapide, le bus ne monterait pas assez haut et l'ecran
// paraitrait muet a tort.
const uint8_t T = 20;                                       // microsecondes par demi-periode

inline void sclHaut() { pinMode(SCL_PIN, INPUT_PULLUP); }   // relachee : tiree par les resistances de l'ecran
inline void sclBas()  { pinMode(SCL_PIN, OUTPUT); digitalWrite(SCL_PIN, LOW); }
inline void sdaHaut() { pinMode(SDA_PIN, INPUT_PULLUP); }
inline void sdaBas()  { pinMode(SDA_PIN, OUTPUT); digitalWrite(SDA_PIN, LOW); }

void i2cDepart() { sdaHaut(); sclHaut(); delayMicroseconds(T); sdaBas(); delayMicroseconds(T); sclBas(); delayMicroseconds(T); }
void i2cArret()  { sdaBas(); sclHaut(); delayMicroseconds(T); sdaHaut(); delayMicroseconds(T); }

// Envoie un octet, renvoie true si l'esclave a tire SDA a l'etat bas (ACK).
bool i2cOctet(uint8_t v) {
  for (uint8_t i = 0; i < 8; i++) {
    if (v & 0x80) sdaHaut(); else sdaBas();
    v <<= 1;
    delayMicroseconds(T); sclHaut(); delayMicroseconds(T); sclBas(); delayMicroseconds(T);
  }
  sdaHaut();                       // on relache SDA : l'esclave repond
  delayMicroseconds(T); sclHaut(); delayMicroseconds(T);
  bool ack = (digitalRead(SDA_PIN) == LOW);
  sclBas(); delayMicroseconds(T);
  return ack;
}

bool sonde(uint8_t adr7) {
  i2cDepart();
  bool ack = i2cOctet(adr7 << 1);  // bit 0 a 0 = ecriture
  i2cArret();
  return ack;
}

void setup() {
  Serial.begin(115200);
  delay(200);
}

void loop() {
  // Temps ecoule depuis le demarrage : s'il repart de zero d'une capture a
  // l'autre, c'est que l'ouverture du port serie redemarre la carte — et alors
  // l'enregistrement min/max est efface a chaque fois, donc inutilisable.
  Serial.print(F("=== diag_ecran === (allume depuis "));
  Serial.print(millis() / 1000);
  Serial.println(F(" s)"));

  // Etat des lignes au repos : tirees haut par l'ecran, ou mortes ?
  pinMode(SCL_PIN, INPUT); pinMode(SDA_PIN, INPUT);
  delayMicroseconds(50);
  Serial.print(F("Lignes au repos, sans pull-up : SCL="));
  Serial.print(digitalRead(SCL_PIN) ? F("HAUT") : F("bas"));
  Serial.print(F("  SDA="));
  Serial.println(digitalRead(SDA_PIN) ? F("HAUT") : F("bas"));
  Serial.println(F("  (HAUT = les resistances de tirage de l'ecran sont alimentees)"));

  Serial.print(F("Reponse de l'ecran : 0x3C = "));
  Serial.print(sonde(0x3C) ? F("OUI") : F("non"));
  Serial.print(F("   0x3D = "));
  Serial.println(sonde(0x3D) ? F("OUI") : F("non"));

  // Le plan de masse du shield est pose sur la broche 11 du Mega (defaut v1.2).
  // On la LIT seulement : la mettre en sortie a l'etat haut detruirait la broche.
  pinMode(11, INPUT);
  delayMicroseconds(50);
  int plan = digitalRead(11);
  Serial.print(F("Plan de masse du shield (broche 11, lecture seule) : "));
  Serial.println(plan ? F("HAUT => LA MASSE NE REJOINT PAS CELLE DU MEGA (cavalier 3->6 de JMP1 ?)")
                      : F("bas => masse du shield reliee, correct"));

  // Enregistrement du MINIMUM et du MAXIMUM vus depuis le demarrage : la trace
  // d'un balayage reste, meme si la capture serie n'etait pas ouverte au moment
  // ou la main tournait le bouton. Evite de conclure sur un releve pris pendant
  // que rien ne bougeait.
  static int mini[16], maxi16[16];
  static bool init16 = false;
  if (!init16) { for (uint8_t i = 0; i < 16; i++) { mini[i] = 1023; maxi16[i] = 0; } init16 = true; }

  Serial.print(F("Potentiometres (A0-A15) :"));
  int maxi = 0;
  for (uint8_t i = 0; i < 16; i++) {
    int v = analogRead(A0 + i);
    if (v > maxi) maxi = v;
    if (v < mini[i]) mini[i] = v;
    if (v > maxi16[i]) maxi16[i] = v;
    Serial.print(' '); Serial.print(v);
  }
  Serial.println();
  Serial.print(F("Amplitude vue depuis le demarrage :"));
  int bouge = 0;
  for (uint8_t i = 0; i < 16; i++) {
    int a = maxi16[i] - mini[i];
    if (a > 100) bouge++;
    Serial.print(' '); Serial.print(a);
  }
  Serial.println();
  Serial.print(F("  => "));
  Serial.println(bouge ? F("des potentiometres ONT BOUGE : ils sont bien relies")
                       : F("aucun mouvement enregistre : tourne un bouton, la trace restera"));
  Serial.println(maxi > 40 ? F("  => le shield est ALIMENTE (des curseurs sont a une tension non nulle)")
                           : F("  => tout est a zero : shield NON alimente, ou mal emboite"));
  Serial.println();
  delay(3000);
}
