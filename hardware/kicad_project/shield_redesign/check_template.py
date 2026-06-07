import json, sys, pathlib
geo = json.loads(pathlib.Path("shield_redesign/mega_geometry.json").read_text())
refs = {h["ref"] for h in geo}
assert {"J1","J2","J3","J4","J5","J6","J7"} <= refs, f"embases manquantes: {refs}"
sigs = {p["signal"] for h in geo for p in h["pads"]}
need = {f"A{i}" for i in range(16)} | {f"D{i}" for i in range(0,28)}
missing = need - sigs
assert not missing, f"signaux manquants: {sorted(missing)}"
j7 = next(h for h in geo if h["ref"]=="J7")
assert len(j7["pads"]) == 36, f"J7 pads={len(j7['pads'])}"
print("check_template OK:", len(geo), "embases,", len(sigs), "signaux")
