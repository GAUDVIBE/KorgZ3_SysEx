// diag_mux — croquis de diagnostic du multiplexeur, APRES la reparation
// d'alimentation du 10/08/2026 (voir hardware/CORRESPONDANCE_BROCHES.md).
//
// L'ordre des broches est inverse dans chaque bloc d'embases du shield : les
// numeros ci-dessous sont donc les broches REELLES du Mega, pas celles du
// schema du shield.
//
//   role          net du shield   broche Mega reelle
//   S0            D2              5
//   S1            D3              4
//   S2            D4              3
//   S3            D13             12
//   COM           A15             A8
//
// Canaux cables : 0 = potentiometre #16, 1 = molette de pitch, 2 = switch de
// plage de bend. Les canaux 3-15 ne sont relies a rien.
//
// SECURITE : ce croquis ne configure et ne pilote AUCUNE autre broche. En
// particulier la broche 11 du Mega, sur laquelle repose le plan de masse du
// shield, n'est jamais touchee — la mettre en sortie a l'etat haut la
// detruirait.

const uint8_t MUX_SEL[4] = {5, 4, 3, 12};  // S0, S1, S2, S3
const uint8_t MUX_COM    = A8;

int muxRead(uint8_t channel) {
  for (uint8_t bit = 0; bit < 4; bit++) {
    digitalWrite(MUX_SEL[bit], (channel >> bit) & 1 ? HIGH : LOW);
  }
  delayMicroseconds(50);   // etablissement du canal
  analogRead(MUX_COM);     // lecture blanche : purge la capacite du S/H de l'ADC
  delayMicroseconds(50);
  return analogRead(MUX_COM);
}

void setup() {
  Serial.begin(115200);
  for (uint8_t i = 0; i < 4; i++) {
    pinMode(MUX_SEL[i], OUTPUT);
    digitalWrite(MUX_SEL[i], LOW);
  }
  Serial.println(F("diag_mux — canal0=pot16  canal1=molette  canal2=switch"));
  Serial.println(F("Les trois valeurs doivent varier INDEPENDAMMENT."));
}

void loop() {
  int pot16  = muxRead(0);
  int wheel  = muxRead(1);
  int sw     = muxRead(2);
  int spare  = muxRead(7);   // temoin : canal non cable, doit rester incoherent

  Serial.print(F("pot16=")); Serial.print(pot16);
  Serial.print(F("\twheel=")); Serial.print(wheel);
  Serial.print(F("\tsw=")); Serial.print(sw);
  Serial.print(F("\t(libre=")); Serial.print(spare); Serial.println(')');

  delay(300);
}
