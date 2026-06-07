import json, pathlib

# Guard: must be run from hardware/kicad_project/
assert pathlib.Path("shield_redesign").is_dir(), "Run from hardware/kicad_project/"

# Board outline (Edge.Cuts from SysEx_Patcher.kicad_pcb):
#   gr_line segments and gr_arc corners forming a rounded rectangle:
#     straight edges: x in [20,192], y in [20,192]
#     corners at (25,20), (187,20), (192,25), (192,187), (187,192), (25,192), (20,187), (20,25)
#     corner radius ~5mm
BOARD_X_MIN = 20.0
BOARD_X_MAX = 192.0
BOARD_Y_MIN = 20.0
BOARD_Y_MAX = 192.0
MARGIN = 3.0  # keep pads at least 3mm inside the outline

P = json.loads(pathlib.Path("shield_redesign/placement.json").read_text())
heads = {h["ref"]: h for h in P["headers"]}

ay = [p["y"] for p in heads["J3"]["pads_abs"]]      # A0..A7
dy = [p["y"] for p in heads["J2"]["pads_abs"]]      # D8..SCL / D21

assert abs(sum(ay)/len(ay) - 86) < 3, \
    "rangée analog mal calée: %.2f" % (sum(ay)/len(ay))
assert abs(sum(dy)/len(dy) - 134) < 3, \
    "rangée digit mal calée: %.2f" % (sum(dy)/len(dy))

j7 = heads["J7"]["pads_abs"]
xs = [p["x"] for p in j7]
ys = [p["y"] for p in j7]

# Use REAL board outline extents with 3mm margin
assert max(xs) < BOARD_X_MAX - MARGIN, \
    "J7 dépasse le bord droit: max_x=%.2f > %.2f" % (max(xs), BOARD_X_MAX - MARGIN)
assert min(xs) > BOARD_X_MIN + MARGIN, \
    "J7 dépasse le bord gauche: min_x=%.2f < %.2f" % (min(xs), BOARD_X_MIN + MARGIN)
assert max(ys) < BOARD_Y_MAX - MARGIN, \
    "J7 dépasse le bord bas: max_y=%.2f > %.2f" % (max(ys), BOARD_Y_MAX - MARGIN)
assert min(ys) > BOARD_Y_MIN + MARGIN, \
    "J7 dépasse le bord haut: min_y=%.2f < %.2f" % (min(ys), BOARD_Y_MIN + MARGIN)

# Check all headers are inside the board outline (with margin)
for ref, h in heads.items():
    hxs = [p["x"] for p in h["pads_abs"]]
    hys = [p["y"] for p in h["pads_abs"]]
    assert min(hxs) > BOARD_X_MIN + MARGIN, \
        "%s bord gauche: min_x=%.2f < %.2f" % (ref, min(hxs), BOARD_X_MIN + MARGIN)
    assert max(hxs) < BOARD_X_MAX - MARGIN, \
        "%s bord droit: max_x=%.2f > %.2f" % (ref, max(hxs), BOARD_X_MAX - MARGIN)
    assert min(hys) > BOARD_Y_MIN + MARGIN, \
        "%s bord haut: min_y=%.2f < %.2f" % (ref, min(hys), BOARD_Y_MIN + MARGIN)
    assert max(hys) < BOARD_Y_MAX - MARGIN, \
        "%s bord bas: max_y=%.2f > %.2f" % (ref, max(hys), BOARD_Y_MAX - MARGIN)

print("check_placement OK — analogY=%.1f digitY=%.1f J7x=%.1f..%.1f" % (
    sum(ay)/len(ay), sum(dy)/len(dy), min(xs), max(xs)))
print("Board outline: X=%.0f..%.0f Y=%.0f..%.0f (margin=%.0fmm)" % (
    BOARD_X_MIN, BOARD_X_MAX, BOARD_Y_MIN, BOARD_Y_MAX, MARGIN))
print("TX=%.2f  TY=%.2f" % (P["TX"], P["TY"]))
