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

# --- 5 boutons de page + 2 boutons de fonction, cables sur le PCB ---
# Colonne de droite, de haut en bas : pages I a V.
assert re.search(r'PAGE_BTN\[NUM_PAGES\]\s*=\s*\{\s*14\s*,\s*7\s*,\s*8\s*,\s*6\s*,\s*5\s*\}', fw), \
    "PAGE_BTN incorrect (attendu D14,D7,D8,D6,D5 de haut en bas)"
assert re.search(r'PAGE_LED\[NUM_PAGES\]\s*=\s*\{\s*15\s*,\s*12\s*,\s*11\s*,\s*10\s*,\s*9\s*\}', fw), \
    "PAGE_LED incorrect (attendu D15,D12,D11,D10,D9)"
# Colonne de gauche : dump en haut, presets en bas.
assert re.search(r'BTN_DUMP\s*=\s*16', fw),     "BTN_DUMP attendu sur D16"
assert re.search(r'LED_DUMP_REQ\s*=\s*17', fw), "LED_DUMP_REQ attendue sur D17"
assert re.search(r'BTN_PRESET\s*=\s*18', fw),   "BTN_PRESET attendu sur D18"
assert re.search(r'LED_SLOT\s*=\s*19', fw),     "LED_SLOT attendue sur D19"
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

print("check_fw OK — pages{14,7,8,6,5}/LED{15,12,11,10,9} | dump D16/D17 | preset D18/D19 | "
      "mux S0..S3={2,3,4,13} COM=A15 | molette ch1 | switch ch2 | pot16 ch0")
