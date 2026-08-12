/**
 * KORG Z3 - Contrôleur 16 potentiomètres, 5 pages, OLED
 * Version avec Dump Request dynamique + persistance EEPROM
 *
 * Boutons :
 *  - Pin 7 (−)  : page précédente
 *  - Pin 8 (+)  : page suivante
 *  - Pin 5      : appui court → naviguer dans les dumps mémorisés
 *                 appui long ≥3s → sauvegarder le dump en cours (RAM + EEPROM)
 *  - Pin 6      : appui long ≥3s → envoyer Dump Request au Z3 et recevoir le dump
 *
 * LEDs de page : pin 12=I, pin 11=II, pin 10=III (pages IV et V sans LED)
 * LED confirmation sauvegarde : pin 9
 *
 * EEPROM layout (Arduino Mega = 4096 octets) :
 *  @0       : magic byte 0xA5
 *  @1       : presetCount
 *  @2       : currentSlot
 *  @3 + n*108 : slots (2 size + 10 name + 96 data = 108 octets/slot)
 *  Total : 3 + 8×108 = 867 octets sur 4096 disponibles.
 *
 * Bibliothèques : Wire.h, Adafruit_GFX.h, Adafruit_SSD1306.h, EEPROM.h
 */

#include <EEPROM.h>
#include "oled.h"
#include "board_map.h"
#include "midi_port.h"
#include "mux.h"
#include "pitch.h"

// D14 a D19 portent la 5e paire bouton+LED de la colonne de pages (SW6/D7) et
// les deux paires de fonction de gauche (SW7/D8 et SW8/D9). Ces broches sont
// libres sur le bloc JMD3 ; le bloc 2x18 qui les portait auparavant a ete
// supprime, sa position reelle sur le Mega traversant la colonne de
// potentiometres RV3/RV7/RV11/RV15. Voir PAGE_BTN, PAGE_LED, BTN_DUMP,
// BTN_PRESET plus bas pour leur affectation.

// =================== ÉCRAN OLED ===================
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define OLED_RESET    -1
Ecran display;   // I2C logiciel sur les broches 8/9 — voir oled.h

// =================== PRESETS ===================
const int DUMP_MAX_SIZE = 96;
const int MAX_PRESETS   = 8;

struct DumpSlot {
  byte data[DUMP_MAX_SIZE];
  int  size;
  char name[10];
  bool valid;
};

DumpSlot presetSlots[MAX_PRESETS];
int      presetCount = 0;
int      currentSlot = 0;

byte sendBuffer[DUMP_MAX_SIZE];
int  sendBufferSize = 0;
char currentName[10] = "--------";

// =================== PRESETS USINE ===================
// Places en PROGMEM : sur AVR un `const byte[]` ordinaire occupe la RAM,
// dont il ne reste qu'environ 3 Ko. La flash, elle, est utilisee a 10 %.

// Preset 0 — SynLead, cree a partir du DynoPian d'usine.
// Le nom embarque disait encore «DynoPian» : corrige, car c'est lui que le
// Z3 affiche et dont extractName() derive le nom montre sur l'ecran.
const byte SYNLEAD_DUMP[] PROGMEM = {
  0xF0, 0x42, 0x30, 0x1D, 0x40,          // en-tete SysEx
  0x53, 0x79, 0x6E, 0x4C, 0x65, 0x61, 0x64, 0x20,   // nom, 8 caracteres
  0x03, 0x02, 0x02, 0x01, 0x2F, 0x17, 0x03, 0x04, 0x02, 0x02,
  0x01, 0x00, 0x01, 0x0A, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00,
  0x00, 0x03, 0x07, 0x07, 0x03, 0x00, 0x00, 0x00, 0x00, 0x63,
  0x1C, 0x62, 0x00, 0x19, 0x18, 0x06, 0x15, 0x00, 0x00, 0x00,
  0x1F, 0x00, 0x02, 0x00, 0x00, 0x00, 0x1F, 0x00, 0x06, 0x0E,
  0x08, 0x07, 0x08, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
  0x00, 0x02, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x04, 0x06, 0x02, 0x04, 0x00, 0x00, 0x03,
  0x00,
  0xF7
};

// Preset 1 — FlutAtk : la flute douce du Z3, rendue sensible a l'attaque.
// Le Flute__1 d'origine n'avait de Velocity Int que sur C-2, une PORTEUSE :
// la velocite n'y faisait varier que le VOLUME, jamais le timbre. On l'ajoute
// sur les MODULATEURS M-1 et M-2 (valeur 8), ce qui ouvre le timbre quand on
// attaque fort — le comportement de SynLead, transpose sur un son doux.
// Six octets seulement different du dump d'origine : le nom, et ces deux la.
// Dosable en direct sur la page V, potentiometres M1Velo (A0) et M2Velo (A8).
const byte FLUTE_ATK_DUMP[] PROGMEM = {
  0xF0, 0x42, 0x30, 0x1D, 0x40,          // en-tete SysEx
  0x46, 0x6C, 0x75, 0x74, 0x41, 0x74, 0x6B, 0x20,   // nom, 8 caracteres
  0x03, 0x07, 0x02, 0x01, 0x4B, 0x20, 0x04, 0x26, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x03, 0x04, 0x02, 0x02, 0x00, 0x00, 0x00,
  0x00, 0x04, 0x04, 0x04, 0x04, 0x01, 0x00, 0x00, 0x00, 0x1B,
  0x2F, 0x24, 0x46, 0x1F, 0x1F, 0x1F, 0x10, 0x14, 0x11, 0x1F,
  0x1F, 0x05, 0x02, 0x00, 0x00, 0x13, 0x00, 0x00, 0x00, 0x09,
  0x06, 0x05, 0x0B, 0x01, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
  0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x08, 0x00, 0x08, 0x0A, 0x00, 0x04, 0x04,
  0x01,
  0xF7
};

struct FactoryPreset { const byte* data; byte size; };
const FactoryPreset FACTORY[] = {
  { SYNLEAD_DUMP,   sizeof(SYNLEAD_DUMP)   },
  { FLUTE_ATK_DUMP, sizeof(FLUTE_ATK_DUMP) }
};
const int FACTORY_COUNT = sizeof(FACTORY) / sizeof(FACTORY[0]);

// =================== EEPROM ===================
const byte EE_MAGIC        = 0xA5;
const int  EE_ADDR_MAGIC   = 0;
const int  EE_ADDR_COUNT   = 1;
const int  EE_ADDR_CURRENT = 2;
const int  EE_ADDR_SLOTS   = 3;
const int  SLOT_EE_SIZE    = 108;
// Nombre de presets usine deja installes sur cet appareil. Place bien APRES la
// zone des slots (3 + 8 x 108 = 867) pour ne pas invalider les enregistrements
// existants lors de la mise a jour du firmware. 0xFF = jamais ecrit.
const int  EE_ADDR_FACTORY = 900;

int slotEEAddr(int n) { return EE_ADDR_SLOTS + n * SLOT_EE_SIZE; }

void eeWriteByte(int addr, byte val) {
  if (EEPROM.read(addr) != val) EEPROM.write(addr, val);
}

void eeWriteSlot(int idx) {
  if (idx < 0 || idx >= MAX_PRESETS) return;
  int base = slotEEAddr(idx);
  eeWriteByte(base,     (byte)(presetSlots[idx].size & 0xFF));
  eeWriteByte(base + 1, (byte)((presetSlots[idx].size >> 8) & 0xFF));
  for (int i = 0; i < 10; i++)
    eeWriteByte(base + 2 + i, (byte)presetSlots[idx].name[i]);
  for (int i = 0; i < DUMP_MAX_SIZE; i++)
    eeWriteByte(base + 12 + i, presetSlots[idx].data[i]);
}

void eeReadSlot(int idx) {
  if (idx < 0 || idx >= MAX_PRESETS) return;
  int base = slotEEAddr(idx);
  int sz = (int)EEPROM.read(base) | ((int)EEPROM.read(base + 1) << 8);
  if (sz < 6 || sz > DUMP_MAX_SIZE) {
    presetSlots[idx].valid = false;
    presetSlots[idx].size  = 0;
    return;
  }
  presetSlots[idx].size  = sz;
  presetSlots[idx].valid = true;
  for (int i = 0; i < 10; i++)
    presetSlots[idx].name[i] = (char)EEPROM.read(base + 2 + i);
  presetSlots[idx].name[9] = '\0';
  for (int i = 0; i < DUMP_MAX_SIZE; i++)
    presetSlots[idx].data[i] = EEPROM.read(base + 12 + i);
}

void eeSaveMeta() {
  eeWriteByte(EE_ADDR_MAGIC,   EE_MAGIC);
  eeWriteByte(EE_ADDR_COUNT,   (byte)presetCount);
  eeWriteByte(EE_ADDR_CURRENT, (byte)currentSlot);
}

bool eeLoad() {
  if (EEPROM.read(EE_ADDR_MAGIC) != EE_MAGIC) return false;
  presetCount = (int)EEPROM.read(EE_ADDR_COUNT);
  currentSlot = (int)EEPROM.read(EE_ADDR_CURRENT);
  if (presetCount < 0 || presetCount > MAX_PRESETS) presetCount = 0;
  if (currentSlot < 0 || currentSlot >= presetCount) currentSlot = 0;
  for (int i = 0; i < presetCount; i++) eeReadSlot(i);
  return true;
}

// =================== DUMP REQUEST ===================
const byte DUMP_REQUEST[] = { 0xF0, 0x42, 0x30, 0x1D, 0x10, 0xF7 };

// =================== RÉCEPTION MIDI ===================
const byte DUMP_HEADER[] = { 0xF0, 0x42, 0x30, 0x1D, 0x40 };
const int  HEADER_LEN    = 5;

byte rxBuffer[DUMP_MAX_SIZE];
int  rxLen       = 0;
bool rxInSysEx   = false;
int  rxHeaderPos = 0;

// =================== STRUCTURE POT ===================
struct PotConfig {
  int  pin;
  int  position;
  int  lastValue;
  int  physValue;
  int  candidate;
  int  stableCount;
  int  minVal;
  int  maxVal;
  const char* name;
  int  filteredRaw;
  bool caught;
  int  catchOffset; // lecture ADC BRUTE au moment de l'activation de la page
};

const int NUM_POTS  = 16;
const int NUM_PAGES = 5;

// --- Prototypes ---
void updateFiltered(PotConfig &pot);
int  getQuantized(PotConfig &pot);
void initFilter(PotConfig &pot);
void writeToBuffer(PotConfig &pot, int val);

// --------- Page 0 : I — Global & Niveaux ---------
PotConfig page0[NUM_POTS] = {
  {A0,  13, -1,-1,-1,0, 0, 7,   "Algo",   0,false,0},
  {A5,  14, -1,-1,-1,0, 0, 7,   "FeedB.", 0,false,0},
  {A8,  15, -1,-1,-1,0, 0, 3,   "LFOWave",0,false,0},
  {A12, 17, -1,-1,-1,0, 0, 255, "Rate",   0,false,0},
  {A1,  18, -1,-1,-1,0, 0, 127, "PMD",    0,false,0},
  {A4,  19, -1,-1,-1,0, 0, 7,   "PMS",    0,false,0},
  {A9,  20, -1,-1,-1,0, 0, 127, "AMD",    0,false,0},
  {A13, 21, -1,-1,-1,0, 0, 3,   "AMS",    0,false,0},
  {A2,  22, -1,-1,-1,0, 0, 7,   "M1Wave", 0,false,0},
  {A6,  23, -1,-1,-1,0, 0, 7,   "C1Wave", 0,false,0},
  {A10, 24, -1,-1,-1,0, 0, 7,   "M2Wave", 0,false,0},
  {A14, 25, -1,-1,-1,0, 0, 7,   "C2Wave", 0,false,0},
  {A3,  42, -1,-1,-1,0, 0, 127, "M1Lvl",  0,false,0},
  {A7,  43, -1,-1,-1,0, 0, 127, "C1Lvl",  0,false,0},
  {A11, 44, -1,-1,-1,0, 0, 127, "M2Lvl",  0,false,0},
  {A15, 45, -1,-1,-1,0, 0, 127, "C2Lvl",  0,false,0}
};

// --------- Page 1 : II — Enveloppes Attack/Decay/Sustain ---------
PotConfig page1[NUM_POTS] = {
  {A0,  46, -1,-1,-1,0, 0, 31, "M1Att.", 0,false,0},
  {A5,  47, -1,-1,-1,0, 0, 31, "C1Att.", 0,false,0},
  {A8,  48, -1,-1,-1,0, 0, 31, "M2Att.", 0,false,0},
  {A12, 49, -1,-1,-1,0, 0, 31, "C2Att.", 0,false,0},
  {A1,  50, -1,-1,-1,0, 0, 31, "M1Decy", 0,false,0},
  {A4,  51, -1,-1,-1,0, 0, 31, "C1Decy", 0,false,0},
  {A9,  52, -1,-1,-1,0, 0, 31, "M2Decy", 0,false,0},
  {A13, 53, -1,-1,-1,0, 0, 31, "C2Decy", 0,false,0},
  {A2,  54, -1,-1,-1,0, 0, 15, "M1Sust", 0,false,0},
  {A6,  55, -1,-1,-1,0, 0, 15, "C1Sust", 0,false,0},
  {A10, 56, -1,-1,-1,0, 0, 15, "M2Sust", 0,false,0},
  {A14, 57, -1,-1,-1,0, 0, 15, "C2Sust", 0,false,0},
  {A3,  58, -1,-1,-1,0, 0, 31, "M1Dec2", 0,false,0},
  {A7,  59, -1,-1,-1,0, 0, 31, "C1Dec2", 0,false,0},
  {A11, 60, -1,-1,-1,0, 0, 31, "M2Dec2", 0,false,0},
  {A15, 61, -1,-1,-1,0, 0, 31, "C2Dec2", 0,false,0}
};

// --------- Page 2 : III — Release, EG, Detune ---------
PotConfig page2[NUM_POTS] = {
  {A0,  62, -1,-1,-1,0, 0, 15, "M1Rel.", 0,false,0},
  {A5,  63, -1,-1,-1,0, 0, 15, "C1Rel.", 0,false,0},
  {A8,  64, -1,-1,-1,0, 0, 15, "M2Rel.", 0,false,0},
  {A12, 65, -1,-1,-1,0, 0, 15, "C2Rel.", 0,false,0},
  {A1,  74, -1,-1,-1,0, 0, 3,  "M1_EG",  0,false,0},
  {A4,  75, -1,-1,-1,0, 0, 3,  "C1_EG",  0,false,0},
  {A9,  76, -1,-1,-1,0, 0, 3,  "M2_EG",  0,false,0},
  {A13, 77, -1,-1,-1,0, 0, 3,  "C2_EG",  0,false,0},
  {A2,  34, -1,-1,-1,0, 0, 7,  "M1Det1", 0,false,0},
  {A6,  35, -1,-1,-1,0, 0, 7,  "C1Det1", 0,false,0},
  {A10, 36, -1,-1,-1,0, 0, 7,  "M2Det1", 0,false,0},
  {A14, 37, -1,-1,-1,0, 0, 7,  "C2Det1", 0,false,0},
  {A3,  38, -1,-1,-1,0, 0, 3,  "M1Det2", 0,false,0},
  {A7,  39, -1,-1,-1,0, 0, 3,  "C1Det2", 0,false,0},
  {A11, 40, -1,-1,-1,0, 0, 3,  "M2Det2", 0,false,0},
  {A15, 41, -1,-1,-1,0, 0, 3,  "C2Det2", 0,false,0}
};

// --------- Page 3 : IV — Multiply, AMS, Key Scale ---------
PotConfig page3[NUM_POTS] = {
  {A0,  26, -1,-1,-1,0, 0, 15, "M1Mul1", 0,false,0},
  {A5,  27, -1,-1,-1,0, 0, 15, "C1Mul1", 0,false,0},
  {A8,  28, -1,-1,-1,0, 0, 15, "M2Mul1", 0,false,0},
  {A12, 29, -1,-1,-1,0, 0, 15, "C2Mul1", 0,false,0},
  {A1,  30, -1,-1,-1,0, 0, 15, "M1Mul2", 0,false,0},
  {A4,  31, -1,-1,-1,0, 0, 15, "C1Mul2", 0,false,0},
  {A9,  32, -1,-1,-1,0, 0, 15, "M2Mul2", 0,false,0},
  {A13, 33, -1,-1,-1,0, 0, 15, "C2Mul2", 0,false,0},
  {A3,  70, -1,-1,-1,0, 0, 1,  "M1_AMS", 0,false,0},
  {A7,  71, -1,-1,-1,0, 0, 1,  "C1_AMS", 0,false,0},
  {A11, 72, -1,-1,-1,0, 0, 1,  "M2_AMS", 0,false,0},
  {A15, 73, -1,-1,-1,0, 0, 1,  "C2_AMS", 0,false,0},
  {A2,  66, -1,-1,-1,0, 0, 3,  "M1Scal", 0,false,0},
  {A6,  67, -1,-1,-1,0, 0, 3,  "C1Scal", 0,false,0},
  {A10, 68, -1,-1,-1,0, 0, 3,  "M2Scal", 0,false,0},
  {A14, 69, -1,-1,-1,0, 0, 3,  "C2Scal", 0,false,0}
};

// --------- Page 4 : V — Velocity, Track, Reverb, Rate ---------
PotConfig page4[NUM_POTS] = {
  {A0,  86, -1,-1,-1,0, 0, 15, "M1Velo", 0,false,0},
  {A5,  87, -1,-1,-1,0, 0, 15, "C1Velo", 0,false,0},
  {A8,  88, -1,-1,-1,0, 0, 15, "M2Velo", 0,false,0},
  {A12, 89, -1,-1,-1,0, 0, 15, "C2Velo", 0,false,0},
  {A1,  90, -1,-1,-1,0, 0, 15, "M1Trck", 0,false,0},
  {A4,  91, -1,-1,-1,0, 0, 15, "C1Trck", 0,false,0},
  {A9,  92, -1,-1,-1,0, 0, 15, "M2Trck", 0,false,0},
  {A13, 93, -1,-1,-1,0, 0, 15, "C2Trck", 0,false,0},
  {A3,  78, -1,-1,-1,0, 0, 1,  "M1Revb", 0,false,0},
  {A7,  79, -1,-1,-1,0, 0, 1,  "C1Revb", 0,false,0},
  {A11, 80, -1,-1,-1,0, 0, 1,  "M2Revb", 0,false,0},
  {A15, 81, -1,-1,-1,0, 0, 1,  "C2Revb", 0,false,0},
  {A2,  82, -1,-1,-1,0, 0, 7,  "M1Rate", 0,false,0},
  {A6,  83, -1,-1,-1,0, 0, 7,  "C1Rate", 0,false,0},
  {A10, 84, -1,-1,-1,0, 0, 7,  "M2Rate", 0,false,0},
  {A14, 85, -1,-1,-1,0, 0, 7,  "C2Rate", 0,false,0}
};

PotConfig*  pages[NUM_PAGES] = { page0, page1, page2, page3, page4 };
const char* pageNames[NUM_PAGES] = { "I", "II", "III", "IV", "V" };
// =================== LEDS ===================
// COLONNE DE DROITE — 5 paires bouton+LED, de haut en bas = pages I a V.
// Chaque page a son bouton : le defilement par deux boutons etait impraticable
// en jeu, il fallait parfois quatre appuis pour atteindre la bonne page.
// La LED de la page courante reste allumee en permanence : c'est le repere.
// ATTENTION — carte v1.2 : l'ordre des broches est inverse dans chaque bloc
// d'embases (voir board_map.h). Les numeros du schema du shield seraient
// {14,7,8,6,5} et {15,12,11,10,9} ; traduits, plusieurs tombent sur des broches
// inexploitables et sont renvoyes vers des broches libres du bloc 2x18 :
//   page II  : bouton sur RX0 et page IV sur TX0 -> casseraient l'USB
//   page III : bouton sur la meme broche que la page I (D21)
//   page III/IV : LED sur une masse et sur AREF
//   page V   : LED sur la meme broche que la page I (D20)
// Consequence a l'usage : seules les pages I et V sont atteignables au bouton,
// et la LED de la page I s'allume sur deux LED a la fois. Le materiel des
// autres pages attend soit des fils volants vers le bloc 2x18, soit une v1.5.
const byte PAGE_BTN[NUM_PAGES] = {megaDigital(14), PIN_LIBRE_1,
                                  PIN_LIBRE_2,     PIN_LIBRE_3,
                                  megaDigital(5)};
const byte PAGE_LED[NUM_PAGES] = {PIN_LIBRE_4, PIN_LIBRE_5, PIN_LIBRE_6,
                                  PIN_LIBRE_7, PIN_LIBRE_8};

// COLONNE DE GAUCHE — 2 paires de fonction, toutes sur de vraies broches.
// Sur la carte v1.2 le Dump Request est inoperant (pas de MIDI IN, voir
// midi_port.h). Son bouton et sa LED sont donc reaffectes a la navigation :
//   - le bouton devient PAGE SUIVANTE, ce qui rend les cinq pages atteignables
//   - la LED devient le 3e bit du code de page
const int BTN_DUMP     = PIN_LIBRE_9;      // inerte : Dump Request sans MIDI IN
const int LED_DUMP_REQ = PIN_LIBRE_10;      // inerte
const int BTN_PRESET   = megaDigital(18);  // court = slot suivant, maintenu = sauvegarder
const int LED_SLOT     = megaDigital(19);

// Bouton de defilement : l'ancien bouton Dump, physiquement intact.
const int PAGE_NEXT_BTN = megaDigital(16);

// Affichage de la page sur les trois seules LED pilotables de la carte.
// La premiere commande deux LED a la fois (nets D9 et D15 du shield tombent sur
// la meme broche du Mega), elles s'allument ensemble : c'est sans consequence.
const byte PAGE_CODE_LED[3]     = {megaDigital(15), megaDigital(12), megaDigital(17)};
const byte PAGE_CODE[NUM_PAGES] = {0b001, 0b010, 0b100, 0b011, 0b110};

const unsigned long LED_BRIEF = 300; // duree d'allumage bref (ms)

// Le bouton de page porte deux fonctions : appui court = page suivante,
// appui maintenu = Dump Request (l'ancienne fonction de ce bouton).
const unsigned long DUMP_REQUEST_HOLD_MS = 800;

int           ledDumpReqBlink = 0;   // demi-périodes restantes (0=arrêté)
unsigned long ledDumpReqTimer = 0;
bool          ledDumpReqState = false;

// ledSlotBlink : 0=éteint, -1=fixe allumé, >0=clignote N demi-périodes
int           ledSlotBlink    = 0;
unsigned long ledSlotTimer    = 0;
bool          ledSlotState    = false;

int         currentPage = 0;
PotConfig*  pots = page0;
int         lastMovedPot = -1;

// =============== DÉFILEMENT AUTO ===============
unsigned long lastScrollTime        = 0;
const unsigned long SCROLL_INTERVAL = 3000;
int           currentDisplayGroup   = 0;
bool          autoScrollEnabled     = true;

// =================== BOUTONS ===================
// 5 boutons de page, un par page : etat de rebond independant pour chacun.
bool          pageBtnLast[NUM_PAGES]     = {HIGH, HIGH, HIGH, HIGH, HIGH};
bool          pageBtnState[NUM_PAGES]    = {HIGH, HIGH, HIGH, HIGH, HIGH};
unsigned long pageBtnDebounce[NUM_PAGES] = {0, 0, 0, 0, 0};

// Bouton de gauche, bas : slot suivant (court) / sauvegarde (long)
const int CYCLE_BUTTON_PIN = BTN_PRESET;

bool          cycleButtonLastState  = HIGH;
bool          cycleButtonState      = HIGH;
unsigned long cyclePressStartTime   = 0;
bool          cyclePressActive      = false;
bool          cycleLongPressHandled = false;
unsigned long cycleLastDebounceTime = 0;

// Bouton de gauche, haut : Dump Request (court) / supprimer (maintenu)
const int DUMP_BTN_PIN = BTN_DUMP;
bool          dumpBtnLastState      = HIGH;
bool          dumpBtnState          = HIGH;
unsigned long dumpBtnPressStart     = 0;
bool          dumpBtnActive         = false;
bool          dumpBtnLongHandled    = false;
unsigned long dumpBtnDebounceTime   = 0;

// --- Suppression d'un preset : appui LONG sur la broche 6 ---
// La meme broche porte les deux actions, distinguees par la duree :
//   appui court  -> Dump Request
//   appui maintenu -> suppression du preset courant
// L'invite n'apparait qu'apres DELETE_PROMPT_MS, pour qu'un appui bref
// destine au Dump Request ne fasse pas clignoter « Supprimer ? ».
// Le maintien fait office de confirmation : relacher avant la fin annule.
const unsigned long DELETE_HOLD_MS   = 3000;
const unsigned long DELETE_PROMPT_MS = 600;
bool          delPrompted = false;

const unsigned long LONG_PRESS_SAVE  = 3000;
const unsigned long SHORT_PRESS_TIME = 500;
const unsigned long debounceDelay    = 50;

unsigned long lastSendTime = 0;
const unsigned long DELAY_MS = 300;
bool needToSend = false;

bool          waitingForDump  = false;
unsigned long dumpRequestTime = 0;
const unsigned long DUMP_TIMEOUT = 3000;

// =================== LECTURE POTS ===================
// Lecture d'un potentiometre du shield, dans le bon sens.
//
// Deux corrections de carte sont concentrees ici :
//  - A15 est la sortie commune du 4067 : le pot #16 s'y lit par le canal 0,
//    un analogRead(A15) direct lirait un canal indetermine ;
//  - les 16 potentiometres ont leurs deux extremites cablees a l'envers du sens
//    de rotation (butee gauche = maximum), d'ou l'inversion 1023 - raw. La
//    molette et le switch de plage, cables dans la guitare, ne sont PAS
//    concernes : ils ne passent pas par ici.
static int lirePot(int shieldPin) {
  int raw = (shieldPin == A15) ? muxRead(0) : analogRead(megaAnalog(shieldPin));
  return 1023 - raw;
}

void updateFiltered(PotConfig &pot) {
  int raw = lirePot(pot.pin);
  if (pot.maxVal >= 31) {
    pot.filteredRaw = (pot.filteredRaw * 7 + raw) / 8;
  } else {
    pot.filteredRaw = (pot.filteredRaw * 3 + raw) / 4;
  }
}

int getQuantized(PotConfig &pot) {
  int val   = pot.filteredRaw;
  int steps = pot.maxVal + 1;
  int width = 1024 / steps;

  // Pour maxVal=255 (Rate 2 octets), width=4 — deadband applicable normalement.
  int db = max(1, width / 10);

  int raw_step  = constrain(val / width, 0, pot.maxVal);
  int dist_low  = val - raw_step * width;
  int dist_high = (raw_step + 1) * width - 1 - val;

  bool nearBoundary = false;
  if (raw_step > 0          && dist_low  < db) nearBoundary = true;
  if (raw_step < pot.maxVal && dist_high < db) nearBoundary = true;

  if (nearBoundary)
    return (pot.physValue >= 0) ? pot.physValue : raw_step;

  return raw_step;
}

void initFilter(PotConfig &pot) {
  int sum = 0;
  for (int i = 0; i < 8; i++) {
    sum += lirePot(pot.pin);
    delay(1);
  }
  pot.filteredRaw = sum / 8;
}

// =================== BUFFER & MIDI ===================
void writeToBuffer(PotConfig &pot, int val) {
  if (pot.maxVal == 255 && pot.position == 17) {
    // Rate : 2 octets — hi(offset 16)=val/128, lo(offset 17)=val%128
    if (16 < sendBufferSize) sendBuffer[16] = (byte)(val / 128);
    if (17 < sendBufferSize) sendBuffer[17] = (byte)(val % 128);
  } else {
    if (pot.position < sendBufferSize)
      sendBuffer[pot.position] = (byte)val;
  }
}

void sendSysEx() {
  if (sendBufferSize <= 0) return;
  for (int i = 0; i < sendBufferSize; i++) { MIDI_PORT.write(sendBuffer[i]); delay(2); }
}

void sendDumpRequest() {
  for (int i = 0; i < (int)sizeof(DUMP_REQUEST); i++) { MIDI_PORT.write(DUMP_REQUEST[i]); delay(2); }
}

// =================== NOM DU PRESET ===================
void extractName(const byte* buf, int size, char* out) {
  if (size < 13) { strncpy(out, "?????", 9); out[5] = '\0'; return; }
  int len = 0;
  for (int i = 5; i <= 12 && len < 8; i++) {
    byte c = buf[i];
    out[len++] = (c >= 0x20 && c < 0x7F) ? (char)c : ' ';
  }
  while (len > 0 && out[len-1] == ' ') len--;
  out[len] = '\0';
  if (len == 0) { strncpy(out, "?????", 9); out[5] = '\0'; }
}

// =================== AFFICHAGE OLED ===================
void showMessage(const char* line1, const char* line2 = nullptr) {
  if (!OLED_UTILISABLE) return;
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println(line1);
  if (line2) { display.setCursor(0, 12); display.println(line2); }
  display.display();
}

// Appelee par Ecran::display() entre deux tranches du transfert I2C (voir
// oled.h). pitchUpdate() ne touche jamais a l'affichage — elle depose ses
// evenements, que loop() consomme apres coup — donc aucune recursion possible ;
// le garde-fou ci-dessous n'est la que pour rendre cette propriete explicite.
void ecranPompe() {
  static bool dedans = false;
  if (dedans) return;
  dedans = true;
  pitchUpdate();
  dedans = false;
}

void updateDisplay() {
  if (!OLED_UTILISABLE) return;
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);

  if (waitingForDump) {
    display.print("Attente dump...");
  } else if (sendBufferSize == 0) {
    display.print("Korg Z3 ~ (vide)");
  } else {
    // Ligne 1 : "Z3~NomPreset X/N  III"
    display.print("Z3~");
    display.print(currentName);
    if (presetCount > 0) {
      display.print(" ");
      display.print(currentSlot + 1);
      display.print("/");
      display.print(presetCount);
    }
    display.print(" ");
    display.print(pageNames[currentPage]);
  }

  if (sendBufferSize > 0) {
    display.setTextSize(1);
    int startIdx = currentDisplayGroup * 4;
    int y = 12, x = 0;
    // Le rendu des polices coute ~43 ms pour une quarantaine de caracteres :
    // autant que trois periodes de molette. On rend donc la main a chaque
    // cellule dessinee, comme entre les tranches du transfert.
    ecranPompe();
    for (int i = 0; i < 4; i++) {
      ecranPompe();
      int idx = startIdx + i;
      if (idx < NUM_POTS) {
        display.setCursor(x, y);
        char buf[12];
        if (idx == lastMovedPot)
          snprintf(buf, sizeof(buf), ">%4s:%2d", pots[idx].name, pots[idx].lastValue);
        else
          snprintf(buf, sizeof(buf), "%5s:%2d", pots[idx].name, pots[idx].lastValue);
        display.print(buf);
      }
      x += 64;
      if (i == 1) { y += 8; x = 0; }
    }
  }
  display.display();
}

// =================== GESTION LEDS ===================
// Appelée dans chaque loop() — gère les timers de toutes les LEDs.
void updateLeds() {
  unsigned long now = millis();

  // LED_DUMP_REQ : clignote tant que ledDumpReqBlink > 0
  if (ledDumpReqBlink > 0 && (now - ledDumpReqTimer >= 200)) {
    ledDumpReqTimer = now;
    ledDumpReqState = !ledDumpReqState;
    digitalWrite(LED_DUMP_REQ, ledDumpReqState ? HIGH : LOW);
    ledDumpReqBlink--;
    if (ledDumpReqBlink == 0) digitalWrite(LED_DUMP_REQ, LOW);
  }

  // LED_SLOT : -1=fixe allumé / >0=clignote / 0=éteint
  if (ledSlotBlink == -1) {
    // fixe allumé — rien à faire, géré à l'activation
  } else if (ledSlotBlink > 0 && (now - ledSlotTimer >= 200)) {
    ledSlotTimer = now;
    ledSlotState = !ledSlotState;
    digitalWrite(LED_SLOT, ledSlotState ? HIGH : LOW);
    ledSlotBlink--;
    if (ledSlotBlink == 0) digitalWrite(LED_SLOT, LOW);
  }
}

// Affiche la page courante. Sur une carte au brochage correct, une LED par
// page ; sur la v1.2, un code binaire sur les trois LED pilotables :
//   I = *..    II = .*.    III = ..*    IV = **.    V = .**
void majLedsPages() {
  // PAGE_LED reste defini pour une carte au brochage correct, mais sur la v1.2
  // il ne pointe que sur des broches libres : c'est le code ci-dessous qui
  // pilote les LED reelles. Ne pas ecrire les deux, ils se marcheraient dessus.
  const byte code = PAGE_CODE[currentPage];
  for (byte b = 0; b < 3; b++) {
    digitalWrite(PAGE_CODE_LED[b], (code & (1 << b)) ? HIGH : LOW);
  }
}

// Démarre le clignotement LED_DUMP_REQ (continue jusqu'à fin du dump request)
void startLedDumpReq() {
  ledDumpReqBlink = 100; // grand nombre : continuera jusqu'à ce qu'on l'arrête
  ledDumpReqTimer = millis();
  ledDumpReqState = true;
  digitalWrite(LED_DUMP_REQ, HIGH);
}

void stopLedDumpReq() {
  ledDumpReqBlink = 0;
  ledDumpReqState = false;
  digitalWrite(LED_DUMP_REQ, LOW);
}

// LED_SLOT fixe = chargement d'un dump depuis la mémoire
void ledSlotLoad() {
  ledSlotBlink = -1;
  ledSlotState = true;
  digitalWrite(LED_SLOT, HIGH);
}

// LED_SLOT éteinte
void ledSlotOff() {
  ledSlotBlink = 0;
  ledSlotState = false;
  digitalWrite(LED_SLOT, LOW);
}

// LED_SLOT clignote 3 fois = sauvegarde confirmée
void ledSlotSave() {
  ledSlotBlink = 6; // 3 clignotements = 6 demi-périodes
  ledSlotTimer = millis();
  ledSlotState = true;
  digitalWrite(LED_SLOT, HIGH);
}

// =================== RECHARGEMENT DES POTS ===================
void reloadPotsFromBuffer() {
  for (int p = 0; p < NUM_PAGES; p++) {
    for (int i = 0; i < NUM_POTS; i++) {
      PotConfig &pot = pages[p][i];
      if (pot.maxVal == 255 && pot.position == 17) {
        int hi = (16 < sendBufferSize) ? sendBuffer[16] : 0;
        int lo = (17 < sendBufferSize) ? sendBuffer[17] : 0;
        pot.lastValue = constrain(hi * 128 + lo, 0, 255);
      } else if (pot.position < sendBufferSize) {
        pot.lastValue = sendBuffer[pot.position];
      }
    }
  }
  for (int i = 0; i < NUM_POTS; i++) {
    initFilter(pots[i]);
    pots[i].physValue   = getQuantized(pots[i]);
    pots[i].candidate   = pots[i].physValue;
    pots[i].stableCount = 0;
    pots[i].caught      = false;
    pots[i].catchOffset = pots[i].filteredRaw; // snapshot de la position physique actuelle
  }
  lastMovedPot        = -1;
  currentDisplayGroup = 0;
  lastScrollTime      = millis();
  autoScrollEnabled   = true;
}

// =================== GESTION DES SLOTS ===================
void loadSlot(int idx) {
  if (idx < 0 || idx >= presetCount || !presetSlots[idx].valid) return;
  currentSlot    = idx;
  sendBufferSize = presetSlots[idx].size;
  memcpy(sendBuffer, presetSlots[idx].data, sendBufferSize);
  strncpy(currentName, presetSlots[idx].name, 9);
  currentName[9] = '\0';
  reloadPotsFromBuffer();
  eeWriteByte(EE_ADDR_CURRENT, (byte)currentSlot);
  ledSlotLoad();   // LED_SLOT fixe allumée
  sendSysEx();
  lastSendTime = millis();
  updateDisplay();
}

void saveCurrentDump() {
  if (sendBufferSize == 0) return;
  if (presetCount >= MAX_PRESETS) {
    // Auparavant on decalait tout d'un cran, ce qui sacrifiait le slot 0 —
    // donc un preset usine. Puisqu'ils sont proteges, on refuse plutot
    // l'enregistrement : l'utilisateur libere lui-meme une place avec la
    // combinaison 7+8.
    showMessage("Memoire pleine", "Supprimer 7+8");
    delay(1500);
    updateDisplay();
    return;
  }
  int newIdx = presetCount;
  presetSlots[newIdx].size  = sendBufferSize;
  presetSlots[newIdx].valid = true;
  memcpy(presetSlots[newIdx].data, sendBuffer, sendBufferSize);
  extractName(sendBuffer, sendBufferSize, presetSlots[newIdx].name);
  strncpy(currentName, presetSlots[newIdx].name, 9);
  currentName[9] = '\0';
  presetCount++;
  currentSlot = newIdx;
  eeWriteSlot(newIdx);
  eeSaveMeta();
  ledSlotSave(); // LED_SLOT clignote 3 fois

  char msg[20];
  snprintf(msg, sizeof(msg), "Slot %d/%d sauve", currentSlot + 1, presetCount);
  showMessage("Dump sauvegarde!", msg);
  delay(800);
  updateDisplay();
}

// =================== CHANGEMENT DE PAGE ===================
void setPage(int newPage) {
  if (newPage == currentPage) return;
  currentPage = newPage;
  majLedsPages();   // la LED de la page courante reste allumee
  pots = pages[currentPage];
  for (int i = 0; i < NUM_POTS; i++) {
    initFilter(pots[i]);
    pots[i].physValue   = getQuantized(pots[i]);
    pots[i].candidate   = pots[i].physValue;
    pots[i].stableCount = 0;
    pots[i].caught      = false;
    pots[i].catchOffset = pots[i].filteredRaw; // snapshot pour le pickup relatif
    if (pots[i].maxVal == 255 && pots[i].position == 17) {
      int hi = (16 < sendBufferSize) ? sendBuffer[16] : 0;
      int lo = (17 < sendBufferSize) ? sendBuffer[17] : 0;
      pots[i].lastValue = constrain(hi * 128 + lo, 0, 255);
    } else if (pots[i].position < sendBufferSize) {
      pots[i].lastValue = sendBuffer[pots[i].position];
    }
  }
  lastMovedPot        = -1;
  currentDisplayGroup = 0;
  lastScrollTime      = millis();
  autoScrollEnabled   = true;
  updateDisplay();
}

// =================== RÉCEPTION MIDI ===================
void receiveMidi() {
  while (MIDI_PORT.available()) {
    byte b = MIDI_PORT.read();
    if (b == 0xF0) {
      rxLen = 0; rxInSysEx = true; rxHeaderPos = 1;
      rxBuffer[rxLen++] = b;
      continue;
    }
    if (!rxInSysEx) continue;
    if (rxHeaderPos < HEADER_LEN) {
      if (b == DUMP_HEADER[rxHeaderPos]) { rxHeaderPos++; }
      else { rxInSysEx = false; rxLen = 0; continue; }
    }
    if (rxLen < DUMP_MAX_SIZE) rxBuffer[rxLen++] = b;
    if (b == 0xF7) {
      rxInSysEx      = false;
      sendBufferSize = rxLen;
      memcpy(sendBuffer, rxBuffer, rxLen);
      extractName(sendBuffer, sendBufferSize, currentName);
      waitingForDump = false;
      stopLedDumpReq();
      reloadPotsFromBuffer();
      updateDisplay();
    }
  }
}

// Un preset usine est-il deja present en memoire ?
// On compare sur le NOM du dump (octets 5 a 12), qui identifie le patch de
// facon stable : c'est ce meme champ que extractName() utilise pour nommer les
// slots, a la sauvegarde comme au chargement usine.
bool factoryDejaPresent(const byte* dump) {
  byte entete[13];
  for (byte i = 0; i < 13; i++) entete[i] = pgm_read_byte(dump + i);
  char nom[10];
  extractName(entete, 13, nom);
  for (int i = 0; i < presetCount; i++) {
    if (presetSlots[i].valid && strncmp(presetSlots[i].name, nom, 9) == 0) return true;
  }
  return false;
}

// Copie un preset usine dans le slot idx. Les tableaux vivent en PROGMEM :
// memcpy_P, et non memcpy.
void chargerFactory(int idx, int f) {
  presetSlots[idx].size  = FACTORY[f].size;
  presetSlots[idx].valid = true;
  memcpy_P(presetSlots[idx].data, FACTORY[f].data, FACTORY[f].size);
  // Le nom affiche vient du dump lui-meme, comme a la sauvegarde : un preset
  // usine porte donc son vrai nom sans cas particulier.
  extractName(presetSlots[idx].data, presetSlots[idx].size, presetSlots[idx].name);
  eeWriteSlot(idx);
}

// Un preset usine ne doit pas pouvoir etre supprime. On identifie les slots
// proteges par leur NOM, et on ne protege que la PREMIERE occurrence : si
// l'utilisateur a enregistre une variante sous le meme nom, ce doublon-la
// reste supprimable, ce qui est precisement le cas d'usage.
bool slotEstUsine(int idx) {
  if (idx < 0 || idx >= presetCount || !presetSlots[idx].valid) return false;
  for (int f = 0; f < FACTORY_COUNT; f++) {
    byte entete[13];
    for (byte i = 0; i < 13; i++) entete[i] = pgm_read_byte(FACTORY[f].data + i);
    char nom[10];
    extractName(entete, 13, nom);
    if (strncmp(presetSlots[idx].name, nom, 9) != 0) continue;
    for (int j = 0; j < idx; j++) {
      if (presetSlots[j].valid && strncmp(presetSlots[j].name, nom, 9) == 0) return false;
    }
    return true;
  }
  return false;
}

// Supprime le slot courant : les suivants remontent d'un cran, le compteur
// diminue, et l'EEPROM est reecrite pour les slots deplaces.
void deleteCurrentPreset() {
  if (presetCount <= 0) return;
  if (slotEstUsine(currentSlot)) {
    showMessage("Preset usine", "non supprimable");
    delay(1200);
    updateDisplay();
    return;
  }
  char nom[10];
  strncpy(nom, presetSlots[currentSlot].name, 9);
  nom[9] = '\0';

  for (int i = currentSlot; i < presetCount - 1; i++) {
    presetSlots[i] = presetSlots[i + 1];
    eeWriteSlot(i);
  }
  presetCount--;
  presetSlots[presetCount].valid = false;
  if (currentSlot >= presetCount) currentSlot = presetCount - 1;
  if (currentSlot < 0) currentSlot = 0;
  eeSaveMeta();

  if (presetCount > 0) {
    sendBufferSize = presetSlots[currentSlot].size;
    memcpy(sendBuffer, presetSlots[currentSlot].data, sendBufferSize);
    strncpy(currentName, presetSlots[currentSlot].name, 9);
    currentName[9] = '\0';
    reloadPotsFromBuffer();
  } else {
    sendBufferSize = 0;
    strncpy(currentName, "--------", 9);
  }

  char msg[20];
  snprintf(msg, sizeof(msg), "%d restant(s)", presetCount);
  showMessage(nom, msg);
  delay(1200);
  updateDisplay();
}

// =================== SETUP ===================
void setup() {
  // Serial0 n'est plus utilise que par le televersement : le MIDI a son propre
  // port logiciel sur cette carte (voir midi_port.h). Plus aucune trace n'y est
  // ecrite — une impression periodique bloque quand rien ne vide le tampon USB.
  Serial.begin(115200);
  MIDI_PORT.begin(31250);
  delay(500);

  // Multiplexeur et molette : muxBegin() doit preceder tout initFilter(),
  // car le pot #16 se lit desormais par le canal 0.
  muxBegin();
  pitchBegin();

  // Boutons et LEDs en reserve du shield v1.0.

  // Boutons page +/-
  for (int i = 0; i < NUM_PAGES; i++) {
    pageBtnLast[i]  = (digitalRead(PAGE_BTN[i]) == HIGH);
    pageBtnState[i] = pageBtnLast[i];
  }

  // LEDs
  for (int i = 0; i < NUM_PAGES; i++) {
    pinMode(PAGE_BTN[i], INPUT_PULLUP);
    pinMode(PAGE_LED[i], OUTPUT);
    digitalWrite(PAGE_LED[i], LOW);
  }
  pinMode(LED_DUMP_REQ, OUTPUT); digitalWrite(LED_DUMP_REQ, LOW);
  for (byte b = 0; b < 3; b++) { pinMode(PAGE_CODE_LED[b], OUTPUT); digitalWrite(PAGE_CODE_LED[b], LOW); }
  pinMode(PAGE_NEXT_BTN, INPUT_PULLUP);
  pinMode(LED_SLOT,     OUTPUT); digitalWrite(LED_SLOT,     LOW);

  // Bouton cycle (pin 5)
  pinMode(CYCLE_BUTTON_PIN, INPUT_PULLUP);
  cycleButtonState     = (digitalRead(CYCLE_BUTTON_PIN) == HIGH);
  cycleButtonLastState = cycleButtonState;

  // Bouton Dump Request (pin 6)
  pinMode(DUMP_BTN_PIN, INPUT_PULLUP);
  dumpBtnState     = (digitalRead(DUMP_BTN_PIN) == HIGH);
  dumpBtnLastState = dumpBtnState;

  // OLED — voir OLED_UTILISABLE dans board_map.h. Sur la carte v1.2 les lignes
  // I2C de l'ecran ne tombent pas sur D20/D21, l'ecran ne peut donc pas
  // repondre. On evite Wire.begin() : le TWI confisquerait D20/D21, qui portent
  // le bouton et la LED de la page I.
  if (OLED_UTILISABLE) {
    bool oledOk = display.begin();
    display.setRotation(2);
    display.clearDisplay();
    display.display();
  }

  // Slots RAM
  for (int i = 0; i < MAX_PRESETS; i++) {
    presetSlots[i].valid = false;
    presetSlots[i].size  = 0;
  }

  // Chargement EEPROM
  bool eepromOk = eeLoad();
  if (!eepromOk || presetCount == 0) {
    // EEPROM vierge : on installe tous les presets usine.
    showMessage("1er demarrage", "Presets usine");
    delay(1000);
    for (int i = 0; i < FACTORY_COUNT; i++) chargerFactory(i, i);
    presetCount = FACTORY_COUNT;
    currentSlot = 0;
    eeWriteByte(EE_ADDR_FACTORY, (byte)FACTORY_COUNT);
    eeSaveMeta();
  } else {
    // EEPROM deja peuplee : on installe les presets usine que cet appareil n'a
    // JAMAIS recus, pour qu'une mise a jour du firmware les fasse apparaitre.
    //
    // On se fie a un compteur en EEPROM, et non a la presence des presets :
    // sinon un preset usine supprime par l'utilisateur reviendrait a chaque
    // demarrage, ce qui rendrait la suppression impossible.
    byte dejaInstalles = EEPROM.read(EE_ADDR_FACTORY);
    if (dejaInstalles == 0xFF || dejaInstalles > FACTORY_COUNT) {
      // Compteur jamais ecrit : appareil migrant depuis un firmware anterieur.
      // On retombe une seule fois sur la detection par nom, puis on amorce le
      // compteur.
      dejaInstalles = 0;
      for (int i = 0; i < FACTORY_COUNT; i++) {
        if (factoryDejaPresent(FACTORY[i].data)) dejaInstalles = i + 1;
      }
    }

    int ajoutes = 0;
    for (int i = dejaInstalles; i < FACTORY_COUNT && presetCount < MAX_PRESETS; i++) {
      chargerFactory(presetCount, i);
      presetCount++;
      ajoutes++;
    }
    eeWriteByte(EE_ADDR_FACTORY, (byte)FACTORY_COUNT);
    if (ajoutes > 0) {
      eeSaveMeta();
      char msg[20];
      snprintf(msg, sizeof(msg), "%d preset(s) usine", ajoutes);
      showMessage("Mise a jour", msg);
      delay(1200);
    }
  }

  // Charger le slot courant
  sendBufferSize = presetSlots[currentSlot].size;
  memcpy(sendBuffer, presetSlots[currentSlot].data, sendBufferSize);
  strncpy(currentName, presetSlots[currentSlot].name, 9);
  currentName[9] = '\0';

  // Init pots page 0
  pots = page0;
  currentPage = 0;
  for (int i = 0; i < NUM_POTS; i++) {
    initFilter(page0[i]);
    page0[i].physValue   = getQuantized(page0[i]);
    page0[i].candidate   = page0[i].physValue;
    page0[i].stableCount = 0;
    page0[i].caught      = false;
    page0[i].catchOffset = page0[i].filteredRaw;
    if (page0[i].maxVal == 255 && page0[i].position == 17) {
      int hi = (16 < sendBufferSize) ? sendBuffer[16] : 0;
      int lo = (17 < sendBufferSize) ? sendBuffer[17] : 0;
      page0[i].lastValue = constrain(hi * 128 + lo, 0, 255);
    } else {
      page0[i].lastValue = (page0[i].position < sendBufferSize)
                           ? sendBuffer[page0[i].position] : 0;
    }
  }

  // Si un dump est chargé dès le démarrage, allumer LED_SLOT fixe
  if (sendBufferSize > 0) ledSlotLoad();
  lastMovedPot        = -1;
  currentDisplayGroup = 0;
  lastScrollTime      = millis();
  autoScrollEnabled   = true;

  char startMsg[20];
  snprintf(startMsg, sizeof(startMsg), "%d dump(s) charge(s)", presetCount);
  showMessage("Korg Z3 Ready", startMsg);
  delay(1500);
  updateDisplay();

  majLedsPages();

  sendSysEx();
  lastSendTime = millis();
}

// =================== LOOP ===================
void loop() {

  // --- Molette de pitch : scannee a chaque tour, avant tout le reste ---
  pitchUpdate();

  switch (pitchTakeEvent()) {
    case PITCH_EVENT_CONNECTED:
      showMessage("Molette OK", "Centre calibre");
      delay(1000);
      updateDisplay();
      break;
    case PITCH_EVENT_DISCONNECTED:
      showMessage("Molette", "Debranchee");
      delay(1000);
      updateDisplay();
      break;
    case PITCH_EVENT_RANGE: {
      const char* label = "+/-1 octave";
      if (pitchBendSemitones() == 2) label = "+/-1 ton";
      else if (pitchBendSemitones() == 3) label = "+/-1 ton 1/2";
      showMessage("Bend range", label);
      delay(1000);
      updateDisplay();
      break;
    }
    default:
      break;
  }

  // --- Réception MIDI ---
  receiveMidi();

  // --- Timeout Dump Request ---
  if (waitingForDump && (millis() - dumpRequestTime > DUMP_TIMEOUT)) {
    waitingForDump = false;
    stopLedDumpReq();
    showMessage("Pas de reponse", "Verif. MIDI IN");
    delay(1000);
    updateDisplay();
  }

  // --- Bouton PAGE (carte v1.2) : court = page suivante, long = Dump Request ---
  // C'etait le bouton du Dump Request. Comme la carte v1.2 n'a pas de MIDI IN
  // par defaut, il sert d'abord a la navigation ; le Dump Request reste
  // accessible en maintien. Il ne recevra de reponse du Z3 que si le fil
  // sortie optocoupleur -> broche 53 est pose (voir midi_port.h) ; sans lui la
  // requete part dans le vide, sans dommage.
  {
    static int  navLast = HIGH, navState = HIGH;
    static unsigned long navDebounce = 0, navPressStart = 0;
    static bool navPressed = false, navLongDone = false;

    int r = digitalRead(PAGE_NEXT_BTN);
    if (r != navLast) navDebounce = millis();
    if ((millis() - navDebounce) > debounceDelay && r != navState) {
      navState = r;
      if (navState == LOW) {
        navPressStart = millis();
        navPressed    = true;
        navLongDone   = false;
      } else if (navPressed) {
        if (!navLongDone) setPage((currentPage + 1) % NUM_PAGES);
        navPressed = false;
      }
    }
    navLast = r;

    if (navPressed && !navLongDone &&
        (millis() - navPressStart) >= DUMP_REQUEST_HOLD_MS) {
      navLongDone     = true;
      waitingForDump  = true;
      dumpRequestTime = millis();
      rxLen           = 0;
      rxInSysEx       = false;
      showMessage("Dump Request...", "En attente Z3");
      sendDumpRequest();
    }
  }

  // --- 5 boutons de page : acces direct ---
  // Un bouton par page, avec sa LED. Remplace le defilement par deux boutons,
  // qui pouvait demander quatre appuis pour atteindre la bonne page.
  for (int i = 0; i < NUM_PAGES; i++) {
    int r = digitalRead(PAGE_BTN[i]);
    if (r != pageBtnLast[i]) pageBtnDebounce[i] = millis();
    if ((millis() - pageBtnDebounce[i]) > debounceDelay) {
      if (r != pageBtnState[i]) {
        pageBtnState[i] = r;
        if (pageBtnState[i] == LOW && i != currentPage) setPage(i);
      }
    }
    pageBtnLast[i] = r;
  }

  // --- Bouton cycle pin 5 : court=naviguer, long=sauvegarder ---
  {
    int r = digitalRead(CYCLE_BUTTON_PIN);
    if (r != cycleButtonLastState) cycleLastDebounceTime = millis();
    if ((millis() - cycleLastDebounceTime) > debounceDelay) {
      if (r != cycleButtonState) {
        cycleButtonState = r;
        if (cycleButtonState == LOW) {
          cyclePressStartTime   = millis();
          cyclePressActive      = true;
          cycleLongPressHandled = false;
        } else {
          // Tout relachement AVANT que l'appui long (3 s) ne se declenche fait
          // passer au slot suivant. Auparavant la fenetre etait limitee a
          // SHORT_PRESS_TIME (500 ms) : un appui un peu trop long ne faisait
          // rien du tout, d'ou l'impression qu'il fallait cliquer plusieurs
          // fois. La sauvegarde reste protegee par cycleLongPressHandled.
          if (cyclePressActive && !cycleLongPressHandled) {
            if (presetCount > 0) {
              loadSlot((currentSlot + 1) % presetCount);
            } else {
              showMessage("Aucun dump stocke", "Appui long 6=Dump");
              delay(800);
              updateDisplay();
            }
          }
          cyclePressActive = false;
        }
      }
    }
    cycleButtonLastState = r;

    // Détection appui long pin 5
    if (cyclePressActive && !cycleLongPressHandled) {
      if ((millis() - cyclePressStartTime) >= LONG_PRESS_SAVE) {
        cycleLongPressHandled = true;
        saveCurrentDump();
      }
    }
  }

  // --- Bouton pin 6 : court = Dump Request, maintenu = supprimer ---
  {
    int r = digitalRead(DUMP_BTN_PIN);
    if (r != dumpBtnLastState) dumpBtnDebounceTime = millis();
    if ((millis() - dumpBtnDebounceTime) > debounceDelay) {
      if (r != dumpBtnState) {
        dumpBtnState = r;
        if (dumpBtnState == LOW) {
          dumpBtnPressStart  = millis();
          dumpBtnActive      = true;
          dumpBtnLongHandled = false;
          delPrompted        = false;
        } else {
          // Relachement : si l'appui long n'a pas deja agi, c'est un Dump Request.
          if (dumpBtnActive && !dumpBtnLongHandled) {
            if (delPrompted) { delPrompted = false; updateDisplay(); }
            waitingForDump  = true;
            dumpRequestTime = millis();
            rxLen           = 0;
            rxInSysEx       = false;
            startLedDumpReq();
            showMessage("Dump Request...", "En attente Z3");
            sendDumpRequest();
          }
          dumpBtnActive = false;
          delPrompted   = false;
        }
      }
    }
    dumpBtnLastState = r;

    // Maintien : invite puis suppression
    if (dumpBtnActive && !dumpBtnLongHandled) {
      unsigned long tenu = millis() - dumpBtnPressStart;
      if (!delPrompted && tenu >= DELETE_PROMPT_MS && presetCount > 0) {
        delPrompted = true;
        if (slotEstUsine(currentSlot)) {
          showMessage("Usine - protege", presetSlots[currentSlot].name);
        } else {
          showMessage("Supprimer ?", presetSlots[currentSlot].name);
        }
      }
      if (tenu >= DELETE_HOLD_MS) {
        dumpBtnLongHandled = true;
        delPrompted        = false;
        if (presetCount > 0) deleteCurrentPreset();
      }
    }
  }

  // --- Gestion LEDs ---
  updateLeds();

  // --- Défilement automatique OLED ---
  // Suspendu pendant l'invite de suppression : il rafraichit l'ecran toutes
  // les 3 s et l'effacerait en pleine lecture.
  if (autoScrollEnabled && !delPrompted) {
    if (millis() - lastScrollTime >= SCROLL_INTERVAL) {
      lastScrollTime = millis();
      currentDisplayGroup = (currentDisplayGroup + 1) % 4;
      updateDisplay();
    }
  }

  // --- Potentiomètres ---
  if (sendBufferSize == 0) return;
  if (millis() - lastSendTime < DELAY_MS) return;

  for (int i = 0; i < NUM_POTS; i++) {
    updateFiltered(pots[i]);
    int newPhys = getQuantized(pots[i]);

    // --- MODE PICKUP : zone morte relative ---
    // Au changement de page/preset, caught=false et catchOffset=physValue snapshot.
    // Le pot est ignoré jusqu'à ce qu'il ait bougé d'au moins CATCH_STEPS paliers
    // depuis sa position initiale. Cela évite les faux positifs sans exiger de
    // traverser une valeur précise — la sensibilité reste maximale.
    if (!pots[i].caught) {
      int steps    = pots[i].maxVal + 1;
      int width    = 1024 / steps;
      int raw_step = constrain(pots[i].filteredRaw / width, 0, pots[i].maxVal);
      // Seuil exprime en unites ADC BRUTES, surtout pas en paliers : deux
      // paliers valent 0,8 % de la course sur un parametre 0-255 mais la MOITIE
      // de la course sur un 0-3, qui parait alors completement mort. 30 sur
      // 1024 font 3 % pour tous, quelle que soit la resolution du parametre.
      const int CATCH_RAW = 30;
      if (abs(pots[i].filteredRaw - pots[i].catchOffset) >= CATCH_RAW) {
        pots[i].caught    = true;
        pots[i].physValue = raw_step;
        pots[i].candidate = raw_step;
        // Ne pas écraser lastValue ici : le pot devient actif mais la prochaine
        // validation par stabilité déclenchera l'envoi si la valeur est différente.
      } else {
        pots[i].physValue = raw_step; // suivi silencieux
      }
      continue;
    }

    // Validation par stabilité adaptative
    // Les cellules RC materielles (1 k + 100 nF sur chaque pot) filtrent le
    // bruit en amont : trois lectures concordantes suffisent la ou il en fallait
    // six a huit du temps ou l'ADC lisait un signal nu.
    const int stableNeeded = 3;

    if (newPhys != pots[i].candidate) {
      pots[i].candidate   = newPhys;
      pots[i].stableCount = 1;
    } else {
      pots[i].stableCount++;
      if (pots[i].stableCount >= stableNeeded) {
        if (newPhys != pots[i].physValue) {
          pots[i].physValue   = newPhys;
          pots[i].lastValue   = newPhys;
          writeToBuffer(pots[i], newPhys);
          lastMovedPot        = i;
          currentDisplayGroup = i / 4;
          lastScrollTime      = millis();
          autoScrollEnabled   = true;
          needToSend          = true;
        }
        pots[i].stableCount = 0;
      }
    }
  }

  if (needToSend) {
    sendSysEx();
    lastSendTime = millis();
    needToSend   = false;
    updateDisplay();
  }
}
