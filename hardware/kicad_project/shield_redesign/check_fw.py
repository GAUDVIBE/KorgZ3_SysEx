import pathlib, re

# Verifie que le firmware reel est d'accord avec le cuivre de la carte.
# Chemin resolu quel que soit le cwd : shield_redesign -> kicad_project ->
# hardware -> racine du depot.
_here = pathlib.Path(__file__).resolve()
root = _here.parents[3]
fw_path = root / "KorgZ3_SysEx_26-07-2026" / "KorgZ3_SysEx_26-07-2026.ino"
mux_path = root / "KorgZ3_SysEx_26-07-2026" / "mux.cpp"
pitch_path = root / "KorgZ3_SysEx_26-07-2026" / "pitch.cpp"

fw = fw_path.read_text()
mux = mux_path.read_text()
pitch = pitch_path.read_text()

# --- Boutons et LEDs en reserve du bloc 2x18 ---
assert "butLayout2" in fw, "butLayout2 absent"
assert "LEDLayout2" in fw, "LEDLayout2 absent"
assert re.search(r'butLayout2\s*\[\s*3\s*\]\s*=\s*\{\s*14\s*,\s*16\s*,\s*18\s*\}', fw), \
    "butLayout2 valeurs incorrectes (attendu D14,D16,D18)"
assert re.search(r'LEDLayout2\s*\[\s*3\s*\]\s*=\s*\{\s*15\s*,\s*17\s*,\s*19\s*\}', fw), \
    "LEDLayout2 valeurs incorrectes (attendu D15,D17,D19)"
assert "INPUT_PULLUP" in fw, "pinMode pullup absent"

# --- Multiplexeur 4067 : selection D2/D3/D4/D13, commun sur A15 ---
assert re.search(r'MUX_SEL\s*\[\s*4\s*\]\s*=\s*\{\s*2\s*,\s*3\s*,\s*4\s*,\s*13\s*\}', mux), \
    "lignes de selection du mux incorrectes (attendu D2,D3,D4,D13)"
assert re.search(r'MUX_COM\s*=\s*A15', mux), "sortie commune du mux non cablee sur A15"

# --- Affectation des canaux ---
assert re.search(r'CH_WHEEL\s*=\s*1', pitch), "molette attendue sur le canal 1"
assert re.search(r'CH_SWITCH\s*=\s*2', pitch), "switch attendu sur le canal 2"

# --- Le pot #16 doit passer par le canal 0, pas par un analogRead(A15) direct ---
assert re.search(r'pot\.pin\s*==\s*A15\s*\)\s*\?\s*muxRead\(0\)', fw), \
    "le pot #16 ne passe pas par muxRead(0)"

print("check_fw OK — butLayout2{14,16,18} | LEDLayout2{15,17,19} | "
      "mux S0..S3={2,3,4,13} COM=A15 | molette ch1 | switch ch2 | pot16 ch0")
