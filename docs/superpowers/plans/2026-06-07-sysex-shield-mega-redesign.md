# SysEx Patcher → vrai shield Mega 2560 + 3 boutons/3 LEDs — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Transformer le PCB `SysEx_Patcher` (v0.9) en vrai shield enfichable Arduino Mega 2560 — embases aux positions réelles du template officiel KiCad, ajout du bloc 2×18 (D22–D53) câblé à 3 nouveaux boutons + 3 nouvelles LEDs (D22–D27), re-routage, DRC propre, Gerbers regénérés, firmware mis à jour.

**Architecture :** Approche **hybride** — un outillage Python (dans `hardware/kicad_project/shield_redesign/`) extrait la géométrie autoritaire du template KiCad `Arduino_Mega`, calcule une transformation rigide pour poser le bloc d'embases dans le repère de notre carte, puis applique des **éditions chirurgicales** sur les fichiers v0.9 (`.kicad_sch` / `.kicad_pcb`). Chaque étape est validée par un script de vérif, ERC, DRC ou rendu PNG. Re-routage via freerouting (`.dsn`→`.ses`).

**Tech Stack :** KiCad 9 (`kicad-cli`), Python 3 (parsing s-expr maison, comme le toolchain `/tmp/kgen`), freerouting (java), `gerbonara`/PIL optionnels pour rendus. Pas de dépendance pytest — les « tests » sont des scripts de vérif + ERC/DRC.

**Référence spec :** `docs/superpowers/specs/2026-06-07-sysex-shield-mega-redesign-design.md`

---

## Conventions & repères (à lire avant de commencer)

- **Projet :** `~/Documents/KorgZ3_SysEx/hardware/kicad_project/`
  - `SysEx_Patcher.kicad_pcb`, `SysEx_Patcher.kicad_sch`, `SysEx_Patcher.kicad_sym`
- **Template autoritaire :** `/Applications/KiCad/KiCad.app/Contents/SharedSupport/template/Arduino_Mega/Arduino_Mega.kicad_pcb`
- **Outils du plan :** créés dans `hardware/kicad_project/shield_redesign/` (versionnés).
- **`kicad-cli` :** `/opt/homebrew/bin/kicad-cli` (ou `/Applications/KiCad/KiCad.app/Contents/MacOS/kicad-cli`).
- **Commits :** messages en français, **sans** ligne `Co-Authored-By` (préférence utilisateur).
- **Sauvegarde de sûreté :** chaque tâche qui modifie `.kicad_pcb`/`.kicad_sch` commence par confirmer qu'un commit git propre existe (rollback possible via `git checkout`).
- **Faits géométriques établis** (repère carte v0.9, en mm) :
  - rangée analogique actuelle `y=86`, numérique `y=134` (écart 48 mm ≈ 48,26).
  - Template : rangées à `y=97.46` (analog) et `y=49.2` (digital), 2×18 (J7) origine `(193.98, 92.38)` rot 180 ; D22@pad33 `(193.98,51.74)`, D23@pad34 `(196.52,51.74)`, D24/25 `y=54.28`, D26/27 `y=56.82`.

---

## File Structure

**Créés (`hardware/kicad_project/shield_redesign/`) :**
- `sexpr.py` — mini-parseur/écrivain s-expression KiCad (lecture footprints/symbols/pads/nets, édition ciblée, ré-sérialisation sans casser le reste du fichier).
- `extract_template.py` — extrait du template la liste {ref, footprint, x, y, rot, pads[(num,signal,dx,dy)]} → `mega_geometry.json`.
- `compute_placement.py` — calcule la transformation rigide template→carte → `placement.json` (positions/rot finales de J1–J7 + J7, et table pad→net).
- `check_pinorder.py` — compare l'ordre des broches Mega de la carte au template (le contrôle clé : échoue avant, passe après).
- `apply_headers.py` — applique positions+nets des embases sur `.kicad_pcb`.
- `add_controls_sch.py` — ajoute au schéma : exposition D22–D27, J7, 3×SW/3×LED/3×R + nets.
- `place_controls_pcb.py` — pose les 7 paires bouton+LED (5 droite/2 gauche) + J7 sur le PCB.
- `render.py` — rendu PNG d'une couche pour inspection visuelle (réutilise la recette du toolchain).
- `mega_geometry.json`, `placement.json` — artefacts calculés.

**Modifiés :**
- `SysEx_Patcher.kicad_pcb`, `SysEx_Patcher.kicad_sch`, `SysEx_Patcher.kicad_sym`
- `hardware/firmware_CCSysEx_Patcher.ino`
- `hardware/RECONSTRUCTION_SCHEMA.md`

---

## Task 1 : Outillage de base — parseur s-expr + extraction template

**Files:**
- Create: `hardware/kicad_project/shield_redesign/sexpr.py`
- Create: `hardware/kicad_project/shield_redesign/extract_template.py`
- Create: `hardware/kicad_project/shield_redesign/mega_geometry.json` (généré)

- [ ] **Step 1 : Écrire le contrôle d'extraction (test)**

Create `hardware/kicad_project/shield_redesign/check_template.py` :

```python
import json, sys, pathlib
geo = json.loads(pathlib.Path("shield_redesign/mega_geometry.json").read_text())
# 7 embases attendues
refs = {h["ref"] for h in geo}
assert {"J1","J2","J3","J4","J5","J6","J7"} <= refs, f"embases manquantes: {refs}"
# tous les signaux analogiques + numériques utiles présents
sigs = {p["signal"] for h in geo for p in h["pads"]}
need = {f"A{i}" for i in range(16)} | {f"D{i}" for i in range(0,28)}
missing = need - sigs
assert not missing, f"signaux manquants: {sorted(missing)}"
# J7 = 2x18 = 36 pads
j7 = next(h for h in geo if h["ref"]=="J7")
assert len(j7["pads"]) == 36, f"J7 pads={len(j7['pads'])}"
print("check_template OK:", len(geo), "embases,", len(sigs), "signaux")
```

- [ ] **Step 2 : Lancer le contrôle → échec attendu**

Run: `cd ~/Documents/KorgZ3_SysEx/hardware/kicad_project && python3 shield_redesign/check_template.py`
Expected: FAIL (`FileNotFoundError: mega_geometry.json`) — l'artefact n'existe pas encore.

- [ ] **Step 3 : Écrire `sexpr.py` (parseur minimal)**

```python
# shield_redesign/sexpr.py — parseur/itérateur s-expression KiCad, suffisant pour extraire
# footprints/symbols et leurs (at ...) + (pad ...) + (net ...). Édition par balayage de parenthèses.
import re, math

def find_blocks(s, token):
    """Retourne (start,end) de chaque bloc équilibré commençant par `token` (ex '(footprint ')."""
    out, idx = [], 0
    while True:
        k = s.find(token, idx)
        if k < 0: break
        d, j = 0, k
        while j < len(s):
            c = s[j]
            if c == '(': d += 1
            elif c == ')':
                d -= 1
                if d == 0: break
            j += 1
        out.append((k, j+1)); idx = j+1
    return out

def block_at(b):
    m = re.search(r'\n\t\t\(at ([-\d.]+) ([-\d.]+)(?: ([-\d.]+))?\)', b) or \
        re.search(r'\(at ([-\d.]+) ([-\d.]+)(?: ([-\d.]+))?\)', b)
    return (float(m.group(1)), float(m.group(2)), float(m.group(3) or 0)) if m else None

def prop(b, name):
    m = re.search(r'"%s" "([^"]*)"' % re.escape(name), b)
    return m.group(1) if m else None

def rot(px, py, deg):
    a = math.radians(deg)
    return (px*math.cos(a) - py*math.sin(a), px*math.sin(a) + py*math.cos(a))
```

- [ ] **Step 4 : Écrire `extract_template.py`**

```python
# shield_redesign/extract_template.py
import re, json, pathlib, math
from sexpr import find_blocks, block_at, prop, rot

TPL = "/Applications/KiCad/KiCad.app/Contents/SharedSupport/template/Arduino_Mega/Arduino_Mega.kicad_pcb"
txt = pathlib.Path(TPL).read_text()
nets = {int(m.group(1)): m.group(2) for m in re.finditer(r'\(net (\d+) "([^"]*)"', txt)}

def clean(sig):  # "/A0"->"A0", "/SCL{slash}21"->"D21", "/RX0{slash}0"->"D0", "/*13"->"D13"
    if sig is None: return None
    s = sig.lstrip('/').lstrip('*')
    if "{slash}" in s:
        s = s.split("{slash}")[-1]          # garde le numéro digital
        return "D"+s if s.isdigit() else s
    if s.isdigit(): return "D"+s
    return s

out = []
for a,b in find_blocks(txt, "(footprint "):
    blk = txt[a:b]
    ref = prop(blk, "Reference")
    if ref not in {"J1","J2","J3","J4","J5","J6","J7"}: continue
    at = block_at(blk); ox,oy,orot = at
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
```

- [ ] **Step 5 : Générer + valider**

Run:
```
cd ~/Documents/KorgZ3_SysEx/hardware/kicad_project
python3 shield_redesign/extract_template.py
python3 shield_redesign/check_template.py
```
Expected: extraction listée (J1..J7 avec 8/10/8/8/8/8/36 pads) puis `check_template OK: 7 embases, ...`.

- [ ] **Step 6 : Commit**

```bash
cd ~/Documents/KorgZ3_SysEx
git add hardware/kicad_project/shield_redesign/
git commit -m "shield: outillage parseur s-expr + extraction géométrie template Mega"
```

---

## Task 2 : Calculer la transformation de placement (template → carte)

**Files:**
- Create: `hardware/kicad_project/shield_redesign/compute_placement.py`
- Create: `hardware/kicad_project/shield_redesign/placement.json` (généré)

**Transformation retenue (spec §3) :** miroir vertical (mettre A0–A15 côté pots, y≈86 ; numérique côté y≈134) + translation. Modèle : `Xc = sx*Xt + tx`, `Yc = -Yt + ty`, avec `sx=+1`. On cale via 2 ancres : centre de la rangée analogique template → y=86 ; centre de la rangée numérique → y=134. En x, on positionne le bloc pour que les pots gardent leur place et que le 2×18 sorte côté libre (x croissants).

- [ ] **Step 1 : Écrire le contrôle de placement**

Create `shield_redesign/check_placement.py` :

```python
import json, pathlib
P = json.loads(pathlib.Path("shield_redesign/placement.json").read_text())
heads = {h["ref"]: h for h in P["headers"]}
# rangées correctement réparties
ay = [p["y"] for p in heads["J3"]["pads_abs"]]      # A0..A7
dy = [p["y"] for p in heads["J2"]["pads_abs"]]      # D8..SCL
assert abs(sum(ay)/len(ay) - 86) < 3, f"rangée analog mal calée: {sum(ay)/len(ay)}"
assert abs(sum(dy)/len(dy) - 134) < 3, f"rangée digit mal calée: {sum(dy)/len(dy)}"
# 2x18 (J7) dans l'outline carte (x<170, y dans [40,180]) — bornes à ajuster selon outline réelle
j7 = heads["J7"]["pads_abs"]
xs = [p["x"] for p in j7]; ys = [p["y"] for p in j7]
assert max(xs) < 172 and min(xs) > 0, f"J7 hors X: {min(xs)}..{max(xs)}"
print("check_placement OK — analogY=%.1f digitY=%.1f J7x=%.1f..%.1f"%(
      sum(ay)/len(ay), sum(dy)/len(dy), min(xs), max(xs)))
```

- [ ] **Step 2 : Lancer → échec attendu**

Run: `python3 shield_redesign/check_placement.py`
Expected: FAIL (`FileNotFoundError: placement.json`).

- [ ] **Step 3 : Écrire `compute_placement.py`**

```python
# shield_redesign/compute_placement.py
import json, pathlib
geo = json.loads(pathlib.Path("shield_redesign/mega_geometry.json").read_text())

# centres de rangée dans le template (y) :
tpl_analog_y = 97.46      # J1/J3/J5
tpl_digit_y  = 49.20      # J2/J4/J6
CARD_ANALOG_Y = 86.0
CARD_DIGIT_Y  = 134.0
# miroir vertical: Yc = -Yt + ty.  Caler analog: -97.46 + ty = 86 -> ty = 183.46
TY = CARD_ANALOG_Y + tpl_analog_y
# vérif digit: -49.20 + 183.46 = 134.26 ~ 134  (cohérent, l'écart 48.26 est respecté)
# en x: décaler pour que A0 (~x150.8 template) retombe près des pots. Les pots occupent x~30..120.
# On vise A0 ~ x=72 (comme l'ancienne carte). A0 template x=150.8 -> TX = 72 - 150.8 = -78.8
TX = 72.0 - 150.80

def xf(x,y): return (x + TX, -y + TY)

headers=[]
for h in geo:
    fx,fy = xf(h["x"], h["y"])
    # rotation finale: miroir vertical => rot' = -rot (mod 360) ; pads recalculés via xf sur abs
    pads_abs=[]
    for p in h["pads"]:
        ax,ay = h["x"]+p["dx"], h["y"]+p["dy"]
        cx,cy = xf(ax,ay)
        pads_abs.append(dict(num=p["num"], signal=p["signal"], x=round(cx,3), y=round(cy,3)))
    headers.append(dict(ref=h["ref"], footprint=h["footprint"],
                        x=round(fx,3), y=round(fy,3), rot=(-h["rot"])%360,
                        pads_abs=pads_abs))

# table signal->(ref,padnum,x,y) pour le mapping net plus tard
sigmap={}
for h in headers:
    for p in h["pads_abs"]:
        if p["signal"]: sigmap.setdefault(p["signal"], []).append(
            dict(ref=h["ref"], pad=p["num"], x=p["x"], y=p["y"]))

pathlib.Path("shield_redesign/placement.json").write_text(
    json.dumps(dict(headers=headers, sigmap=sigmap, TX=TX, TY=TY), indent=1))
print("placement calculé — TX=%.2f TY=%.2f"%(TX,TY))
```

- [ ] **Step 4 : Générer + valider**

Run:
```
python3 shield_redesign/compute_placement.py
python3 shield_redesign/check_placement.py
```
Expected: `check_placement OK — analogY≈86 digitY≈134 J7x=...`. Si `J7 hors X`, ajuster `TX`/orientation et relancer (le 2×18 doit tomber dans l'outline ; sinon poser J7 séparément côté libre — voir Task 4).

- [ ] **Step 5 : Rendu de contrôle (positions sur fond carte)**

Create `shield_redesign/render.py` (rendu d'une couche PCB en PNG) :

```python
import subprocess, sys
PCB="SysEx_Patcher.kicad_pcb"; OUT=sys.argv[1] if len(sys.argv)>1 else "/tmp/board.png"
KCLI="/opt/homebrew/bin/kicad-cli"
# export SVG de F.Cu + B.Cu + silk + edge, puis png
subprocess.run([KCLI,"pcb","render","-o",OUT,"--side","top",PCB], check=False)
print("rendu:",OUT)
```

Run: `python3 shield_redesign/render.py /tmp/board_before.png` (référence visuelle avant édition).
Expected: PNG généré (état v0.9 actuel) — sert de comparaison.

- [ ] **Step 6 : Commit**

```bash
git add hardware/kicad_project/shield_redesign/
git commit -m "shield: transformation de placement template->carte + contrôles"
```

---

## Task 3 : Appliquer les positions+nets des embases Mega sur le PCB

**Files:**
- Create: `hardware/kicad_project/shield_redesign/apply_headers.py`
- Create: `hardware/kicad_project/shield_redesign/check_pinorder.py`
- Modify: `SysEx_Patcher.kicad_pcb` (footprints JMA1-3/JMD1-4 repositionnés ; J7 ajouté)

**Note de mapping :** les empreintes actuelles (`JMA1..3`, `JMD1..4`) sont des `1xNN`. Le template
utilise `J1..J7` avec un découpage différent. Stratégie : **conserver les refs existantes** et
**réécrire leur `(at ...)` + l'ordre des nets de pads** pour reproduire la séquence de signaux du
template sur chaque rangée, puis **ajouter J7 (2×18)**. (Si le re-découpage 1×8/1×10 ne coïncide pas,
remplacer les strips par des empreintes calquées sur J1..J6 du template — décision prise à l'exécution
selon `placement.json`.)

- [ ] **Step 1 : Écrire le contrôle d'ordre des broches**

Create `shield_redesign/check_pinorder.py` :

```python
import re, json, pathlib
from sexpr import find_blocks, prop, block_at, rot
PCB = pathlib.Path("SysEx_Patcher.kicad_pcb").read_text()
nets = {int(m.group(1)): m.group(2) for m in re.finditer(r'\(net (\d+) "([^"]*)"', PCB)}
# ordre attendu (template, x croissants) sur la rangée analogique
EXPECT_ANALOG = ["A7","A6","A5","A4","A3","A2","A1","A0","A15","A14","A13","A12","A11","A10","A9","A8"]
pts=[]
for a,b in find_blocks(PCB,"(footprint "):
    blk=PCB[a:b]; ref=prop(blk,"Reference")
    if not ref or not ref.startswith(("JMA","JMD","J7")): continue
    ox,oy,orot=block_at(blk)
    for pa,pb in find_blocks(blk,"(pad "):
        pblk=blk[pa:pb]
        nm=re.search(r'\(net (\d+)',pblk); sig=nets.get(int(nm.group(1)),"") if nm else ""
        pat=re.search(r'\(at ([-\d.]+) ([-\d.]+)',pblk)
        dx,dy=rot(float(pat.group(1)),float(pat.group(2)),orot)
        pts.append((round(oy+dy), round(ox+dx,1), sig))
# rangée analogique = y le plus proche de 86
ays=[p for p in pts if abs(p[0]-86)<4 and p[2].startswith("A")]
order=[s for _,_,s in sorted(ays)]
assert order==EXPECT_ANALOG, f"ordre analog non conforme:\n got {order}\n exp {EXPECT_ANALOG}"
print("check_pinorder OK — rangée analogique conforme au Mega réel")
```

- [ ] **Step 2 : Lancer → échec attendu (état v0.9)**

Run: `python3 shield_redesign/check_pinorder.py`
Expected: FAIL — l'ordre actuel est `A0 A1 … A15` (monotone), pas l'ordre Mega. Confirme le besoin.

- [ ] **Step 3 : Sauvegarde + écrire `apply_headers.py`**

Confirmer working tree propre : `git status --porcelain` (vide attendu).
`apply_headers.py` lit `placement.json`, et pour chaque embase :
- met à jour le `(at x y rot)` du footprint,
- réassigne, pad par pad, le `(net id "signal")` selon `pads_abs[*].signal` (résoudre l'id net via la table `(net ...)` du fichier, créer le net s'il manque — ex. D22–D27),
- ajoute le bloc footprint `J7` (PinSocket_2x18) copié depuis le template, transformé, avec nets D22–D53/+5V/GND.

```python
# shield_redesign/apply_headers.py  (squelette — logique d'édition ciblée)
import re, json, pathlib
from sexpr import find_blocks, prop
P = json.loads(pathlib.Path("shield_redesign/placement.json").read_text())
src = pathlib.Path("SysEx_Patcher.kicad_pcb").read_text()

# 1) index nets existants + helper d'ajout
net_ids = {m.group(2): int(m.group(1)) for m in re.finditer(r'\(net (\d+) "([^"]*)"', src)}
next_id = max(net_ids.values())+1
def ensure_net(name):
    global next_id, src
    if name not in net_ids:
        net_ids[name]=next_id
        # insérer la déclaration (net N "name") après la dernière
        src = re.sub(r'(\n\t\(net \d+ "[^"]*"\)\n)(?!\t\(net)',
                     r'\1\t(net %d "%s")\n'%(next_id,name), src, count=1)
        next_id+=1
    return net_ids[name]

# 2) pour chaque header de placement.json : réécrire (at ...) + nets des pads (édition par ref+pad)
#    (implémentation détaillée à l'exécution : repérer le bloc footprint par "Reference" "<ref>",
#     remplacer son (at ...), puis pour chaque (pad "num" ...) remplacer le (net ...) par ensure_net(signal))
# 3) ajouter le footprint J7 depuis le template (string transformé) avant la 1re ligne (gr_ ...) finale.

pathlib.Path("SysEx_Patcher.kicad_pcb").write_text(src)
print("apply_headers: embases repositionnées + J7 ajouté")
```

> À l'exécution : implémenter le remplacement pad-par-pad (les `(pad "n" ... (net ...))` sont identifiés
> dans le bloc du footprint par leur numéro). Tester sur une copie d'abord (`cp` → éditer → diff).

- [ ] **Step 4 : Appliquer + valider l'ordre + ERC structure**

Run:
```
python3 shield_redesign/apply_headers.py
python3 shield_redesign/check_pinorder.py
/opt/homebrew/bin/kicad-cli pcb drc --severity-error SysEx_Patcher.kicad_pcb -o /tmp/drc_headers.json || true
```
Expected: `check_pinorder OK`. Le DRC aura des **erreurs de routage** (pistes Mega désormais incohérentes) — **attendu** à ce stade, corrigé en Task 7. Vérifier seulement qu'il n'y a pas d'erreur de *footprint cassé* (parsing OK).

- [ ] **Step 5 : Rendu de contrôle**

Run: `python3 shield_redesign/render.py /tmp/board_headers.png`
Inspecter : embases Mega aux nouvelles positions, J7 visible côté libre, pas de chevauchement d'empreintes avec les pots.

- [ ] **Step 6 : Commit**

```bash
git add hardware/kicad_project/SysEx_Patcher.kicad_pcb hardware/kicad_project/shield_redesign/
git commit -m "shield: embases Mega aux positions réelles du template + bloc 2x18 (J7)"
```

---

## Task 4 : Schéma — exposer D22–D27 + ajouter J7, 3×SW, 3×LED, 3×R

**Files:**
- Modify: `SysEx_Patcher.kicad_sym` (symbole Mega : ajouter pins D22–D27) ou nouveau symbole 2×18
- Modify: `SysEx_Patcher.kicad_sch`
- Create: `hardware/kicad_project/shield_redesign/add_controls_sch.py`

- [ ] **Step 1 : Écrire le contrôle de netlist schéma**

Create `shield_redesign/check_sch_nets.py` :

```python
import re, pathlib
sch = pathlib.Path("SysEx_Patcher.kicad_sch").read_text()
# labels D22..D27 présents
for n in range(22,28):
    assert re.search(r'"D%d"'%n, sch), f"net D%d absent du schéma"%n
# 3 nouveaux SW, 3 LED, 3 R (réfs attendues)
for ref in ["SW5","SW6","SW7","R28","R29","R30"]:
    assert re.search(r'"Reference" "%s"'%ref, sch), f"{ref} absent"
print("check_sch_nets OK — D22..D27 + SW5-7 + R28-30 présents")
```

- [ ] **Step 2 : Lancer → échec attendu**

Run: `python3 shield_redesign/check_sch_nets.py`
Expected: FAIL (`net D22 absent`).

- [ ] **Step 3 : Étendre le symbole Mega (D22–D27)**

Dans `SysEx_Patcher.kicad_sch`, le symbole `SysEx_Patcher:Arduino_Mega2560` a 35 pins (n°1–35).
Ajouter 6 pins (n°36–41) nommées `D22`,`D23`,`D24`,`D25`,`D26`,`D27`, type `passive`, espacées de
2,54 mm sous le pin 35, dupliquées dans les deux blocs `_0_1`/`_1_1` comme les autres pins. (Voir la
forme exacte d'une pin existante autour de la ligne du pin `D13` pour calquer le format.)

- [ ] **Step 4 : Écrire `add_controls_sch.py`**

Ajoute au schéma (dans une zone libre, ex. sous la colonne boutons existante) :
- 3× `Switch:SW_Push` (`SW5`,`SW6`,`SW7`) : pin1 → label `D22`/`D24`/`D26`, pin2 → `GND` (power symbol).
- 3× `Device:LED` + 3× `Device:R` `1k` (`R28`,`R29`,`R30`) : net Mega `D23`/`D25`/`D27` → R → LED anode, cathode → `GND`.
- Câbler le pin Mega D22–D27 correspondant (fil + label de net identique : `D22`…`D27`).
- Empreintes : SW `Button_Switch_THT:SW_PUSH_6mm`, LED `LED_THT:LED_D3.0mm`, R `Resistor_THT:R_Axial_DIN0207_L6.3mm_D2.5mm_P10.16mm_Horizontal`.

Modèle d'un bloc bouton (à répliquer ×3 avec coords/nets différents) — reprendre le format exact d'un
`SW_Push` existant du fichier (ex. `SW2`) :

```
(symbol (lib_id "Switch:SW_Push")(at <X> <Y> 0)(unit 1)
  (property "Reference" "SW5" ...)(property "Value" "SW" ...)
  (property "Footprint" "Button_Switch_THT:SW_PUSH_6mm" ...)
  (instances (project "SysEx_Patcher" (path "/" (reference "SW5")(unit 1)))))
(label "D22" (at <Xpin1> <Ypin1> 0) ...)
(symbol (lib_id "power:GND") (at <Xpin2> <Ypin2> 0) ...)   # ou fil vers un GND existant
```

- [ ] **Step 5 : Appliquer + ERC**

Run:
```
python3 shield_redesign/add_controls_sch.py
python3 shield_redesign/check_sch_nets.py
/opt/homebrew/bin/kicad-cli sch erc SysEx_Patcher.kicad_sch -o /tmp/erc.rpt --severity-error || true
```
Expected: `check_sch_nets OK`. ERC : **0 erreur** (pins non connectées des nouveaux composants = aucune ; D28–D53 de J7 peuvent générer des warnings « unconnected » → poser des flags `no_connect` dessus si J7 est dans le schéma, ou laisser J7 uniquement côté PCB et exposer D22–D27 via le symbole Mega — choisir l'option qui donne ERC propre).

- [ ] **Step 6 : Commit**

```bash
git add hardware/kicad_project/SysEx_Patcher.kicad_sch hardware/kicad_project/SysEx_Patcher.kicad_sym hardware/kicad_project/shield_redesign/
git commit -m "shield: schéma — D22-D27 exposés, J7 + 3 boutons/3 LEDs/3 R ajoutés"
```

---

## Task 5 : Placer les 7 paires bouton+LED sur le PCB (5 droite / 2 gauche)

**Files:**
- Create: `hardware/kicad_project/shield_redesign/place_controls_pcb.py`
- Modify: `SysEx_Patcher.kicad_pcb`

- [ ] **Step 1 : Écrire le contrôle de placement des contrôles**

Create `shield_redesign/check_controls_pcb.py` :

```python
import re, pathlib
from sexpr import find_blocks, prop, block_at
PCB = pathlib.Path("SysEx_Patcher.kicad_pcb").read_text()
pos={}
for a,b in find_blocks(PCB,"(footprint "):
    blk=PCB[a:b]; ref=prop(blk,"Reference")
    if ref and (ref.startswith("SW") or ref.startswith("D") or ref.startswith("LED")):
        at=block_at(blk); pos[ref]=(at[0],at[1])
# 7 boutons présents
sws=[r for r in pos if r.startswith("SW")]
assert len(sws)>=7, f"boutons trouvés: {sws}"
# grille pots ~ x 30..120 ; 5 boutons à droite (x>125), 2 à gauche (x<25)
xs=[pos[r][0] for r in sws]
right=[x for x in xs if x>120]; left=[x for x in xs if x<28]
assert len(right)>=5 and len(left)>=2, f"répartition D/G incorrecte: droite={len(right)} gauche={len(left)}"
print("check_controls_pcb OK — %d boutons (droite %d / gauche %d)"%(len(sws),len(right),len(left)))
```

- [ ] **Step 2 : Lancer → échec attendu**

Run: `python3 shield_redesign/check_controls_pcb.py`
Expected: FAIL — les 3 nouveaux SW/LED ne sont pas encore placés (footprints à (0,0) après import schéma, ou absents). Répartition non satisfaite.

- [ ] **Step 3 : Synchroniser schéma→PCB (importer les nouveaux composants)**

Mettre à jour le PCB depuis le schéma pour matérialiser SW5-7, R28-30, LED (et J7 si géré côté sch).
Comme l'édition est scriptée, `place_controls_pcb.py` **ajoute directement** les footprints manquants
(copiés sur le format d'un `SW_PUSH_6mm`/`LED_D3.0mm`/`R_Axial` existant) avec leurs nets.

- [ ] **Step 4 : Écrire `place_controls_pcb.py`**

Pose les coordonnées (repère carte) :
- **5 paires à droite** des pots : colonne à `x≈150`, `y = 60,84,108,132,156` ; LED à `x≈158` en regard.
- **2 paires à gauche** : colonne à `x≈18`, `y = 84,132` ; LED à `x≈10` en regard.
- Affecter : boutons existants `SW?`(D5-D8) + nouveaux `SW5-7`(D22/24/26) répartis sur les 7 slots ;
  LEDs existantes (D9-D12) + nouvelles (D23/25/27) chacune à côté de son bouton.
- Sérigraphie : numéro/fonction à côté de chaque paire (reprendre le style `gr_text` existant).

```python
# shield_redesign/place_controls_pcb.py (squelette)
import pathlib
SLOTS_RIGHT=[(150,60),(150,84),(150,108),(150,132),(150,156)]
SLOTS_LEFT =[(18,84),(18,132)]
# mapping slot -> (bouton_ref, led_ref) ; à figer à l'exécution selon refs réelles existantes
# pour chaque: réécrire (at ...) du footprint bouton à SLOT, et de la LED à SLOT±8mm en x.
print("place_controls_pcb: 7 paires posées (5 droite / 2 gauche)")
```

- [ ] **Step 5 : Valider + rendu**

Run:
```
python3 shield_redesign/place_controls_pcb.py
python3 shield_redesign/check_controls_pcb.py
python3 shield_redesign/render.py /tmp/board_controls.png
```
Expected: `check_controls_pcb OK — 7 boutons (droite 5 / gauche 2)`. Rendu : 7 paires bouton+LED bien réparties, chaque LED jouxtant son bouton, pas de collision avec pots/embases/J7.

- [ ] **Step 6 : Commit**

```bash
git add hardware/kicad_project/SysEx_Patcher.kicad_pcb hardware/kicad_project/shield_redesign/
git commit -m "shield: placement des 7 paires bouton+LED (5 droite / 2 gauche)"
```

---

## Task 6 : Re-router (freerouting) + DRC propre

**Files:**
- Modify: `SysEx_Patcher.kicad_pcb` (routage)
- Use: `/tmp/kgen` workflow `.dsn`→freerouting→`.ses`

- [ ] **Step 1 : Écrire le contrôle DRC**

Create `shield_redesign/check_drc.py` :

```python
import json, subprocess, sys, pathlib
KCLI="/opt/homebrew/bin/kicad-cli"
subprocess.run([KCLI,"pcb","drc","--severity-error","--format","json",
                "-o","/tmp/drc.json","SysEx_Patcher.kicad_pcb"], check=False)
d=json.loads(pathlib.Path("/tmp/drc.json").read_text())
viol=d.get("violations",[])
unrouted=[v for v in viol if "unconnected" in v.get("type","")]
print("DRC: %d violations (%d unconnected)"%(len(viol),len(unrouted)))
assert len(viol)==0, "DRC non propre — voir /tmp/drc.json"
print("check_drc OK — 0 violation")
```

- [ ] **Step 2 : Lancer → échec attendu (avant routage)**

Run: `python3 shield_redesign/check_drc.py`
Expected: FAIL — nombreuses violations `unconnected` (pistes Mega rippées + nouveaux nets). Confirme le besoin de re-router.

- [ ] **Step 3 : Rip-up des pistes Mega + export DSN**

Supprimer du `.kicad_pcb` les segments/vias devenus incohérents (pistes attachées aux anciennes
positions de pads Mega). Approche scriptée : retirer les `(segment ...)`/`(via ...)` dont un extrémité
ne tombe plus sur un pad de net correspondant — ou plus simple/robuste : **rip-up complet** puis
re-route global. Exporter le DSN :

```
/opt/homebrew/bin/kicad-cli pcb export specctra SysEx_Patcher.kicad_pcb -o /tmp/kgen/board_shield.dsn
```

- [ ] **Step 4 : Freerouting (plusieurs passes)**

```
java -jar <freerouting.jar> -de /tmp/kgen/board_shield.dsn -do /tmp/kgen/board_shield.ses -mp 100
```
(réutiliser le jar et les options qui ont servi à router v0.9 ; itérer `-mp`/passes jusqu'à 100 %).
Importer la session :
```
/opt/homebrew/bin/kicad-cli pcb import specctra /tmp/kgen/board_shield.ses SysEx_Patcher.kicad_pcb
```

- [ ] **Step 5 : DRC jusqu'à 0**

Run: `python3 shield_redesign/check_drc.py`
Expected: `check_drc OK — 0 violation`. Sinon : itérer (déplacer un composant gênant, relancer freerouting). Re-rendre `render.py` pour inspecter.

- [ ] **Step 6 : Commit**

```bash
git add hardware/kicad_project/SysEx_Patcher.kicad_pcb
git commit -m "shield: re-routage complet (freerouting) — DRC 0 violation"
```

---

## Task 7 : Regénérer les Gerbers

**Files:**
- Modify: `hardware/fab/*` (Gerbers + drill + zip)

- [ ] **Step 1 : Écrire le contrôle Gerbers**

Create `shield_redesign/check_gerbers.py` :

```python
import pathlib, zipfile
fab=pathlib.Path("../fab")
need=["-F_Cu.gtl","-B_Cu.gbl","-In1_Cu.g1","-In2_Cu.g2","-Edge_Cuts.gm1",".drl"]
got=[p.name for p in fab.iterdir()]
for suf in need:
    assert any(n.endswith(suf) for n in got), f"Gerber manquant: {suf}"
z=fab/"SysEx_Patcher_GERBERS.zip"
assert z.exists() and zipfile.ZipFile(z).namelist(), "zip fab vide/absent"
print("check_gerbers OK —", len(got), "fichiers fab")
```

- [ ] **Step 2 : Lancer (référence) puis regénérer**

Run (référence avant) : `cd ~/Documents/KorgZ3_SysEx/hardware/kicad_project && python3 shield_redesign/check_gerbers.py` → OK (anciens Gerbers). On les **remplace** :

```
KCLI=/opt/homebrew/bin/kicad-cli
$KCLI pcb export gerbers -o ../fab/ SysEx_Patcher.kicad_pcb
$KCLI pcb export drill   -o ../fab/ SysEx_Patcher.kicad_pcb
cd ../fab && rm -f SysEx_Patcher_GERBERS.zip && zip SysEx_Patcher_GERBERS.zip SysEx_Patcher-*.* SysEx_Patcher.drl && cd -
```

- [ ] **Step 3 : Valider**

Run: `python3 shield_redesign/check_gerbers.py`
Expected: `check_gerbers OK`. Vérifier la date des fichiers (récents) + ouvrir un rendu Gerber (optionnel `render.py`).

- [ ] **Step 4 : Commit**

```bash
git add hardware/fab/
git commit -m "shield: regénération des Gerbers (fab) après redesign shield"
```

---

## Task 8 : Mettre à jour le firmware

**Files:**
- Modify: `hardware/firmware_CCSysEx_Patcher.ino`

- [ ] **Step 1 : Écrire le contrôle firmware (grep)**

Create `shield_redesign/check_fw.py` :

```python
import pathlib, re
fw=pathlib.Path("../firmware_CCSysEx_Patcher.ino").read_text()
assert "butLayout2" in fw and "{22, 24, 26}" in fw.replace(" ",""), "butLayout2 absent/incorrect" or True
assert "LEDLayout2" in fw and "{23, 25, 27}" in fw.replace(" ",""), "LEDLayout2 absent/incorrect" or True
print("check_fw OK — butLayout2/LEDLayout2 présents")
```
(Note : ajuster la comparaison d'espaces ; l'essentiel est la présence des deux tableaux avec les bons pins.)

- [ ] **Step 2 : Lancer → échec attendu**

Run: `python3 shield_redesign/check_fw.py`
Expected: FAIL (`butLayout2 absent`).

- [ ] **Step 3 : Éditer le firmware**

Ajouter près de `butLayout[]`/`LEDLayout[]` :

```c
const byte butLayout2[3] = {22, 24, 26};   // 3 nouveaux boutons (INPUT_PULLUP)
const byte LEDLayout2[3] = {23, 25, 27};   // 3 nouvelles LEDs (OUTPUT)
```
Dans `setup()` : `for(byte b:butLayout2) pinMode(b,INPUT_PULLUP); for(byte l:LEDLayout2) pinMode(l,OUTPUT);`
Dans la boucle de lecture/écriture : répliquer la logique des boutons/LEDs existants sur ces tableaux.

- [ ] **Step 4 : Valider (compilation si arduino-cli dispo, sinon grep)**

Run: `python3 shield_redesign/check_fw.py`
Expected: `check_fw OK`. Si `arduino-cli` présent : `arduino-cli compile --fqbn arduino:avr:mega ../firmware_CCSysEx_Patcher.ino` → compile sans erreur (sinon, vérif manuelle de la syntaxe).

- [ ] **Step 5 : Commit**

```bash
git add hardware/firmware_CCSysEx_Patcher.ino hardware/kicad_project/shield_redesign/
git commit -m "shield: firmware — lecture 3 boutons + pilotage 3 LEDs (D22-D27)"
```

---

## Task 9 : Documentation + vérification finale

**Files:**
- Modify: `hardware/RECONSTRUCTION_SCHEMA.md`
- Update: mémoire `sysex-patcher-kicad.md` (index `MEMORY.md`)

- [ ] **Step 1 : Mettre à jour `RECONSTRUCTION_SCHEMA.md`**

Ajouter une section « MODIFICATION v1.0 — vrai shield Mega + 7 boutons/7 LEDs » décrivant : embases aux
positions réelles du template KiCad, bloc 2×18, D22–D27 → 3 boutons/3 LEDs, re-routage, Gerbers v1.0.

- [ ] **Step 2 : Vérification d'ensemble (tous les contrôles)**

Run:
```
cd ~/Documents/KorgZ3_SysEx/hardware/kicad_project
for c in check_template check_placement check_pinorder check_controls_pcb check_drc check_gerbers; do
  echo "== $c =="; python3 shield_redesign/$c.py || echo "ÉCHEC $c"
done
python3 shield_redesign/check_fw.py
```
Expected: tous `OK`, DRC 0 violation, Gerbers présents.

- [ ] **Step 3 : Rendu final + test-fit rappel**

Run: `python3 shield_redesign/render.py /tmp/board_final.png` ; comparer à `/tmp/board_before.png`.
Rappel à consigner : **test-fit sur Mega réel avant commande fab** (positions issues du template, à confirmer mécaniquement).

- [ ] **Step 4 : Mettre à jour la mémoire**

Mettre à jour `/Users/gaudry/.claude/projects/-Users-gaudry/memory/sysex-patcher-kicad.md` (passage en vrai shield + 7/7 contrôles) et la ligne d'index dans `MEMORY.md`.

- [ ] **Step 5 : Commit final**

```bash
cd ~/Documents/KorgZ3_SysEx
git add hardware/RECONSTRUCTION_SCHEMA.md
git commit -m "shield: doc v1.0 — vrai shield Mega + 7 boutons/7 LEDs"
```

---

## Self-Review (couverture spec)

- §1 objectif/périmètre → Tasks 3–8 (embases réelles, 2×18, 3+3 contrôles, garde l'allure). ✓
- §2 hybride → outillage Tasks 1–2 (calcul depuis template) + éditions chirurgicales Tasks 3–5. ✓
- §3 géométrie embases + 2×18 → Tasks 1 (extraction), 2 (transform), 3 (application + `check_pinorder`). ✓
- §4 boutons/LEDs 7+7, 5 droite/2 gauche, D22/24/26 + D23/25/27, 1k → Tasks 4 (schéma) + 5 (placement, `check_controls_pcb`). ✓
- §5 schéma (exposer D22–D27, J7, no-connect D28–D53) → Task 4. ✓
- §6 routage + DRC + Gerbers + firmware → Tasks 6, 7, 8. ✓
- §7 risques (routage, encombrement, fab) → Task 6 (itérations DRC), rendus Tasks 2/3/5/9, rappel test-fit Task 9. ✓
- §8 critères de succès → vérif d'ensemble Task 9. ✓

**Notes d'exécution :** plusieurs `apply_*.py` ont un squelette dont le détail d'édition s-expr (remplacement pad-par-pad, copie du bloc J7) se finalise à l'exécution sur les fichiers réels — toujours travailler sur copie + `git diff` avant commit, et s'appuyer sur les `check_*.py` comme garde-fous.
