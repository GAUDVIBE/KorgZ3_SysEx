// diag_midi_in — croquis de diagnostic de la CHAINE MIDI IN, carte v1.2.
//
// But : separer une panne ELECTRIQUE (rien n'arrive sur la broche) d'une panne
// LOGICIELLE (les octets arrivent mais le firmware complet les perd). Le
// firmware complet bloque jusqu'a 144 ms dans le rafraichissement de l'ecran et
// 223 ms dans sendSysEx() ; or le tampon de reception de SoftwareSerial ne fait
// que 64 octets, soit 20 ms de flux MIDI, alors qu'un dump du Z3 en fait 95.
// Ce croquis-ci ne fait RIEN d'autre qu'ecouter : aucun ecran, aucun
// potentiometre, aucun blocage. Si un dump passe ici et pas dans le firmware,
// la cause est le blocage ; s'il ne passe pas ici non plus, elle est en amont.
//
//   role                     net du shield   broche Mega reelle
//   MIDI OUT (TX)            D1_TX           6
//   MIDI IN  (RX)            —               53  <- FIL VOLANT depuis la patte
//                                                 "signal" de R5 (sortie Vo du
//                                                 6N138). Sans ce fil, rien ne
//                                                 peut arriver : le net D0_RX
//                                                 du shield tombe sur la broche
//                                                 7 du Mega, qui n'est ni un
//                                                 UART ni une broche a
//                                                 interruption de changement
//                                                 d'etat.
//
// SECURITE : ce croquis ne pilote AUCUNE autre broche (la 7 est seulement lue,
// en entree, comme au demarrage). En
// particulier la broche 11 du Mega, sur laquelle repose le plan de masse du
// shield, n'est jamais touchee — la mettre en sortie a l'etat haut la
// detruirait.
//
// Usage : arduino-cli monitor -p /dev/cu.usbmodemXXXX -c baudrate=115200
//   d = envoyer un Dump Request        a = scruter 1 s l'activite de la broche
//   r = remettre les compteurs a zero
//   b = test de BOUCLE (cable MIDI de J2 vers J1) : verifie toute l'entree,
//       sens des fils de J1 compris, sans le Z3 et sans multimetre

#include <SoftwareSerial.h>

const uint8_t MIDI_RX_PIN = 53;
const uint8_t MIDI_TX_PIN = 6;

SoftwareSerial MIDI_PORT(MIDI_RX_PIN, MIDI_TX_PIN);

const byte DUMP_REQUEST[] = { 0xF0, 0x42, 0x30, 0x1D, 0x10, 0xF7 };
const byte DUMP_HEADER[]  = { 0xF0, 0x42, 0x30, 0x1D, 0x40 };

unsigned long totalOctets = 0;
byte  msg[128];
int   msgLen   = 0;
bool  inSysEx  = false;
unsigned long dernierOctet = 0;

void imprimeHex(byte b) {
  if (b < 0x10) Serial.print('0');
  Serial.print(b, HEX);
  Serial.print(' ');
}

// Scrute la broche en direct, sans passer par le decodage serie : detecte une
// activite electrique meme si la vitesse ou le format sont faux.
void scruteActivite(unsigned long duree_ms) {
  Serial.print(F("Scrutation de la broche 53 pendant "));
  Serial.print(duree_ms);
  Serial.println(F(" ms — joue sur le Z3 pendant ce temps."));
  unsigned long t0 = millis();
  unsigned long fronts = 0;
  int precedent = digitalRead(MIDI_RX_PIN);
  bool vuBas = (precedent == LOW);
  while (millis() - t0 < duree_ms) {
    int niveau = digitalRead(MIDI_RX_PIN);
    if (niveau != precedent) { fronts++; precedent = niveau; }
    if (niveau == LOW) vuBas = true;
  }
  Serial.print(F("  fronts = ")); Serial.print(fronts);
  Serial.print(F("   niveau bas vu = ")); Serial.println(vuBas ? F("OUI") : F("non"));
  if (fronts == 0 && !vuBas)
    Serial.println(F("  -> ligne au repos (haut). Soit rien n'entre, soit le fil R5->53 est absent."));
  else if (fronts == 0 && vuBas)
    Serial.println(F("  -> ligne collee en BAS : optocoupleur passant en permanence, ou court-circuit."));
  else
    Serial.println(F("  -> il y a bien du signal sur la broche."));
}

// Test de boucle, en courant continu : un cable MIDI relie J2 (OUT) a J1 (IN).
// TX a l'etat bas = courant dans la boucle (+5V -> R2 -> cable -> LED de l'opto
// -> cable -> R3 -> broche 6) : la sortie de l'opto doit tirer la broche 53 au
// niveau bas. TX au repos (haut) = pas de courant : 53 doit remonter.
// Statique, donc insensible aux delais de l'opto. SoftwareSerial ne sait pas
// ecouter pendant qu'il emet : un echo d'octets reels ne marcherait pas ici.
// La sortie de l'opto (net D0_RX du shield) atteint AUSSI la broche 7 du Mega
// par la piste d'origine (carte v1.2 : megaDigital(0) == 7). La lire en plus de
// la 53 separe le fil volant R5->53 de tout ce qui est en amont (J1, opto).
// Lecture seule, sans pull-up : R5 tire deja la ligne au +5V.
const uint8_t OPTO_PIN_PISTE = 7;

void testBoucle() {
  Serial.println(F("Test de boucle : un cable MIDI doit relier J2 (OUT) a J1 (IN)."));
  MIDI_PORT.end();                        // coupe la reception : pas d'octets parasites
  pinMode(MIDI_TX_PIN, OUTPUT);
  pinMode(OPTO_PIN_PISTE, INPUT);
  uint8_t bons53 = 0, bons7 = 0;
  for (uint8_t i = 0; i < 5; i++) {
    digitalWrite(MIDI_TX_PIN, LOW);
    delay(5);
    int c53 = digitalRead(MIDI_RX_PIN), c7 = digitalRead(OPTO_PIN_PISTE);
    digitalWrite(MIDI_TX_PIN, HIGH);
    delay(5);
    int r53 = digitalRead(MIDI_RX_PIN), r7 = digitalRead(OPTO_PIN_PISTE);
    Serial.print(F("  courant -> 53=")); Serial.print(c53 ? F("HAUT") : F("bas "));
    Serial.print(F(" 7="));              Serial.print(c7  ? F("HAUT") : F("bas "));
    Serial.print(F("   repos -> 53="));  Serial.print(r53 ? F("HAUT") : F("bas "));
    Serial.print(F(" 7="));              Serial.println(r7 ? F("HAUT") : F("bas "));
    if (c53 == LOW && r53 == HIGH) bons53++;
    if (c7  == LOW && r7  == HIGH) bons7++;
  }
  MIDI_PORT.begin(31250);
  if (bons53 == 5)
    Serial.println(F("  => ENTREE MIDI BONNE : J1, opto et fil vers 53 fonctionnent."));
  else if (bons7 == 5)
    Serial.println(F("  => L'OPTO REPOND (broche 7) MAIS PAS LA 53 : le fil R5->53 est en cause."));
  else if (bons53 == 0 && bons7 == 0)
    Serial.println(F("  => NI 7 NI 53 : panne AVANT la sortie de l'opto (fils de J1, D1/R1, 6N138)."));
  else
    Serial.println(F("  => RESULTAT INSTABLE : faux contact probable."));
}

// Deux mesures de CONTINUITE faites par le Mega lui-meme, sans multimetre.
//
// (1) Les points d'observation 7 et 53 sont-ils bien sur la sortie de l'opto ?
//     On tire la 7 a l'etat bas (sans danger : la sortie de l'opto est a
//     collecteur ouvert et R5 limite le courant a 0,5 mA) et on lit la 53 sans
//     pull-up. Si le fil R5->53 est sur la sortie de l'opto, la 53 suit.
//
// (2) La boucle J2 -> cable -> J1 -> R1 -> LED de l'opto -> cable -> R3 est-elle
//     fermee ? On decharge la broche 6, on la laisse flotter 50 us et on la lit.
//     Boucle fermee : le +5V remonte par R2, le cable et la LED -> HAUT.
//     Boucle ouverte : la capacite de la broche garde la charge nulle -> bas.
void testBoucleFermee();

void testContinuite() {
  MIDI_PORT.end();

  Serial.println(F("(1) Liaison sortie opto -> broches 7 et 53 :"));
  pinMode(MIDI_RX_PIN, INPUT);            // 53 sans pull-up
  pinMode(OPTO_PIN_PISTE, OUTPUT);
  digitalWrite(OPTO_PIN_PISTE, LOW);
  delay(1);
  int r53bas = digitalRead(MIDI_RX_PIN);
  pinMode(OPTO_PIN_PISTE, INPUT);
  delay(1);
  int r53haut = digitalRead(MIDI_RX_PIN);
  int r7haut  = digitalRead(OPTO_PIN_PISTE);
  Serial.print(F("  7 tiree bas -> 53 = ")); Serial.println(r53bas ? F("HAUT") : F("bas"));
  Serial.print(F("  7 relachee  -> 53 = ")); Serial.print(r53haut ? F("HAUT") : F("bas"));
  Serial.print(F(", 7 = "));                 Serial.println(r7haut ? F("HAUT") : F("bas"));
  if (r53bas == LOW && r53haut == HIGH && r7haut == HIGH)
    Serial.println(F("  => OK : 7 et 53 sont sur la sortie de l'opto, tiree par R5."));
  else
    Serial.println(F("  => ANOMALIE : 7 et 53 ne sont pas sur le meme fil, ou R5 ne tire pas."));

  testBoucleFermee();
  MIDI_PORT.begin(31250);
}

// Mesure (2) seule : ne touche QUE la broche 6 (sortie MIDI). Sans risque quel
// que soit l'etat du fil volant R5->53, contrairement a la mesure (1) qui tire
// la broche 7 a l'etat bas.
void testBoucleFermee() {
  MIDI_PORT.end();
  Serial.println(F("(2) Boucle J2 -> cable -> J1 -> LED opto (cable J2->J1 requis) :"));
  uint8_t fermee = 0;
  for (uint8_t i = 0; i < 5; i++) {
    pinMode(MIDI_TX_PIN, OUTPUT);
    digitalWrite(MIDI_TX_PIN, LOW);       // decharge
    delay(1);
    pinMode(MIDI_TX_PIN, INPUT);          // flottante, sans pull-up
    delayMicroseconds(50);
    if (digitalRead(MIDI_TX_PIN) == HIGH) fermee++;
  }
  pinMode(MIDI_TX_PIN, OUTPUT);
  digitalWrite(MIDI_TX_PIN, HIGH);        // retour au repos MIDI
  Serial.print(F("  broche 6 remontee "));  Serial.print(fermee); Serial.println(F("/5"));
  if (fermee == 5)
    Serial.println(F("  => BOUCLE FERMEE : le courant passe par J1 (dans le sens de la LED ou par un court-circuit)."));
  else if (fermee == 0)
    Serial.println(F("  => BOUCLE OUVERTE : coupure entre J2, le cable, J1, R1, la LED ou D1."));
  else
    Serial.println(F("  => INSTABLE : faux contact probable."));

  MIDI_PORT.begin(31250);
}

// Ou arrive REELLEMENT le fil volant parti de R5 ? Le bloc 2x18 (22-53) n'est
// pas cable sur la carte v1.2 : on peut y mettre toutes les broches en entree
// avec pull-up sans risque. On tire ensuite la sortie de l'opto a l'etat bas
// par la broche 7 : la broche du bloc qui descend avec elle porte le fil.
void chercheFil() {
  MIDI_PORT.end();
  for (uint8_t p = 22; p <= 53; p++) pinMode(p, INPUT_PULLUP);
  delay(2);
  Serial.print(F("Deja basses avant le test (reliees a la masse ?) :"));
  for (uint8_t p = 22; p <= 53; p++) if (digitalRead(p) == LOW) { Serial.print(' '); Serial.print(p); }
  Serial.println();
  pinMode(OPTO_PIN_PISTE, OUTPUT);
  digitalWrite(OPTO_PIN_PISTE, LOW);
  delay(2);
  Serial.print(F("Descendues avec la sortie de l'opto (= le fil) :"));
  uint8_t n = 0;
  for (uint8_t p = 22; p <= 53; p++) if (digitalRead(p) == LOW) { Serial.print(' '); Serial.print(p); n++; }
  Serial.println(n ? F("") : F(" AUCUNE -> le fil n'atteint aucune broche 22-53"));
  pinMode(OPTO_PIN_PISTE, INPUT);
  for (uint8_t p = 22; p <= 53; p++) pinMode(p, INPUT);
  MIDI_PORT.begin(31250);
}

// Temps de remontee d'une broche lachee apres decharge : mesure la FORCE de ce
// qui la tire vers le haut. La capacite d'une broche + piste fait ~10-50 pF :
//   ~0 tour      -> tirage fort (quelques centaines d'ohms a quelques kohms)
//   quelques     -> tirage type pull-up interne (~35 kohms)
//   60000 (max)  -> rien ne tire, la broche reste basse (flottante)
// Un tour de boucle ~ 0,5 us. Comparer a une broche libre (30) et au pull-up
// interne de cette meme broche 30, qui servent d'etalons.
uint16_t tempsMontee(uint8_t pin, bool pullup) {
  volatile uint8_t *ddr = portModeRegister(digitalPinToPort(pin));
  volatile uint8_t *out = portOutputRegister(digitalPinToPort(pin));
  volatile uint8_t *in  = portInputRegister(digitalPinToPort(pin));
  uint8_t m = digitalPinToBitMask(pin);
  *out &= ~m; *ddr |= m;                  // sortie a l'etat bas : decharge
  delayMicroseconds(20);                  // court : si la broche etait reliee au
                                          // +5V, on ne la met pas en court long
  noInterrupts();
  *ddr &= ~m;                             // entree
  if (pullup) *out |= m;                  // pull-up interne (etalon)
  uint16_t n = 0;
  while (!(*in & m) && n < 60000) n++;
  interrupts();
  *out &= ~m;                             // entree sans pull-up
  return n;
}

// Une broche qui remonte vite peut etre tiree par une resistance (R5 = 10 k,
// sortie de l'opto) ou COLLEE au +5V par un fil. Pour les distinguer : on force
// la broche a l'etat bas et on la relit. Le transistor de sortie de l'AVR (~25
// ohms) gagne contre 10 k -> on lit "bas". Contre un fil au +5V, il perd -> on
// lit "HAUT". Duree 20 us : assez pour lire, trop court pour abimer la broche.
const char* natureBroche(uint8_t pin) {
  volatile uint8_t *ddr = portModeRegister(digitalPinToPort(pin));
  volatile uint8_t *out = portOutputRegister(digitalPinToPort(pin));
  volatile uint8_t *in  = portInputRegister(digitalPinToPort(pin));
  uint8_t m = digitalPinToBitMask(pin);
  *out &= ~m; *ddr |= m;
  delayMicroseconds(20);
  uint8_t lu = (*in & m);
  *ddr &= ~m;
  return lu ? "COLLEE au +5V (fil direct)" : "tiree par une resistance (10k = sortie opto)";
}

// Les broches 7 et 53 sont-elles le MEME fil ? On tire l'une, on lit l'autre.
void testNature() {
  MIDI_PORT.end();
  Serial.print(F("Broche 7  : "));  Serial.println(natureBroche(OPTO_PIN_PISTE));
  Serial.print(F("Broche 53 : "));  Serial.println(natureBroche(MIDI_RX_PIN));
  Serial.print(F("Broche 6  : "));  Serial.println(natureBroche(MIDI_TX_PIN));
  // 53 tiree bas, lecture de 7
  pinMode(OPTO_PIN_PISTE, INPUT);
  pinMode(MIDI_RX_PIN, OUTPUT); digitalWrite(MIDI_RX_PIN, LOW);
  delayMicroseconds(50);
  int lu7 = digitalRead(OPTO_PIN_PISTE);
  pinMode(MIDI_RX_PIN, INPUT);
  Serial.print(F("53 tiree bas -> 7 = ")); Serial.println(lu7 ? F("HAUT (fils differents)") : F("bas (meme fil)"));
  pinMode(MIDI_TX_PIN, OUTPUT); digitalWrite(MIDI_TX_PIN, HIGH);
  MIDI_PORT.begin(31250);
}

// Mesure FINE de la remontee de la broche 6 : 16 lectures consecutives du port,
// une toutes les ~125 ns, au lieu d'une boucle a 500 ns. But : distinguer le
// chemin du signal MIDI (R2 + R1 + LED de l'opto + R3, soit ~660 ohms, remontee
// en quelques dizaines de ns) d'un tirage bien plus faible. En comparant cable
// de boucle branche puis debranche, on sait si le courant traverse vraiment J1.
void testMonteeFine() {
  MIDI_PORT.end();
  volatile uint8_t *ddr = portModeRegister(digitalPinToPort(MIDI_TX_PIN));
  volatile uint8_t *out = portOutputRegister(digitalPinToPort(MIDI_TX_PIN));
  volatile uint8_t *in  = portInputRegister(digitalPinToPort(MIDI_TX_PIN));
  uint8_t m = digitalPinToBitMask(MIDI_TX_PIN);
  uint8_t e[16];
  *out &= ~m; *ddr |= m;                   // decharge
  delayMicroseconds(50);
  noInterrupts();
  *ddr &= ~m;                              // laissee libre
  e[0]=*in;  e[1]=*in;  e[2]=*in;  e[3]=*in;
  e[4]=*in;  e[5]=*in;  e[6]=*in;  e[7]=*in;
  e[8]=*in;  e[9]=*in;  e[10]=*in; e[11]=*in;
  e[12]=*in; e[13]=*in; e[14]=*in; e[15]=*in;
  interrupts();
  Serial.print(F("Remontee fine broche 6 (1 = haut, un cran = ~125 ns) : "));
  int premier = -1;
  for (uint8_t i = 0; i < 16; i++) {
    uint8_t h = (e[i] & m) ? 1 : 0;
    Serial.print(h);
    if (h && premier < 0) premier = i;
  }
  Serial.println();
  if (premier < 0)      Serial.println(F("  => toujours bas apres 2 us : tirage tres faible ou inexistant"));
  else if (premier <= 1) Serial.println(F("  => remontee IMMEDIATE : chemin de faible resistance (le courant passe par J1)"));
  else { Serial.print(F("  => remontee apres ")); Serial.print(premier * 125); Serial.println(F(" ns environ")); }
  pinMode(MIDI_TX_PIN, OUTPUT);
  digitalWrite(MIDI_TX_PIN, HIGH);
  MIDI_PORT.begin(31250);
}

void testMontee() {
  MIDI_PORT.end();
  Serial.print(F("Remontee broche 6 (sortie MIDI)      : ")); Serial.println(tempsMontee(MIDI_TX_PIN, false));
  Serial.print(F("Remontee broche 7 (sortie opto, R5)  : ")); Serial.println(tempsMontee(OPTO_PIN_PISTE, false));
  Serial.print(F("Remontee broche 53 (fil volant)      : ")); Serial.println(tempsMontee(MIDI_RX_PIN, false));
  Serial.print(F("Etalon broche 30 libre, sans pull-up  : ")); Serial.println(tempsMontee(30, false));
  Serial.print(F("Etalon broche 30, pull-up interne 35k : ")); Serial.println(tempsMontee(30, true));
  pinMode(MIDI_TX_PIN, OUTPUT);
  digitalWrite(MIDI_TX_PIN, HIGH);        // repos MIDI
  MIDI_PORT.begin(31250);
}

void envoieDumpRequest() {
  Serial.print(F("TX> "));
  for (uint8_t i = 0; i < sizeof(DUMP_REQUEST); i++) {
    MIDI_PORT.write(DUMP_REQUEST[i]);
    imprimeHex(DUMP_REQUEST[i]);
    delay(2);
  }
  Serial.println(F("  (Dump Request envoye)"));
}

// La requete s'adresse au Z3 par l'octet 0x3n, ou n est le canal MIDI moins 1.
// Si le Z3 n'est pas sur le canal 1, il ignore la demande sans rien dire. On
// essaie donc les 16 canaux, en laissant a chaque fois le temps de repondre.
void baladeCanaux() {
  Serial.println(F("Essai des 16 canaux MIDI (0x30 a 0x3F)..."));
  for (uint8_t n = 0; n < 16; n++) {
    unsigned long avant = totalOctets;
    byte req[] = { 0xF0, 0x42, (byte)(0x30 + n), 0x1D, 0x10, 0xF7 };
    for (uint8_t i = 0; i < sizeof(req); i++) { MIDI_PORT.write(req[i]); delay(2); }
    unsigned long t0 = millis();
    while (millis() - t0 < 400) {              // fenetre d'ecoute
      while (MIDI_PORT.available()) {
        byte b = MIDI_PORT.read();
        totalOctets++;
        if (b == 0xF0) Serial.print(F("\n  RX< "));
        imprimeHex(b);
      }
    }
    if (totalOctets != avant) {
      Serial.print(F("\n  *** REPONSE sur le canal "));
      Serial.print(n + 1);
      Serial.print(F(" ("));
      Serial.print(totalOctets - avant);
      Serial.println(F(" octets) ***"));
    }
  }
  Serial.println(F("Balade terminee."));
}

void finDeMessage() {
  Serial.print(F("  <- message de "));
  Serial.print(msgLen);
  Serial.println(F(" octets"));
  bool entete = (msgLen >= (int)sizeof(DUMP_HEADER));
  for (uint8_t i = 0; entete && i < sizeof(DUMP_HEADER); i++)
    if (msg[i] != DUMP_HEADER[i]) entete = false;
  if (entete && msgLen == 95)
    Serial.println(F("  *** DUMP Z3 COMPLET ET CONFORME (95 octets) ***"));
  else if (entete)
    Serial.println(F("  !!! entete de dump correcte mais LONGUEUR anormale (95 attendus) — octets perdus"));
  else
    Serial.println(F("  (ce n'est pas un dump du Z3)"));
  msgLen = 0;
}

void setup() {
  Serial.begin(115200);
  MIDI_PORT.begin(31250);
  delay(200);
  Serial.println(F("=== diag_midi_in — carte v1.2 ==="));
  Serial.println(F("RX = broche 53 (fil volant depuis R5/Vo), TX = broche 6"));
  Serial.println(F("d = dump request | a = scruter la broche | b = test de boucle J2->J1 | r = raz"));
  scruteActivite(300);
}

void loop() {
  while (Serial.available()) {
    char c = Serial.read();
    if (c == 'd') envoieDumpRequest();
    else if (c == 'a') scruteActivite(1000);
    else if (c == 'b') testBoucle();
    else if (c == 'c') testContinuite();
    else if (c == 's') chercheFil();
    else if (c == 'l') testBoucleFermee();
    else if (c == 't') testMontee();
    else if (c == 'v') testNature();
    else if (c == 'k') baladeCanaux();
    else if (c == 'f') testMonteeFine();
    else if (c == 'r') { totalOctets = 0; msgLen = 0; inSysEx = false;
                         Serial.println(F("compteurs remis a zero")); }
  }

  while (MIDI_PORT.available()) {
    byte b = MIDI_PORT.read();
    totalOctets++;
    dernierOctet = millis();
    if (b == 0xF0) { inSysEx = true; msgLen = 0; Serial.print(F("RX< ")); }
    imprimeHex(b);
    if (inSysEx && msgLen < (int)sizeof(msg)) msg[msgLen++] = b;
    if (b == 0xF7 && inSysEx) { inSysEx = false; Serial.println(); finDeMessage(); }
    if (!inSysEx && (totalOctets % 16) == 0) Serial.println();
  }

  if (MIDI_PORT.overflow()) {
    Serial.println(F("!!! DEBORDEMENT du tampon SoftwareSerial (64 octets) !!!"));
  }

  static unsigned long dernierBilan = 0;
  if (millis() - dernierBilan > 5000) {
    dernierBilan = millis();
    Serial.print(F("[bilan] octets recus depuis le demarrage : "));
    Serial.println(totalOctets);
  }
}
