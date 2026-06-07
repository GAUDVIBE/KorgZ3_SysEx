import json, math, pathlib

# Guard: must be run from hardware/kicad_project/
assert pathlib.Path("shield_redesign").is_dir(), "Run from hardware/kicad_project/"

# Board outline (Edge.Cuts): rounded rectangle (20,20)..(192,192), corner radius ~5mm
# All coordinates in mm.

geo = json.loads(pathlib.Path("shield_redesign/mega_geometry.json").read_text())

tpl_analog_y = 97.46   # template analog row (J3/J5 @ y=97.46)
tpl_digit_y  = 49.20   # template digital row (J2/J4/J6 @ y=49.2)

# CARD_ANALOG_Y / CARD_DIGIT_Y measured from the v0.9 SysEx_Patcher.kicad_pcb header rows
CARD_ANALOG_Y = 86.0   # target Y for analog header row on the shield PCB
CARD_DIGIT_Y  = 134.0  # target Y for digital header row on the shield PCB

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


def _rot_matrix(r_deg):
    """Return (cos, sin) for rotation r_deg CCW."""
    rad = math.radians(r_deg)
    return math.cos(rad), math.sin(rad)


def apply_rot_mirror(ox, oy, r, mirror, lx, ly):
    """
    Place a pad at local offset (lx, ly) given:
      - footprint origin (ox, oy)
      - rotation r (degrees, CCW)
      - mirror: None | "x" (flip Y) | "y" (flip X)
    Matches KiCad convention: rotate first, then mirror.
    """
    c, s = _rot_matrix(r)
    rx = lx * c - ly * s
    ry = lx * s + ly * c
    if mirror == "x":
        ry = -ry
    elif mirror == "y":
        rx = -rx
    return (ox + rx, oy + ry)


def get_local_offsets(h):
    """
    Convert template (dx,dy) pad offsets to footprint-local coords by
    undoing the template footprint rotation.
    In template world: pad_world = (h.x + dx, h.y + dy)
    Local coords = R(-template_rot) * (dx, dy)
    """
    rot_t = h["rot"]
    c, s = _rot_matrix(-rot_t)
    locs = []
    for p in h["pads"]:
        dx, dy = p["dx"], p["dy"]
        lx = dx * c - dy * s
        ly = dx * s + dy * c
        locs.append((p["num"], p["signal"], lx, ly))
    return locs


def find_rot_mirror(h, ox, oy, target_by_num):
    """
    Brute-force over r in {0,90,180,270} and mirror in {None,"x","y"}.
    Prefer mirror=None, then "x", then "y" (prefer no mirror for 1×n headers).
    Return first (r, mirror) whose world pad positions match target_by_num within 0.01 mm.
    """
    locals_ = get_local_offsets(h)
    for r in [0, 90, 180, 270]:
        for mirror in [None, "x", "y"]:
            if all(
                abs(apply_rot_mirror(ox, oy, r, mirror, lx, ly)[0] - target_by_num[num][0]) < 0.01
                and abs(apply_rot_mirror(ox, oy, r, mirror, lx, ly)[1] - target_by_num[num][1]) < 0.01
                for num, _sig, lx, ly in locals_
            ):
                return r, mirror
    return None, None


headers = []
for h in geo:
    fx, fy = xf(h["x"], h["y"])

    # Compute absolute pad positions (these are CORRECT — do not change)
    pads_abs = []
    target_by_num = {}
    for p in h["pads"]:
        ax, ay = h["x"] + p["dx"], h["y"] + p["dy"]
        cx, cy = xf(ax, ay)
        cx, cy = round(cx, 3), round(cy, 3)
        pads_abs.append(dict(num=p["num"], signal=p["signal"], x=cx, y=cy))
        target_by_num[p["num"]] = (cx, cy)

    # Determine consistent (rot, mirror) pair via brute force
    rot, mirror = find_rot_mirror(h, round(fx, 3), round(fy, 3), target_by_num)
    if rot is None:
        raise RuntimeError(f"No valid (rot, mirror) found for header {h['ref']}")

    headers.append(dict(
        ref=h["ref"],
        footprint=h["footprint"],
        x=round(fx, 3),
        y=round(fy, 3),
        rot=rot,
        mirror=mirror,
        pads_abs=pads_abs
    ))

sigmap = {}
for h in headers:
    for p in h["pads_abs"]:
        if p["signal"]:
            sigmap.setdefault(p["signal"], []).append(
                dict(ref=h["ref"], pad=p["num"], x=p["x"], y=p["y"])
            )

out = dict(
    headers=headers,
    sigmap=sigmap,
    TX=round(TX, 5),
    TY=round(TY, 5),
)
pathlib.Path("shield_redesign/placement.json").write_text(json.dumps(out, indent=1))
print("placement calculé — TX=%.5f TY=%.5f" % (TX, TY))
print("Board outline: X=20..192, Y=20..192 (rounded rect, r=5mm)")

# Summary
for h in headers:
    xs = [p["x"] for p in h["pads_abs"]]
    ys = [p["y"] for p in h["pads_abs"]]
    print("  %-4s: rot=%-3s mirror=%-4s  X=%.1f..%.1f  Y=%.1f..%.1f" % (
        h["ref"], h["rot"], str(h["mirror"]), min(xs), max(xs), min(ys), max(ys)))

# ── Self-verification ──────────────────────────────────────────────────────────
# For every header, assert that applying (x, y, rot, mirror) to the template
# local pad offsets reproduces pads_abs within 0.01 mm.
# This guarantees Task 3 can trust both fields.
print("\nSelf-verification:")
geo_by_ref = {h["ref"]: h for h in geo}
for hdr in headers:
    h_tpl = geo_by_ref[hdr["ref"]]
    locals_ = get_local_offsets(h_tpl)
    abs_by_num = {p["num"]: (p["x"], p["y"]) for p in hdr["pads_abs"]}
    ox, oy = hdr["x"], hdr["y"]
    r, mirror = hdr["rot"], hdr["mirror"]
    for num, sig, lx, ly in locals_:
        wx, wy = apply_rot_mirror(ox, oy, r, mirror, lx, ly)
        tx, ty = abs_by_num[num]
        if abs(wx - tx) > 0.01 or abs(wy - ty) > 0.01:
            raise AssertionError(
                f"Self-verification FAILED for {hdr['ref']} pad {num}: "
                f"computed=({wx:.4f},{wy:.4f}) expected=({tx:.4f},{ty:.4f})"
            )
    print(f"  {hdr['ref']:4s}: rot={r:<3} mirror={str(mirror):<5}  OK ({len(locals_)} pads)")

print("\nSelf-verification PASSED — all headers consistent.")
