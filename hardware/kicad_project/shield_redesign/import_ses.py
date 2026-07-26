#!/usr/bin/env python3
"""
import_ses.py — Importe le résultat du routage Freerouting (.ses) dans le
PCB, puis re-remplit les zones de cuivre (étape obligatoire, sinon DRC
produit des centaines de faux positifs) et sauvegarde.
Doit être lancé avec le python bundlé KiCad (import pcbnew).
Usage: python3 shield_redesign/import_ses.py [pcb_in] [ses_in] [pcb_out]
"""
import sys, os, pathlib

HERE = pathlib.Path(__file__).parent
PCB_DEFAULT = HERE.parent / "SysEx_Patcher.kicad_pcb"
SES_DEFAULT = "/tmp/SysEx_Patcher.ses"

def main():
    import pcbnew
    pcb_in = str(sys.argv[1]) if len(sys.argv) > 1 else str(PCB_DEFAULT)
    ses_in = str(sys.argv[2]) if len(sys.argv) > 2 else SES_DEFAULT
    pcb_out = str(sys.argv[3]) if len(sys.argv) > 3 else pcb_in

    board = pcbnew.LoadBoard(pcb_in)

    ok = pcbnew.ImportSpecctraSES(board, ses_in)
    print(f"Import SES {'OK' if ok else 'ECHEC'}: {ses_in}")

    filler = pcbnew.ZONE_FILLER(board)
    zones = board.Zones()
    filler.Fill(zones)
    print(f"Zones remplies: {len(zones)}")

    ok_save = pcbnew.SaveBoard(pcb_out, board)
    print(f"Sauvegarde {'OK' if ok_save else 'ECHEC'}: {pcb_out}")

    os._exit(0 if (ok and ok_save) else 1)

if __name__ == "__main__":
    main()
