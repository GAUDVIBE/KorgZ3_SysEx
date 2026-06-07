import pathlib, re

fw = pathlib.Path("../../firmware_CCSysEx_Patcher.ino").read_text()

assert "butLayout2" in fw, "butLayout2 absent"
assert "LEDLayout2" in fw, "LEDLayout2 absent"
assert re.search(r'butLayout2\s*\[\s*3\s*\]\s*=\s*\{\s*22\s*,\s*24\s*,\s*26\s*\}', fw), "butLayout2 valeurs incorrectes"
assert re.search(r'LEDLayout2\s*\[\s*3\s*\]\s*=\s*\{\s*23\s*,\s*25\s*,\s*27\s*\}', fw), "LEDLayout2 valeurs incorrectes"
assert "INPUT_PULLUP" in fw, "pinMode pullup absent"

print("check_fw OK — butLayout2{22,24,26} + LEDLayout2{23,25,27} + pinMode")
