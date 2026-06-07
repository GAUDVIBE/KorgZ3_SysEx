import re, json, pathlib, math
from sexpr import find_blocks, block_at, prop, rot

TPL = "/Applications/KiCad/KiCad.app/Contents/SharedSupport/template/Arduino_Mega/Arduino_Mega.kicad_pcb"
txt = pathlib.Path(TPL).read_text()
nets = {int(m.group(1)): m.group(2) for m in re.finditer(r'\(net (\d+) "([^"]*)"', txt)}

def clean(sig):  # "/A0"->"A0", "/SCL{slash}21"->"D21", "/RX0{slash}0"->"D0", "/*13"->"D13"
    if sig is None: return None
    s = sig.lstrip('/').lstrip('*')
    if "{slash}" in s:
        s = s.split("{slash}")[-1]
        return "D"+s if s.isdigit() else s
    if s.isdigit(): return "D"+s
    return s

out = []
for a,b in find_blocks(txt, "(footprint "):
    blk = txt[a:b]
    ref = prop(blk, "Reference")
    if ref not in {"J1","J2","J3","J4","J5","J6","J7"}: continue
    ox,oy,orot = block_at(blk)
    pads = []
    for pa,pb in find_blocks(blk, "(pad "):
        pblk = blk[pa:pb]
        num = re.search(r'\(pad "([^"]*)"', pblk).group(1)
        pat = re.search(r'\(at ([-\d.]+) ([-\d.]+)', pblk)
        nm  = re.search(r'\(net (\d+)', pblk)
        sig = clean(nets.get(int(nm.group(1)),"")) if nm else None
        dx,dy = rot(float(pat.group(1)), float(pat.group(2)), orot)
        pads.append(dict(num=num, signal=sig, dx=round(dx,3), dy=round(dy,3)))
    fp = re.search(r'\(footprint "([^"]+)"', blk).group(1)
    out.append(dict(ref=ref, footprint=fp, x=ox, y=oy, rot=orot, pads=pads))

pathlib.Path("shield_redesign/mega_geometry.json").write_text(json.dumps(out, indent=1))
print("extrait:", [(h["ref"], len(h["pads"])) for h in out])
