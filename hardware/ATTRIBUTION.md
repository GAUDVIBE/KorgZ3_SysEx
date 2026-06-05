# Hardware attribution & license

The PCB in this `hardware/` folder is a **KiCad reconstruction** of the
**“MIDI SysEx Patcher Ver.2”** board designed by **baritonomarchetto**
(synthbrigade.altervista.org), reverse-engineered from the publicly shared
Gerber files since no schematic was ever published.

- Original project: <https://www.instructables.com/MIDI-SysEx-Patcher-Ver2/>
- Original repo (firmware + Gerbers): <https://github.com/baritonomarchetto/arduino-SysEx-Patcher>

## License notice (important)

- The **original hardware** (and therefore this reconstruction, which is a
  derivative work) is published by its author under **CC BY-NC-SA**
  (Attribution — NonCommercial — ShareAlike). The files in this `hardware/`
  folder are distributed under those same terms, **not** under the MIT license
  that covers the firmware in the rest of this repository.
- `firmware_CCSysEx_Patcher.ino` is the original author's firmware, included for
  reference under its **MIT** license (see `firmware_LICENSE.txt`).

## What was reconstructed / added here

- Schematic + PCB rebuilt in KiCad from the Gerbers, BOM and firmware pin-map;
  the MIDI-IN (6N138) section was confirmed by tracing the copper layers.
- **Modification v0.3** (this project's own additions): a **CD74HC4067** analog
  multiplexer (extra analog input on a 6.35 mm remote-pot jack + spare channels)
  and **RC anti-noise filters** (1 kΩ series + 100 nF to GND) on every analog input.

See `RECONSTRUCTION_SCHEMA.md` for the full details, netlist and firmware notes.
