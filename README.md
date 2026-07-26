# Korg Z3 SysEx Controller

An Arduino Mega 2560-based MIDI controller for the **Korg Z3 FM synthesizer**, allowing real-time editing of all internal FM parameters via 16 analog potentiometers, organized across 5 pages.

> The Korg Z3 only accepts complete SysEx parameter dumps — this controller handles that constraint transparently, sending a full 96-byte dump on every parameter change.

---

## Features

- **16 potentiometers × 5 pages** = up to 80 parameters mapped to the Z3's FM engine
- **Live Dump Request** — fetch the current preset directly from the Z3 over MIDI
- **Up to 8 preset slots** stored in RAM, persisted to EEPROM (survives power-off)
- **Factory preset** (SynLead) loaded on first boot
- **Pickup mode** — pots are inactive until moved, preventing value jumps on page change
- **Adaptive filtering** — exponential smoothing + proportional deadband, tuned per parameter range
- **128×32 OLED display** — shows preset name, slot index, page, and live parameter values with auto-scroll
- **4 LEDs** with distinct behaviors for navigation, dump request, and memory operations

---

## Hardware

| Component | Detail |
|---|---|
| Microcontroller | Arduino Mega 2560 |
| Potentiometers | 16× analog, pins A0–A15 |
| Button − (page prev) | Digital pin 7 |
| Button + (page next) | Digital pin 8 |
| Button cycle/save | Digital pin 5 |
| Button Dump Request | Digital pin 6 |
| LED page − | Digital pin 12 |
| LED page + | Digital pin 11 |
| LED Dump Request | Digital pin 10 |
| LED slot | Digital pin 9 |
| OLED display | 128×32 I2C, SDA=20, SCL=21, address 0x3C |

> **Tip:** Adding a 100nF ceramic capacitor between each pot's wiper and GND significantly reduces ADC noise and improves parameter stability.

### PCB / KiCad design

A full KiCad reconstruction of the board (schematic + PCB) lives in [`hardware/`](hardware/),
rebuilt from the original Gerbers. It also includes an extension: a **CD74HC4067** analog
multiplexer (extra analog inputs + spare channels) and **RC anti-noise filters** (1 kΩ + 100 nF)
on every analog input. See
[`hardware/RECONSTRUCTION_SCHEMA.md`](hardware/RECONSTRUCTION_SCHEMA.md) and
[`hardware/ATTRIBUTION.md`](hardware/ATTRIBUTION.md) (hardware is CC BY-NC-SA, derived from
baritonomarchetto's design).

### Pitch wheel & bend range (v1.1)

A **pitch wheel** and a **3-position bend-range switch** are mounted **on the guitar** and connected
to the shield by a single 4-conductor cable through a **mini-XLR** socket. The switch selects the
musical amplitude of the wheel:

| Switch position | Bend range |
|---|---|
| 1 | ±1 tone |
| 2 | ±1.5 tones |
| 3 | ±1 octave |

The Korg Z3 hard-maps the full MIDI pitch-bend range `[0..16383]` onto ±12 semitones and offers no
way to change it. To obtain a smaller, more playable amplitude, the firmware restricts the range it
emits to `8192 ± N × 8192 / 12`.

Both inputs are read through the multiplexer: the wheel on channel 1, the switch on channel 2. The
switch is encoded as an analog voltage by a resistor ladder inside the guitar, which is what keeps
the cable down to four conductors.

**Unplugged-cable safety.** The three switch positions sit at ¼, ½ and ¾ of Vcc — never at 0 V. A
470 kΩ pull-down on the board means an unplugged cable reads ~0 V, a value belonging to no valid
position. The firmware therefore knows nothing is connected, freezes the pitch at centre and ignores
the wheel, instead of reading a floating input and emitting phantom pitch bend. The same channel thus
acts as both range selector and cable-presence detector.

Plugging the cable in also triggers an automatic calibration of the wheel's rest position, so the
controller no longer has to be powered on with the wheel centred.

---

## Button Behavior

| Button | Action |
|---|---|
| Pin 7 (−) | Previous page (I → V → IV → …) |
| Pin 8 (+) | Next page (I → II → … → V → I) |
| Pin 5 short press | Cycle through stored preset slots |
| Pin 5 long press ≥3s | Save current dump to memory (RAM + EEPROM) |
| Pin 6 long press ≥3s | Send Dump Request to Z3, receive and load the response |

## LED Behavior

| LED | Pin | Behavior |
|---|---|---|
| LED_PREV | 12 | Flashes 300ms on page − click |
| LED_NEXT | 11 | Flashes 300ms on page + click |
| LED_DUMP_REQ | 10 | Blinks continuously during Dump Request, off on receive/timeout |
| LED_SLOT | 9 | Solid = preset loaded from memory / Blinks 3× = save confirmed |

---

## Pages & Parameters

### Page I — Global & Levels
| Pot | Parameter | Range | SysEx offset |
|---|---|---|---|
| A0 | Algorithm | 0–7 | 13 |
| A5 | Feedback | 0–7 | 14 |
| A8 | LFO Wave | 0–3 | 15 |
| A12 | LFO Rate | 0–255 | 16 (hi) + 17 (lo) |
| A1 | PMD | 0–127 | 18 |
| A4 | PMS | 0–7 | 19 |
| A9 | AMD | 0–127 | 20 |
| A13 | AMS | 0–3 | 21 |
| A2 | M1 Wave | 0–7 | 22 |
| A6 | C1 Wave | 0–7 | 23 |
| A10 | M2 Wave | 0–7 | 24 |
| A14 | C2 Wave | 0–7 | 25 |
| A3 | M1 Level | 0–127 | 42 |
| A7 | C1 Level | 0–127 | 43 |
| A11 | M2 Level | 0–127 | 44 |
| A15 | C2 Level | 0–127 | 45 |

### Page II — Envelopes (Attack / Decay / Sustain / Decay2)
M1, C1, M2, C2 — Attack (0–31), Decay1 (0–31), Sustain (0–15), Decay2 (0–31) — offsets 46–61

### Page III — Release, EG Bias, Detune
M1, C1, M2, C2 — Release (0–15), EG Bias (0–3), Detune1 (0–7), Detune2 (0–3) — offsets 34–41, 62–65, 74–77

### Page IV — Multiply, AMS Enable, Key Scale
M1, C1, M2, C2 — Multiply1 (0–15), Multiply2 (0–15), AMS Enable (0–1), Key Scale (0–3) — offsets 26–33, 66–73

### Page V — Velocity, Keyboard Track, Reverb, Rate/op
M1, C1, M2, C2 — Velocity Int (0–15), Keyboard Track (0–15), Reverb (0–1), Rate (0–7) — offsets 78–93

---

## MIDI / SysEx Details

- **Baud rate:** 31250 (standard MIDI)
- **Dump Request:** `F0 42 30 1D 10 F7`
- **Dump header:** `F0 42 30 1D 40` + 8 bytes ASCII name + parameters + `F7`
- **Dump size:** 95–96 bytes
- **Send guard:** 300ms minimum between consecutive SysEx transmissions

### LFO Rate — 2-byte parameter
LFO Rate is the only 2-byte parameter in the Z3 dump:
- Offset 16 (hi-byte) = `val / 128` → `0x00` or `0x01`
- Offset 17 (lo-byte) = `val % 128` → `0x00`–`0x7F`

---

## EEPROM Layout

| Address | Content |
|---|---|
| 0 | Magic byte `0xA5` (initialized flag) |
| 1 | Preset count |
| 2 | Current slot index |
| 3 + n×108 | Slot n: 2B size + 10B name + 96B data |

**Total used:** 867 bytes out of 4096 available on the Mega.

---

## Filtering & Stability

The controller uses a multi-stage approach to eliminate ADC noise without sacrificing responsiveness:

1. **Exponential smoothing** — coefficient `7/8` for large ranges (≥31 values), `3/4` for small ranges
2. **Proportional deadband** — 10% of step width around each boundary, minimum 1 LSB
3. **Stability validation** — 8 consecutive identical readings required for ranges ≥127, 6 for smaller ranges
4. **Relative pickup** — on page/preset change, a pot must move ≥2 steps from its resting position before becoming active (prevents false triggers without requiring the pot to find an exact value)

---

## Libraries Required

Install via Arduino Library Manager:
- `Adafruit GFX Library`
- `Adafruit SSD1306`
- `Wire` (built-in)
- `EEPROM` (built-in)

---

## Project Background

Built around a custom board inspired by [baritonomarchetto's Arduino SysEx Patcher](https://www.instructables.com/MIDI-SysEx-Patcher-Ver2/). The Z3's FM engine requires complete parameter dumps for every edit — this controller manages that constraint internally, giving the user a seamless knob-per-function experience.

---

## License

MIT
