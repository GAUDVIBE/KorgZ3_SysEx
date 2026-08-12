#include "pitch.h"
#include "midi_port.h"
#include "mux.h"
#include "pitch_math.h"

static const byte CH_WHEEL  = 1;

// Sens de la molette. Sur la Stratocaster, le +5 V et la masse aboutissent aux
// extremites de la piste dans l'ordre inverse du sens de jeu : pousser vers le
// haut fait DESCENDRE la lecture, donc le pitch. On retablit ici plutot que de
// rouvrir le pickguard et d'echanger deux fils. Repasser a false le jour ou le
// cablage serait corrige cote guitare.
static const bool MOLETTE_INVERSEE = true;

// Lecture de la molette dans le sens de jeu. Le centre etant calibre sur cette
// meme echelle, zones mortes et seuils restent valables tels quels.
static int lireMolette() {
  int v = muxRead(CH_WHEEL);
  return MOLETTE_INVERSEE ? (1023 - v) : v;
}
static const byte CH_SWITCH = 2;

static const byte MIDI_CHANNEL = 0;   // 0 = canal MIDI 1, Basic Channel du Z3

// Reglages de reactivite (revus le 10/08/2026 : la molette trainait).
// La cellule RC materielle du canal filtre desormais le bruit en amont, le
// logiciel n'a plus a le faire, et ces valeurs peuvent etre bien plus vives.
//
// Zone morte : exprimee en unites ADC sur 1024. La course utile de la molette
// vaut environ +/-450 unites, donc 20 font 4 % de la demi-course — assez pour
// qu'elle se taise au repos, assez peu pour qu'elle reponde immediatement.
static const int DEADZONE_IN  = 8;
static const int DEADZONE_OUT = 12;
// Duree pendant laquelle il faut rester au centre avant de figer la valeur.
static const unsigned long CENTER_SETTLE_MS = 40;
// Cadence de lecture et d'emission. 8 ms = 125 Hz : trois octets de pitch bend
// occupent ~1 ms a 31250 bauds, donc environ 12 % de la bande passante MIDI.
static const unsigned long INTERVAL_MS      = 12;
// Seuil d'emission, exprime en UNITES ADC et non en valeurs MIDI. En mode
// octave une seule unite ADC vaut ~18 valeurs de pitch bend : un seuil de 8
// valeurs MIDI etait donc franchi par le simple bruit de +/-1 LSB de l'ADC, et
// la carte emettait en continu meme molette immobile — ce qui saturait le Z3.
// Trois unites ADC valent 0,7 % de la demi-course : inaudible, mais au-dessus
// du bruit.
static const int RAW_MIN_DELTA = 3;

// Un ON-ON-ON ouvre brievement le contact pendant la bascule : la ligne part
// en l'air et le pull-down la tire vers 0 V, ce qui ressemble a un
// debranchement. On confirme donc tout changement sur cette duree.
static const unsigned long SWITCH_CONFIRM_MS = 300;
static const unsigned long SWITCH_POLL_MS    = 20;

static int  centerRaw   = 512;
static int  filteredRaw = 512;
static int  lastValue14 = 8192;
static int  lastSentRaw = -1000;  // position brute du dernier envoi
static bool atCenter    = true;
static unsigned long inZoneSince  = 0;
static unsigned long lastSendTime = 0;

static bool connected = false;
static byte position  = 0;            // 0 = rien branche, 1..3 = position
static byte pendingWindow = PITCH_WINDOW_INVALID;
static unsigned long pendingSince   = 0;
static unsigned long lastSwitchPoll = 0;
static bool forceSend = false;
static PitchEvent pendingEvent = PITCH_EVENT_NONE;

// Vrai des qu'une lecture a 0 V apparait alors qu'on etait connecte : on
// suspecte un debranchement sans l'avoir encore confirme. On gele la sortie
// pendant ce doute plutot que de continuer a emettre une position qui n'est
// peut-etre plus reelle.
static bool suspectUnplugged = false;

static void sendBend(int value14) {
  if (value14 < 0)     value14 = 0;
  if (value14 > 16383) value14 = 16383;
  MIDI_PORT.write((byte)(0xE0 | (MIDI_CHANNEL & 0x0F)));
  MIDI_PORT.write((byte)(value14 & 0x7F));
  MIDI_PORT.write((byte)((value14 >> 7) & 0x7F));
}

// Mesure la position de repos de la molette. Appelee a chaque branchement,
// jamais au demarrage : tant que le cable est absent, il n'y a rien a mesurer.
static void calibrate() {
  // La molette doit etre RELACHEE pendant cette mesure. Si elle bouge — doigt
  // encore dessus au branchement — le centre est faux et la carte emet un bend
  // permanent au repos. On exige donc une serie stable, et on recommence
  // jusqu'a l'obtenir (une seconde au plus, sinon on prend la derniere serie).
  long sum = 0;
  for (int essai = 0; essai < 8; essai++) {
    int mini = 1023, maxi = 0;
    sum = 0;
    for (int i = 0; i < 16; i++) {
      int v = lireMolette();
      if (v < mini) mini = v;
      if (v > maxi) maxi = v;
      sum += v;
      delay(2);
    }
    if (maxi - mini <= 4) break;   // serie stable : la molette est au repos
  }
  centerRaw   = (int)(sum / 16);
  filteredRaw = centerRaw;
  lastSentRaw = centerRaw;
  atCenter    = true;
  inZoneSince = 0;
  lastValue14 = 8192;
}

static void pollSwitch() {
  if (millis() - lastSwitchPoll < SWITCH_POLL_MS) return;
  lastSwitchPoll = millis();

  unsigned char w = switchWindow(muxRead(CH_SWITCH));

  if (connected) suspectUnplugged = (w == 0);

  // Entre deux fenetres : bascule en cours, on ne conclut rien.
  if (w == PITCH_WINDOW_INVALID) {
    pendingWindow = PITCH_WINDOW_INVALID;
    pendingSince  = 0;
    return;
  }

  byte current = connected ? position : 0;
  if (w == current) {
    pendingWindow = PITCH_WINDOW_INVALID;
    pendingSince  = 0;
    return;
  }

  if (w != pendingWindow) {
    pendingWindow = w;
    pendingSince  = millis();
    return;
  }
  if (millis() - pendingSince < SWITCH_CONFIRM_MS) return;

  // Changement confirme sur SWITCH_CONFIRM_MS de lectures coherentes.
  pendingWindow = PITCH_WINDOW_INVALID;
  pendingSince  = 0;

  if (w == 0) {
    // Debranchement : on recentre le Z3 une fois, puis on se tait.
    connected = false;
    position  = 0;
    suspectUnplugged = false;
    sendBend(8192);
    lastValue14   = 8192;
    pendingEvent  = PITCH_EVENT_DISCONNECTED;
    return;
  }

  bool wasConnected = connected;
  position  = w;
  connected = true;
  suspectUnplugged = false;
  if (!wasConnected) {
    calibrate();
    pendingEvent = PITCH_EVENT_CONNECTED;
  } else {
    // A position physique egale, la valeur MIDI depend de la plage : il faut
    // reemettre tout de suite, sinon le Z3 reste sur l'ancienne valeur.
    pendingEvent = PITCH_EVENT_RANGE;
  }
  forceSend = true;
}

void pitchBegin() {
  connected   = false;
  position    = 0;
  centerRaw   = 512;
  filteredRaw = 512;
  lastValue14 = 8192;
  atCenter    = true;
  inZoneSince = 0;
  forceSend   = false;
  suspectUnplugged = false;
  pendingEvent   = PITCH_EVENT_NONE;
  pendingWindow  = PITCH_WINDOW_INVALID;
  pendingSince   = 0;
  lastSwitchPoll = 0;
  lastSendTime   = 0;
}

void pitchUpdate() {
  pollSwitch();
  if (!connected || suspectUnplugged) return;

  if (millis() - lastSendTime < INTERVAL_MS) return;
  lastSendTime = millis();

  int raw = lireMolette();
  // Moyenne sur deux echantillons seulement : a 125 Hz la constante de temps
  // tombe a une dizaine de millisecondes, contre pres de 200 ms avec l'ancien
  // filtre a 3/4 echantillonne a 50 Hz — c'etait la cause principale de la
  // latence ressentie.
  filteredRaw = (filteredRaw + raw) / 2;

  int distance = abs(filteredRaw - centerRaw);

  // Hysteresis : on entre au centre a DEADZONE_IN, on n'en sort qu'a
  // DEADZONE_OUT, et il faut y rester CENTER_SETTLE_MS pour figer la valeur.
  if (atCenter) {
    if (distance > DEADZONE_OUT) { atCenter = false; inZoneSince = 0; }
  } else {
    if (distance <= DEADZONE_IN) {
      if (inZoneSince == 0) inZoneSince = millis();
      if (millis() - inZoneSince >= CENTER_SETTLE_MS) atCenter = true;
    } else {
      inZoneSince = 0;
    }
  }

  int value14 = pitchValue14(filteredRaw, centerRaw, DEADZONE_OUT,
                             pitchBendSemitones(), atCenter);

  bool atCenterNow = (value14 == 8192);
  bool wasAtCenter = (lastValue14 == 8192);
  bool aBouge      = (abs(filteredRaw - lastSentRaw) >= RAW_MIN_DELTA)
                     && (value14 != lastValue14);

  // --- Recentrage automatique ---
  // Filet de securite contre un centre mal calibre : la molette est a ressort,
  // donc elle passe l'essentiel de son temps a sa position de repos. Si la
  // lecture reste immobile assez longtemps a un endroit qui n'est pas le centre
  // connu, c'est que le centre connu est faux — on l'adopte. Tenir un bend
  // parfaitement fige plusieurs secondes sur un ressort est assez improbable
  // pour que ce soit sans danger.
  {
    static int  refStable   = -1000;
    static unsigned long stableSince = 0;
    if (abs(filteredRaw - refStable) > 3) {
      refStable   = filteredRaw;
      stableSince = millis();
    } else if (millis() - stableSince >= 4000 &&
               abs(refStable - centerRaw) > DEADZONE_OUT) {
      centerRaw   = refStable;
      lastSentRaw = refStable;
      atCenter    = true;
      forceSend   = true;          // remet le Z3 d'aplomb immediatement
      stableSince = millis();
    }
  }

  if (forceSend || aBouge || (atCenterNow && !wasAtCenter)) {
    sendBend(value14);
    lastValue14 = value14;
    lastSentRaw = filteredRaw;
    forceSend   = false;
  }
}

int pitchBendSemitones() {
  switch (position) {
    case 1:  return 2;   // +/-1 ton
    case 2:  return 3;   // +/-1 ton et demi
    case 3:  return 12;  // +/-1 octave
    default: return 0;
  }
}

bool pitchConnected() { return connected; }

PitchEvent pitchTakeEvent() {
  PitchEvent e = pendingEvent;
  pendingEvent = PITCH_EVENT_NONE;
  return e;
}
