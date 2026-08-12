#!/usr/bin/env python3
"""Genere KorgZ3_Editor.amxd de A a Z (conteneur ampf + patcher JSON).

Interface calquee sur l'editeur midierror DX100 : panneaux noirs, un libelle
au-dessus de chaque controle, live.dial pour le continu, live.numbox pour les
petits entiers, live.text Off/On pour les booleens.

Cablage (le patcher ne contient aucune logique, tout est dans le pont JS) :
  controle -> [prepend param <pos>] -> [node.script korgz3_bridge.js]
  node.script -> [route sysex load setname status] :
     sysex   -> [iter] -> [midiout]
     load    -> [unpack x80] -> [prepend set] -> controles
     setname -> [prepend set] -> textedit
"""
import json, struct, sys, time
from z3_protocol import all_params, value_at, SYNLEAD, OSC, OSC_PARAMS, GLOBAL

PARAMS = all_params()

FONT   = "Ableton Sans Medium"
WHITE  = [1.0, 1.0, 1.0, 1.0]
BLUE   = [0.443137, 0.529412, 0.941176, 1.0]     # fond actif numbox/dial
GREEN  = [0.301961, 0.772549, 0.305882, 1.0]     # lcd + on
AMBER  = [0.803922, 0.682353, 0.415686, 1.0]     # off
TRI    = [0.843137, 0.741176, 0.431373, 1.0]

# --- metriques (px) ---
COL   = 44          # pas horizontal d'une colonne de controle
WAVE_W = 44         # colonne de la forme d'onde (graphique + sa valeur)
ALGO_W = 72         # colonne de l'algorithme (schema sur 2 rangees + sa valeur)
LBL_H = 15          # hauteur d'un libelle
DIAL_W, DIAL_H = 36, 39
BOX_W,  BOX_H  = 31, 15
PAD   = 8
# Un libelle occupe LBL_H+3 = 18 px : le controle doit commencer APRES.
ROW1_L, ROW1_C = 20, 39     # libelle 20-38, dial 39-78
ROW2_L, ROW2_C = 82, 101    # libelle 82-100, numbox 101-116
ROW3_L, ROW3_C = 120, 139   # libelle 120-138, numbox 139-154


def build_patcher():
    boxes, lines, nid = [], [], [0]
    def new_id():
        nid[0] += 1; return f"obj-{nid[0]}"
    def add(**kw):
        b = {"id": new_id(), **kw}; boxes.append({"box": b}); return b["id"]
    def obj(text, x, y, ins=1, outs=1, otypes=None):
        return add(maxclass="newobj", text=text, numinlets=ins, numoutlets=outs,
                   outlettype=otypes if otypes is not None else [""] * outs,
                   patching_rect=[x, y, max(60, 7 * len(text)), 22])
    def link(s, so, d, di):
        lines.append({"patchline": {"source": [s, so], "destination": [d, di],
                                    "destination_inlet": di}})
    def panel(x, y, w, h):
        add(maxclass="panel", background=1, mode=0, rounded=0, proportion=0.39,
            bgcolor=[0.0, 0.0, 0.0, 1.0], numinlets=1, numoutlets=0,
            patching_rect=[x - 4000, y - 900, w, h],
            presentation=1, presentation_rect=[x, y, w, h])
    def label(text, x, y, w, size=9.0, just=1, h=LBL_H + 3):
        add(maxclass="comment", text=text, numinlets=1, numoutlets=0, outlettype=[],
            fontname=FONT, fontsize=size, textcolor=WHITE, textjustification=just,
            patching_rect=[x - 4000, y - 900, w, h],
            presentation=1, presentation_rect=[x, y, w, h])

    def control(ln, sn, pos, mx, widget, px, py):
        """Cree le widget adapte et renvoie son id."""
        init = value_at(SYNLEAD, pos)
        common = dict(numinlets=1, parameter_enable=1, presentation=1, varname=ln)
        va = {"parameter_longname": ln, "parameter_shortname": sn,
              "parameter_mmax": float(mx) if widget == "dial" else mx,
              "parameter_unitstyle": 0, "parameter_linknames": 1,
              "parameter_initial_enable": 1, "parameter_initial": [init]}
        if widget == "dial":
            va["parameter_type"] = 1
            va["parameter_speedlim"] = 50.0
            return add(maxclass="live.dial", numoutlets=2, outlettype=["", "float"],
                       shownumber=0, showname=0, fontsize=8.0, textcolor=WHITE, tricolor=TRI,
                       activedialcolor=BLUE, activeneedlecolor=WHITE,
                       patching_rect=[pos * 70 - 5000, -700.0, DIAL_W, DIAL_H],
                       presentation_rect=[px, py, DIAL_W, DIAL_H],
                       saved_attribute_attributes={"valueof": va}, **common)
        if widget == "numbox":
            va["parameter_type"] = 1
            return add(maxclass="live.numbox", numoutlets=2, outlettype=["", "float"],
                       activebgcolor=BLUE, lcdcolor=GREEN,
                       patching_rect=[pos * 70 - 5000, -700.0, BOX_W, BOX_H],
                       presentation_rect=[px, py, BOX_W, BOX_H],
                       saved_attribute_attributes={"valueof": va}, **common)
        va["parameter_type"] = 2
        va["parameter_enum"] = ["val1", "val2"]
        va.pop("parameter_unitstyle")
        return add(maxclass="live.text", numoutlets=2, outlettype=["", ""],
                   text="Off", texton="On", activebgcolor=AMBER,
                   activebgoncolor=GREEN, lcdcolor=GREEN,
                   patching_rect=[pos * 70 - 5000, -700.0, BOX_W, BOX_H],
                   presentation_rect=[px, py, BOX_W, BOX_H],
                   saved_attribute_attributes={"valueof": va}, **common)

    ids = {}
    # ---------- colonne de gauche ----------
    LEFT_W = 104
    panel(0, 0, LEFT_W, 158)
    label("KORG Z3", 6, 3, 92, 12.0, 0, 17)
    label("Editor", 6, 21, 92, 9.0, 0, 14)
    bd = add(maxclass="button", numoutlets=1, outlettype=["bang"],
             patching_rect=[-4600, -860, 24, 24], presentation=1,
             presentation_rect=[8, 44, 24, 24])
    label("DUMP", 36, 48, 60, 9.0, 0)
    bs = add(maxclass="button", numoutlets=1, outlettype=["bang"],
             patching_rect=[-4600, -820, 24, 24], presentation=1,
             presentation_rect=[8, 74, 24, 24])
    label("SEND", 36, 78, 60, 9.0, 0)
    label("Name", 6, 106, 92, 9.0, 0, 15)
    te = add(maxclass="textedit", keymode=1, text="SynLead ", numinlets=1,
             numoutlets=4, outlettype=["", "int", "", ""], fontsize=9.0,
             patching_rect=[-4600, -780, 90, 20], presentation=1,
             presentation_rect=[8, 122, 88, 19])

    # ---------- helpers de placement ----------
    GLOB = {g[0].split("Z3 ")[1]: g for g in GLOBAL}
    OSCP = {o[0]: o for o in OSC_PARAMS}

    def wave_ui(mode, px, py):
        return add(maxclass="jsui", filename="z3_wave.js", jsarguments=[mode],
                   numinlets=1, numoutlets=0, outlettype=[], parameter_enable=0,
                   patching_rect=[-4800, -600 - (0 if mode == "lfo" else 60), 44, 39],
                   presentation=1, presentation_rect=[px, py, WAVE_W - 4, DIAL_H])

    def algo_ui(px, py, h):
        return add(maxclass="jsui", filename="z3_algo.js",
                   numinlets=1, numoutlets=0, outlettype=[], parameter_enable=0,
                   patching_rect=[-4800, -520, 72, 77],
                   presentation=1, presentation_rect=[px, py, ALGO_W - 6, h])

    def place(entry, pos, cx, row, cell=None):
        """Pose libelle + controle dans une cellule ; renvoie l'id du controle."""
        ln, sn, mx, w, lab = entry
        cell = cell or COL
        ly, cy = [(ROW1_L, ROW1_C), (ROW2_L, ROW2_C), (ROW3_L, ROW3_C)][row]
        cw = DIAL_W if w == "dial" else BOX_W
        if row == 0 and w != "dial":
            cy += (DIAL_H - BOX_H) / 2
        label(lab, cx, ly, cell - 6)
        ids[pos] = control(ln, sn, pos, mx, w, cx + (cell - 6 - cw) / 2, cy)
        return ids[pos]

    # ---------- panneau GLOBAL : algo + 3 colonnes + forme d'onde LFO ----------
    gx = LEFT_W + PAD
    GW = ALGO_W + 3 * COL + WAVE_W + 12
    panel(gx, 0, GW, 158)
    label("GLOBAL", gx + 6, 3, GW - 12, 10.0, 0, 16)
    # colonne algorithme : schema sur la hauteur des deux premieres rangees
    ax = gx + 6
    label("Algorithm", ax, ROW1_L, ALGO_W - 6)
    algo_disp = algo_ui(ax, ROW1_C, ROW2_C + BOX_H - ROW1_C)
    ln, sn, pos, mx, w, lab = GLOB["Algorithm"]
    place((ln, sn, mx, w, ""), pos, ax, 2, ALGO_W)
    algo_pos = pos
    # trois colonnes de parametres
    cx0 = ax + ALGO_W
    ROWS_G = [["Feedback", "LFO Rate", "PMD"], ["PMS", "AMD", "AMS"]]
    for r, names in enumerate(ROWS_G):
        for cidx, nm in enumerate(names):
            ln, sn, pos, mx, w, lab = GLOB[nm]
            place((ln, sn, mx, w, lab), pos, cx0 + cidx * COL, r)
    # colonne forme d'onde du LFO
    wx = cx0 + 3 * COL
    label("LFO Wave", wx, ROW1_L, WAVE_W - 6)
    lfo_ui = wave_ui("lfo", wx, ROW1_C)
    ln, sn, pos, mx, w, lab = GLOB["LFO Wave"]
    place((ln, sn, mx, w, ""), pos, wx, 1, WAVE_W)
    lfo_pos = pos

    # ---------- 4 panneaux oscillateurs : 6 colonnes + colonne forme d'onde ----
    ox0 = gx + GW + PAD
    OW = 6 * COL + WAVE_W + 12
    ROWS_O = [["Level", "Attack", "Decay1", "Sustain", "Decay2", "Release"],
              ["Mult1", "Mult2", "Detune1", "Detune2", "KScale", "EG"],
              ["RateMod", "VeloInt", "KTrack", "AMS", "Reverb"]]
    wave_uis = {}
    for k, oname in enumerate(OSC):
        ox = ox0 + k * (OW + PAD)
        panel(ox, 0, OW, 158)
        role = "modulateur" if oname.startswith("M") else "porteuse"
        label(f"{oname}   {role}", ox + 6, 3, OW - 12, 10.0, 0, 16)
        for r, names in enumerate(ROWS_O):
            for cidx, suf in enumerate(names):
                s_, base, mx, w, lab = OSCP[suf]
                pos = base + k
                place((f"Z3 {oname} {suf}", f"{oname}{suf}"[:13], mx, w, lab),
                      pos, ox + 6 + cidx * COL, r)
        # colonne de droite : graphique en rangee 1, valeur juste dessous
        wx = ox + 6 + 6 * COL
        label("Wave", wx, ROW1_L, WAVE_W - 6)
        s_, base, mx, w, lab = OSCP["Wave"]
        pos = base + k
        wave_uis[pos] = wave_ui("osc", wx, ROW1_C)
        place((f"Z3 {oname} Wave", f"{oname}Wave", mx, w, ""), pos, wx, 1, WAVE_W)
    wave_uis[lfo_pos] = lfo_ui
    wave_uis[algo_pos] = algo_disp

    total_w = ox0 + 4 * (OW + PAD)

    # ---------- logique ----------
    X, Y = 60.0, 400.0
    nd = obj("node.script korgz3_bridge.js @autostart 1", X, Y, 1, 2)
    rt = obj("route sysex load setname status", X, Y + 40, 1, 5)
    it = obj("iter", X, Y + 80, 1, 1, ["int"])
    mo = obj("midiout", X, Y + 120, 1, 0, [])
    link(nd, 0, rt, 0); link(rt, 0, it, 0); link(it, 0, mo, 0)
    unp = obj("unpack " + " ".join(["0"] * len(PARAMS)), X + 240, Y + 80,
              1, len(PARAMS), ["int"] * len(PARAMS))
    link(rt, 1, unp, 0)
    pn = obj("prepend set", X + 140, Y + 120, 1, 1)
    link(rt, 2, pn, 0); link(pn, 0, te, 0)
    link(rt, 3, obj("print Z3_STATUS", X + 380, Y + 120, 1, 0, []), 0)
    link(rt, 4, obj("print Z3_BRIDGE", X + 500, Y + 120, 1, 0, []), 0)
    for i, (ln, sn, pos, mx, w, lab) in enumerate(PARAMS):
        cid = ids[pos]
        pp = obj(f"prepend param {pos}", pos * 70 - 5000, -620.0, 1, 1)
        link(cid, 0, pp, 0); link(pp, 0, nd, 0)
        ps = obj("prepend set", pos * 70 - 5000, Y + 220, 1, 1)
        link(unp, i, ps, 0); link(ps, 0, cid, 0)
        if pos in wave_uis:                 # afficheur : suit le dump ET l'edition
            link(unp, i, wave_uis[pos], 0)
            link(cid, 0, wave_uis[pos], 0)
    md = add(maxclass="message", text="dump", numinlets=2, numoutlets=1,
             outlettype=[""], patching_rect=[-4600, -740, 50, 22])
    ms = add(maxclass="message", text="send", numinlets=2, numoutlets=1,
             outlettype=[""], patching_rect=[-4600, -700, 50, 22])
    link(bd, 0, md, 0); link(md, 0, nd, 0)
    link(bs, 0, ms, 0); link(ms, 0, nd, 0)
    pnm = obj("prepend name", -4600, -660, 1, 1)
    link(te, 0, pnm, 0); link(pnm, 0, nd, 0)
    lb = obj("loadbang", X + 660, Y, 1, 1, ["bang"])
    mw = add(maxclass="message", text="--- KorgZ3 Editor charge ---", numinlets=2,
             numoutlets=1, outlettype=[""], patching_rect=[X + 660, Y + 40, 190, 22])
    link(lb, 0, mw, 0)
    link(mw, 0, obj("print Z3_LOADED", X + 660, Y + 80, 1, 0, []), 0)

    parameters = {}
    for bx in boxes:
        b = bx["box"]
        if str(b.get("maxclass", "")).startswith("live."):
            v = b["saved_attribute_attributes"]["valueof"]
            parameters[b["id"]] = [v["parameter_longname"], v["parameter_shortname"], 0]

    return {"patcher": {
        "fileversion": 1,
        "appversion": {"major": 8, "minor": 1, "revision": 0,
                       "architecture": "x64", "modernui": 1},
        "classnamespace": "box",
        "rect": [40.0, 80.0, 1300.0, 700.0],
        "openrect": [0.0, 0.0, 0.0, 169.0],
        "bglocked": 0, "openinpresentation": 1,
        "default_fontsize": 10.0, "default_fontface": 0, "default_fontname": FONT,
        "gridonopen": 1, "gridsize": [8.0, 8.0], "gridsnaponopen": 1,
        "objectsnaponopen": 1, "statusbarvisible": 2, "toolbarvisible": 1,
        "lefttoolbarpinned": 0, "toptoolbarpinned": 0, "righttoolbarpinned": 0,
        "bottomtoolbarpinned": 0, "toolbars_unpinned_last_save": 0, "tallnewobj": 0,
        "boxanimatetime": 500, "enablehscroll": 1, "enablevscroll": 1,
        "devicewidth": 0.0, "description": "", "digest": "", "tags": "",
        "style": "", "subpatcher_template": "",
        "boxes": boxes, "lines": lines, "parameters": parameters,
        "latency": 0, "autosave": 0,
    }}, total_w


def wrap_amxd(patcher_json_bytes, fname="KorgZ3_Editor_PEG.amxd"):
    stored = patcher_json_bytes + b"\r\n\x00"
    def sub(tag, data):
        return tag + struct.pack(">I", 8 + len(data)) + data
    fn = fname.encode()
    fn += b"\x00" * (4 - len(fn) % 4 if len(fn) % 4 else 4)
    dire_body = (sub(b"type", b"JSON") + sub(b"fnam", fn)
                 + sub(b"sz32", struct.pack(">I", len(stored)))
                 + sub(b"of32", struct.pack(">I", 16))
                 + sub(b"vers", struct.pack(">I", 0))
                 + sub(b"flag", struct.pack(">I", 17))
                 + sub(b"mdat", struct.pack(">I", int(time.time()) & 0xFFFFFFFF)))
    dire = b"dire" + struct.pack(">I", 8 + len(dire_body)) + dire_body
    dlst = b"dlst" + struct.pack(">I", 8 + len(dire)) + dire
    ptch = (b"mx@c" + struct.pack(">I", 16) + struct.pack(">I", 0)
            + struct.pack(">I", 16 + len(stored)) + stored + dlst)
    return (b"ampf" + struct.pack("<I", 4) + b"mmmm"
            + b"meta" + struct.pack("<I", 4) + struct.pack("<I", 7)
            + b"ptch" + struct.pack("<I", len(ptch)) + ptch)


if __name__ == "__main__":
    dst = sys.argv[1] if len(sys.argv) > 1 else "KorgZ3_Editor.amxd"
    doc, w = build_patcher()
    out = wrap_amxd(json.dumps(doc, indent=1).encode())
    open(dst, "wb").write(out)
    p = doc["patcher"]
    from collections import Counter
    cnt = Counter(b["box"]["maxclass"] for b in p["boxes"])
    print(f"ecrit : {dst} ({len(out)} o) | presentation {w:.0f} x 158 px")
    print(f"  {len(p['boxes'])} boxes, {len(p['lines'])} cordons, {len(p['parameters'])} parametres Live")
    print("  widgets :", {k: v for k, v in cnt.items() if k.startswith('live.') or k in ('comment','panel')})
