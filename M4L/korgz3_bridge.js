// Pont Node for Max pour le Korg Z3 — meme architecture que dx100_bridge.js
// (validee le 30/07/2026), mais ici TOUT le protocole vit dans ce script :
// le Z3 ne recoit que des dumps complets (F0 42 30 1D 40 + nom 8 car. +
// 81 octets + F7), pas de changement de parametre individuel.
const Max = require('max-api');
const { spawn } = require('child_process');
const fs = require('fs');
const path = require('path');

const EXE = path.join(__dirname, 'korgz3_listen');
const LOG = path.join(__dirname, 'korgz3_bridge.log');
const DUMP_REQUEST = [0xF0, 0x42, 0x30, 0x1D, 0x10, 0xF7];

// Ordre canonique des 80 controles = positions triees (identique au patcher).
const POSITIONS = [13, 14, 15, 16, 18, 19, 20, 21];
for (let base = 22; base <= 90; base += 4)
    for (let k = 0; k < 4; k++) POSITIONS.push(base + k);

// Buffer initial : preset usine SynLead (du firmware Arduino).
let buf = [
  0xF0,0x42,0x30,0x1D,0x40,
  0x53,0x79,0x6E,0x4C,0x65,0x61,0x64,0x20,
  0x03,0x02,0x02,0x01,0x2F,0x17,0x03,0x04,0x02,0x02,
  0x01,0x00,0x01,0x0A,0x01,0x01,0x01,0x00,0x00,0x00,
  0x00,0x03,0x07,0x07,0x03,0x00,0x00,0x00,0x00,0x63,
  0x1C,0x62,0x00,0x19,0x18,0x06,0x15,0x00,0x00,0x00,
  0x1F,0x00,0x02,0x00,0x00,0x00,0x1F,0x00,0x06,0x0E,
  0x08,0x07,0x08,0x01,0x00,0x00,0x00,0x00,0x00,0x01,
  0x00,0x02,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x04,0x06,0x02,0x04,0x00,0x00,0x03,
  0x00,
  0xF7
];

let child = null, sendTimer = null;

function log(msg) {
    const line = new Date().toISOString() + '  ' + msg;
    try { fs.appendFileSync(LOG, line + '\n'); } catch (e) {}
    Max.post(line);
    Max.outlet('status', msg);
}

function valueAt(pos) { return pos === 16 ? buf[16] * 128 + buf[17] : buf[pos]; }

function currentName() {
    return buf.slice(5, 13).map(c => (c >= 0x20 && c < 0x7F) ? String.fromCharCode(c) : ' ')
              .join('').replace(/\s+$/, '') || '????';
}

function emitState() {
    Max.outlet('load', ...POSITIONS.map(valueAt));
    Max.outlet('setname', currentName());
}

function scheduleSend() {
    if (sendTimer) return;
    sendTimer = setTimeout(() => {
        sendTimer = null;
        Max.outlet('sysex', ...buf);
    }, 150);
}

function start(port) {
    stop();
    log('demarrage du pont Z3 (script ' + __filename + ')');
    if (!fs.existsSync(EXE)) { log('ERREUR : binaire introuvable ' + EXE); return; }
    child = spawn(EXE, port ? [port] : [], { stdio: ['ignore', 'pipe', 'pipe'] });
    log('korgz3_listen lance (pid ' + child.pid + ')');
    let acc = '';
    child.stdout.on('data', (chunk) => {
        acc += chunk.toString();
        let nl;
        while ((nl = acc.indexOf('\n')) >= 0) {
            const line = acc.slice(0, nl).trim();
            acc = acc.slice(nl + 1);
            const parts = line.split(/\s+/);
            if (parts[0] !== 'DUMP') continue;
            const data = parts.slice(1).map(Number);
            if (data.length < 20 || data.length > 96) {
                log('dump ignore : ' + data.length + ' octets'); continue;
            }
            buf = data.slice();
            log('dump recu du Z3 (' + data.length + ' octets, "' + currentName() + '")');
            emitState();
        }
    });
    child.stderr.on('data', (d) => log('korgz3_listen: ' + d.toString().trim()));
    child.on('exit', (code) => { log('korgz3_listen arrete (code ' + code + ')'); child = null; });
    child.on('error', (e) => log('impossible de lancer korgz3_listen : ' + e.message));
}

function stop() { if (child) { child.kill(); child = null; } }

Max.addHandler('param', (pos, val) => {
    pos = Math.round(pos); val = Math.round(val);
    if (pos === 16) {                                   // Rate : 2 octets hi/lo
        val = Math.max(0, Math.min(255, val));
        buf[16] = Math.floor(val / 128); buf[17] = val % 128;
    } else if (pos >= 13 && pos < buf.length - 1) {
        buf[pos] = Math.max(0, Math.min(127, val));
    }
    scheduleSend();
});
Max.addHandler('name', (...words) => {
    let s = words.join(' ').slice(0, 8);
    while (s.length < 8) s += ' ';
    for (let i = 0; i < 8; i++) {
        const c = s.charCodeAt(i);
        buf[5 + i] = (c >= 0x20 && c < 0x7F) ? c : 0x20;
    }
    scheduleSend();
});
Max.addHandler('dump', () => { log('dump request -> Z3'); Max.outlet('sysex', ...DUMP_REQUEST); });
Max.addHandler('send', () => { log('envoi du buffer complet (' + buf.length + ' octets)'); Max.outlet('sysex', ...buf); });
Max.addHandler('port', start);
Max.addHandler('stop', stop);
Max.addHandlers({ [Max.MESSAGE_TYPES.ALL]: () => {} });

log('=== korgz3_bridge.js charge par node.script ===');
start();
setTimeout(emitState, 700);      // synchronise l'UI sur le buffer initial
process.on('exit', stop);
