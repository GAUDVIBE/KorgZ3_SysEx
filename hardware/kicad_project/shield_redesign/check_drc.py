#!/usr/bin/env python3
"""
check_drc.py — Lance le DRC sur SysEx_Patcher.kicad_pcb et rapporte
les violations et les connexions manquantes.
Usage: python3 shield_redesign/check_drc.py [chemin_pcb]
"""
import sys, os, json, subprocess, pathlib

HERE = pathlib.Path(__file__).parent
PCB_DEFAULT = HERE.parent / "SysEx_Patcher.kicad_pcb"
DRC_OUT = "/tmp/drc_check.json"

def main():
    pcb = str(sys.argv[1]) if len(sys.argv) > 1 else str(PCB_DEFAULT)
    if not os.path.exists(pcb):
        print(f"ERREUR: PCB introuvable: {pcb}")
        sys.exit(1)

    print(f"DRC sur: {pcb}")

    # Run kicad-cli DRC
    kicad_cli = "/opt/homebrew/bin/kicad-cli"
    cmd = [kicad_cli, "pcb", "drc",
           "--severity-error",
           "--format", "json",
           "-o", DRC_OUT,
           pcb]

    result = subprocess.run(cmd, capture_output=True, text=True)
    stdout = result.stdout.strip()
    stderr = result.stderr.strip()

    if stdout:
        print(stdout)
    if stderr:
        print(f"[stderr] {stderr}", file=sys.stderr)

    if not os.path.exists(DRC_OUT):
        print("ERREUR: rapport DRC non généré")
        sys.exit(1)

    with open(DRC_OUT) as f:
        d = json.load(f)

    violations = d.get("violations", [])
    unconnected = d.get("unconnected_items", [])

    # Group violations by type
    by_type = {}
    for v in violations:
        t = v.get("type", "unknown")
        by_type[t] = by_type.get(t, 0) + 1

    print(f"\n{'='*50}")
    print(f"RÉSULTAT DRC")
    print(f"{'='*50}")
    print(f"Violations totales : {len(violations)}")
    for t, c in sorted(by_type.items(), key=lambda x: -x[1]):
        print(f"  {c:4d}  {t}")
    print(f"\nConnexions manquantes (ratsnest) : {len(unconnected)}")
    for u in unconnected:
        desc = u.get("description", "")
        items = u.get("items", [])
        pad_descs = [i.get("description", "") for i in items]
        print(f"  {desc}")
        for pd in pad_descs:
            print(f"    - {pd}")

    # Summary
    print(f"\n{'='*50}")
    if len(unconnected) == 0 and len(violations) == 0:
        print("PARFAIT: DRC 0 violation, 0 non-connecté")
    elif len(unconnected) == 0:
        print(f"Routage complet (0 non-connecté), {len(violations)} violations DRC à corriger")
    else:
        print(f"ATTENTION: {len(unconnected)} non-connecté(s), {len(violations)} violation(s)")

if __name__ == "__main__":
    main()
