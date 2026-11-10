#!/usr/bin/env python3
"""Pilotage du Korg Z3 depuis le Mac, a travers l'Arduino du shield.

Le firmware (KorgZ3_SysEx_26-07-2026, fonction usbRelay) renvoie sur MIDI OUT
tout SysEx recu par l'USB ; un dump de son complet devient le son courant de
la carte. On garde ici le son en cours dans un fichier JSON, on le modifie
parametre par parametre et on renvoie le dump entier — le Z3 ne connait pas
le changement de parametre individuel.

Usage :
    z3_live.py serve                   garde la liaison ouverte (a lancer en fond)
    z3_live.py init                    son vierge (sinus, une porteuse audible)
    z3_live.py set C1Level=100 M1Mult1=2 Algo=4 ...
    z3_live.py show
    z3_live.py save NOM                copie dans patches/NOM.json
    z3_live.py load NOM

Noms : ceux de M4L/z3_protocol.py — globaux (Algo, FeedB, LFOWave, Rate, PMD,
PMS, AMD, AMS) et <Osc><Param> avec Osc dans M1 C1 M2 C2 (ex. C1Level, M2Mult1).
"""
import glob
import json
import os
import pathlib
import subprocess
import sys
import termios
import time

ICI = pathlib.Path(__file__).resolve().parent
sys.path.insert(0, str(ICI.parent / "M4L"))
import z3_protocol as z  # noqa: E402

ETAT = ICI / "z3_patches" / "_courant.json"
PATCHES = ICI / "z3_patches"


def params():
    """nom court -> (position, max)"""
    p = {g[1]: (g[2], g[3]) for g in z.GLOBAL}
    for k, o in enumerate(z.OSC):
        for suf, base, mx, _w, _l in z.OSC_PARAMS:
            p[o + suf] = (base + k, mx)
    return p


def vierge():
    v = {"_nom": "VIERGE",
         "Algo": 7, "FeedB": 0, "LFOWave": 2, "Rate": 0,
         "PMD": 0, "PMS": 0, "AMD": 0, "AMS": 0}
    for o in z.OSC:
        v.update({o + "Level": 0, o + "Attack": 31, o + "Decay1": 0,
                  o + "Sustain": 0, o + "Decay2": 0, o + "Release": 8,
                  o + "Wave": 0, o + "Mult1": 1, o + "Mult2": 0,
                  o + "Detune1": 0, o + "Detune2": 0, o + "KScale": 0,
                  o + "EG": 0, o + "RateMod": 0, o + "VeloInt": 0,
                  o + "KTrack": 0, o + "AMS": 0, o + "Reverb": 0})
    v["C1Level"] = 100
    return v


def dump(etat):
    d = [0] * 95
    d[:5] = z.DUMP_HEADER
    nom = etat["_nom"].upper()[:8].ljust(8)
    d[5:13] = [ord(c) for c in nom]
    for n, (pos, mx) in params().items():
        v = max(0, min(mx, int(etat[n])))
        if pos == 16:
            d[16], d[17] = v // 128, v % 128
        else:
            d[pos] = v
    d[94] = 0xF7
    assert all(b < 0x80 for b in d[1:94])
    return bytes(d)


def port():
    ps = glob.glob("/dev/cu.usbmodem*")
    if not ps:
        sys.exit("Arduino introuvable (/dev/cu.usbmodem*)")
    return ps[0]


SOCK = str(PATCHES / ".z3.sock")


def ouvrir():
    """Ouvre le port serie. L'ouverture remet le Mega a zero (DTR) : on attend
    la fin de son demarrage. D'ou le serveur, qui ne l'ouvre qu'une fois."""
    fd = os.open(port(), os.O_RDWR | os.O_NOCTTY)
    a = termios.tcgetattr(fd)
    a[0] = a[1] = a[3] = 0                                  # brut
    a[2] = (a[2] | termios.CS8 | termios.CREAD | termios.CLOCAL) & ~termios.HUPCL
    a[4] = a[5] = termios.B115200
    a[6][termios.VMIN], a[6][termios.VTIME] = 0, 0
    termios.tcsetattr(fd, termios.TCSANOW, a)
    time.sleep(6)
    return fd


def transmettre(fd, data):
    # Le Mega lit l'USB entre deux tours de boucle : s'il est occupe (envoi
    # MIDI, ecran), son tampon de 64 octets deborde et le dump arrive tronque
    # (accuse « R » au lieu de « K »). On renvoie alors, jusqu'a trois fois.
    for _ in range(3):
        rep = _transmettre(fd, data)
        if b"K" in rep or len(data) != 95:
            return rep
        time.sleep(0.3)
    return rep


def _transmettre(fd, data):
    termios.tcflush(fd, termios.TCIFLUSH)
    # Le tampon de reception du Mega fait 64 octets : envoi par paquets.
    for i in range(0, len(data), 16):
        os.write(fd, data[i:i + 16])
        time.sleep(0.03)
    # La carte renvoie aussi l'echo des sons (F0 ... F7) : on ne garde que les
    # accuses ecrits hors SysEx.
    t0, rep, dans_sysex = time.time(), b"", False
    while time.time() - t0 < 2.5 and not rep:
        for b in os.read(fd, 256):
            if b == 0xF0:
                dans_sysex = True
            elif b == 0xF7:
                dans_sysex = False
            elif not dans_sysex and b in b"KXR":
                rep = bytes([b])
        time.sleep(0.02)
    return rep


def servir():
    """Garde le port ouvert et relaie les dumps recus sur une socket locale."""
    import socket
    PATCHES.mkdir(exist_ok=True)
    if os.path.exists(SOCK):
        os.remove(SOCK)
    fd = ouvrir()
    srv = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
    srv.bind(SOCK)
    srv.listen(1)
    print("serveur pret", flush=True)
    while True:
        c, _ = srv.accept()
        with c:
            data = b""
            while len(data) < 95:
                morceau = c.recv(95 - len(data))
                if not morceau:
                    break
                data += morceau
            c.sendall(transmettre(fd, data) or b"-")


def envoyer(data):
    import socket
    if os.path.exists(SOCK):
        try:
            c = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
            c.settimeout(5)
            c.connect(SOCK)
            c.sendall(data)
            rep = c.recv(16)
            c.close()
            return rep
        except OSError:
            pass
    fd = ouvrir()          # pas de serveur : envoi direct, plus lent
    try:
        return transmettre(fd, data)
    finally:
        os.close(fd)


def charger():
    return json.loads(ETAT.read_text()) if ETAT.exists() else vierge()


def ecrire(etat):
    PATCHES.mkdir(exist_ok=True)
    ETAT.write_text(json.dumps(etat, indent=1))
    rep = envoyer(dump(etat))
    print("Z3 :", "son recu (K)" if b"K" in rep else f"PAS D'ACCUSE ({rep!r})")


def montrer(etat):
    g = ["Algo", "FeedB", "LFOWave", "Rate", "PMD", "PMS", "AMD", "AMS"]
    print(f"[{etat['_nom']}]  " + "  ".join(f"{k}={etat[k]}" for k in g))
    cols = [s for s, *_ in z.OSC_PARAMS]
    print("     " + "".join(f"{c[:7]:>8}" for c in cols))
    for o in z.OSC:
        print(f"{o:4} " + "".join(f"{etat[o + c]:>8}" for c in cols))


def main(a):
    if not a or a[0] == "show":
        montrer(charger()); return
    if a[0] == "serve":
        servir(); return
    if a[0] == "init":
        e = vierge()
    elif a[0] == "set":
        e, ps = charger(), params()
        for kv in a[1:]:
            k, v = kv.split("=")
            if k == "Name":
                e["_nom"] = v; continue
            if k not in ps:
                sys.exit(f"parametre inconnu : {k}")
            if not 0 <= int(v) <= ps[k][1]:
                sys.exit(f"{k} : 0..{ps[k][1]}")
            e[k] = int(v)
    elif a[0] == "save":
        e = charger(); e["_nom"] = a[1][:8]
        (PATCHES / f"{a[1]}.json").write_text(json.dumps(e, indent=1))
    elif a[0] == "load":
        e = json.loads((PATCHES / f"{a[1]}.json").read_text())
    else:
        sys.exit(__doc__)
    ecrire(e)
    montrer(e)


if __name__ == "__main__":
    main(sys.argv[1:])
