#!/usr/bin/env python3
"""
export_dsn.py — Exporte le PCB au format Specctra DSN pour Freerouting.
Doit être lancé avec le python bundlé KiCad (import pcbnew).
Usage: python3 shield_redesign/export_dsn.py [pcb_in] [dsn_out]
"""
import sys, os, pathlib

HERE = pathlib.Path(__file__).parent
PCB_DEFAULT = HERE.parent / "SysEx_Patcher.kicad_pcb"
DSN_DEFAULT = "/tmp/SysEx_Patcher.dsn"

def main():
    import pcbnew
    pcb_path = str(sys.argv[1]) if len(sys.argv) > 1 else str(PCB_DEFAULT)
    dsn_path = str(sys.argv[2]) if len(sys.argv) > 2 else DSN_DEFAULT

    board = pcbnew.LoadBoard(pcb_path)
    ok = pcbnew.ExportSpecctraDSN(board, dsn_path)
    print(f"Export DSN {'OK' if ok else 'ECHEC'}: {dsn_path}")
    os._exit(0 if ok else 1)

if __name__ == "__main__":
    main()
