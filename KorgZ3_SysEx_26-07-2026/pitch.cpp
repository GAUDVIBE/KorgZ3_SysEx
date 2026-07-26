#include "pitch.h"
#include "mux.h"
#include "pitch_math.h"

static const byte CH_WHEEL  = 1;
static const byte CH_SWITCH = 2;

static const byte MIDI_CHANNEL = 0;   // 0 = canal MIDI 1, Basic Channel du Z3

// Reglages repris tels quels du test valide en production.
static const int DEADZONE_IN  = 40;
static const int DEADZONE_OUT = 55;
static const unsigned long CENTER_SETTLE_MS = 80;
static const unsigned long INTERVAL_MS      = 20;   // ~50 Hz
static const int MIN_DELTA = 8;

// Un ON-ON-ON ouvre brievement le contact pendant la bascule : la ligne part
// en l'air et le pull-down la tire vers 0 V, ce qui ressemble a un
// debranchement. On confirme donc tout changement sur cette duree.
static const unsigned long SWITCH_CONFIRM_MS = 300;
static const unsigned long SWITCH_POLL_MS    = 20;

static int  centerRaw   = 512;
static int  filteredRaw = 512;
static int  lastValue14 = 8192;
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
  Serial.write((byte)(0xE0 | (MIDI_CHANNEL & 0x0F)));
  Serial.write((byte)(value14 & 0x7F));
  Serial.write((byte)((value14 >> 7) & 0x7F));
}

// Mesure la position de repos de la molette. Appelee a chaque branchement,
// jamais au demarrage : tant que le cable est absent, il n'y a rien a mesurer.
static void calibrate() {
  long sum = 0;
  for (int i = 0; i < 16; i++) { sum += muxRead(CH_WHEEL); delay(2); }
  centerRaw   = (int)(sum / 16);
  filteredRaw = centerRaw;
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

  int raw = muxRead(CH_WHEEL);
  filteredRaw = (filteredRaw * 3 + raw) / 4;

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

  bool atCenterNow    = (value14 == 8192);
  bool wasAtCenter    = (lastValue14 == 8192);
  bool bigEnoughDelta = (abs(value14 - lastValue14) >= MIN_DELTA);

  if (forceSend || bigEnoughDelta || (atCenterNow && !wasAtCenter)) {
    sendBend(value14);
    lastValue14 = value14;
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
