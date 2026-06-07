import subprocess, sys

PCB = "SysEx_Patcher.kicad_pcb"
OUT = sys.argv[1] if len(sys.argv) > 1 else "/tmp/board.png"
KCLI = "/opt/homebrew/bin/kicad-cli"

result = subprocess.run(
    [KCLI, "pcb", "render", "-o", OUT, "--side", "top", PCB],
    check=False, capture_output=True, text=True
)
if result.returncode == 0:
    print("rendu:", OUT)
else:
    print("kicad-cli render non disponible ou erreur (non bloquant)")
    if result.stderr:
        print("stderr:", result.stderr[:300])
