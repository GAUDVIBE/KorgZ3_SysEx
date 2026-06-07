import json, pathlib

# Board outline (Edge.Cuts): rounded rectangle (20,20)..(192,192), corner radius ~5mm
# All coordinates in mm.

geo = json.loads(pathlib.Path("shield_redesign/mega_geometry.json").read_text())

tpl_analog_y = 97.46   # template analog row (J3/J5 @ y=97.46)
tpl_digit_y  = 49.20   # template digital row (J2/J4/J6 @ y=49.2)

CARD_ANALOG_Y = 86.0
CARD_DIGIT_Y  = 134.0

# Vertical mirror: Yc = -Yt + TY
# Calibration:  -tpl_analog_y + TY = CARD_ANALOG_Y  =>  TY = CARD_ANALOG_Y + tpl_analog_y
TY = CARD_ANALOG_Y + tpl_analog_y   # 183.46

# TX chosen so that:
#   - J2 leftmost pad (template x = 118.796 - 22.86 = 95.936) lands > 23mm (board x=20 + 3mm margin)
#   - J7 (2x18 block) lands in free space to the right of the 1x-row headers, away from pots
#   - All headers stay inside the 20..192 board extent
# TX = -65.0 gives:
#   J2 x_min = 30.94  (>23, OK)
#   J7 x = 128.98..131.52  (well inside board, 14.5mm clearance from nearest pot at x=114)
TX = -65.0

def xf(x, y):
    """Apply the template->card rigid transformation: Xc = x + TX, Yc = -y + TY."""
    return (x + TX, -y + TY)

headers = []
for h in geo:
    fx, fy = xf(h["x"], h["y"])
    pads_abs = []
    for p in h["pads"]:
        ax, ay = h["x"] + p["dx"], h["y"] + p["dy"]
        cx, cy = xf(ax, ay)
        pads_abs.append(dict(num=p["num"], signal=p["signal"],
                             x=round(cx, 3), y=round(cy, 3)))
    headers.append(dict(
        ref=h["ref"],
        footprint=h["footprint"],
        x=round(fx, 3),
        y=round(fy, 3),
        rot=(-h["rot"]) % 360,
        pads_abs=pads_abs
    ))

sigmap = {}
for h in headers:
    for p in h["pads_abs"]:
        if p["signal"]:
            sigmap.setdefault(p["signal"], []).append(
                dict(ref=h["ref"], pad=p["num"], x=p["x"], y=p["y"])
            )

out = dict(headers=headers, sigmap=sigmap, TX=TX, TY=TY)
pathlib.Path("shield_redesign/placement.json").write_text(json.dumps(out, indent=1))
print("placement calculé — TX=%.2f TY=%.2f" % (TX, TY))
print("Board outline: X=20..192, Y=20..192 (rounded rect, r=5mm)")

# Summary
for h in headers:
    xs = [p["x"] for p in h["pads_abs"]]
    ys = [p["y"] for p in h["pads_abs"]]
    print("  %-4s: X=%.1f..%.1f  Y=%.1f..%.1f" % (
        h["ref"], min(xs), max(xs), min(ys), max(ys)))
