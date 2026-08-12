// jsui : dessine la forme d'onde selectionnee.
//   argument "osc" -> 8 formes du Z3 (W1..W8, jeu TX81Z), index 0-7
//   argument "lfo" -> 4 formes : 0=saw 1=square 2=triangle 3=S/H
// Recoit l'index par son inlet, ne sort rien : afficheur pur.
inlets  = 1;
outlets = 0;

var mode = (jsarguments.length > 1) ? jsarguments[1].toString() : "osc";
var idx  = 0;

mgraphics.init();
mgraphics.relative_coords = 0;
mgraphics.autofill = 0;

function msg_int(v)   { idx = v | 0; mgraphics.redraw(); }
function msg_float(v) { msg_int(Math.round(v)); }
function set(v)       { msg_int(v); }

// Version « pointue » d'un sinus : meme signe, flancs creuses.
function peak(s) { return (s < 0 ? -1 : 1) * Math.pow(Math.abs(s), 3); }

function oscValue(w, t) {          // t dans [0,1)
    var s2 = Math.sin(2 * Math.PI * t);        // un cycle
    var s4 = Math.sin(4 * Math.PI * t);        // double frequence
    var half = (t < 0.5);
    switch (w) {
        case 0: return s2;                                    // W1 sinus
        case 1: return peak(s2);                              // W2 sinus pointu
        case 2: return half ? s2 : 0;                         // W3 demi-sinus
        case 3: return half ? peak(s2) : 0;                   // W4 idem pointu
        case 4: return half ? s4 : 0;                         // W5 double freq
        case 5: return half ? peak(s4) : 0;                   // W6 idem pointu
        case 6: return half ? Math.abs(s4) : 0;               // W7 deux bosses
        case 7: return half ? Math.abs(peak(s4)) : 0;         // W8 deux pointes
    }
    return 0;
}

function lfoValue(w, t) {
    switch (w) {
        case 0: return 1 - 2 * t;                                  // dent de scie
        case 1: return (t < 0.5) ? 1 : -1;                         // carre
        case 2: return (t < 0.5) ? (4 * t - 1) : (3 - 4 * t);      // triangle
        case 3: return LFO_SH[Math.min(LFO_SH.length - 1,
                               Math.floor(t * LFO_SH.length))];    // S/H
    }
    return 0;
}
// Paliers fixes : un S/H aleatoire clignoterait a chaque redessin.
var LFO_SH = [0.55, -0.3, 0.85, -0.75, 0.15, -0.55];

function paint() {
    var w = box.rect[2] - box.rect[0];
    var h = box.rect[3] - box.rect[1];
    var pad = 3;
    var mid = h / 2;
    var amp = (h - 2 * pad) / 2;

    with (mgraphics) {
        set_source_rgba(1, 1, 1, 0.16);            // axe median
        set_line_width(1);
        move_to(pad, mid); line_to(w - pad, mid); stroke();

        set_source_rgba(0.843, 0.741, 0.431, 1);   // ambre, comme les dials
        set_line_width(1.4);
        var n = 64, first = true;
        for (var i = 0; i <= n; i++) {
            var t = i / n;
            var v = (mode === "lfo") ? lfoValue(idx, t) : oscValue(idx, t);
            var x = pad + t * (w - 2 * pad);
            var y = mid - v * amp;
            if (first) { move_to(x, y); first = false; }
            else if (mode === "lfo" && idx === 1 && (t === 0.5)) { line_to(x, y); }
            else line_to(x, y);
        }
        stroke();
    }
}
