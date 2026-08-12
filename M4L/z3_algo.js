// jsui : dessine l'algorithme FM courant (0-7 -> algorithmes 1 a 8).
// Topologies relevees sur la carte de reference Yamaha TX81Z, jeu commun a la
// famille 4 operateurs (DX21/27/100, TX81Z, Korg Z3).
// Correspondance des noms Korg <-> operateurs Yamaha :
//   C1 = op1, M1 = op2, C2 = op3, M2 = op4
// C'est le seul mappage ou l'algorithme 5 (2->1, 4->3) se lit M1->C1 et
// M2->C2, ce qui donne leur sens aux noms. Le rebouclage porte toujours sur
// l'operateur 4, donc sur M2.
inlets  = 1;
outlets = 0;

var idx = 0;
mgraphics.init();
mgraphics.relative_coords = 0;
mgraphics.autofill = 0;

function msg_int(v)   { idx = Math.max(0, Math.min(7, v | 0)); mgraphics.redraw(); }
function msg_float(v) { msg_int(Math.round(v)); }
function set(v)       { msg_int(v); }

// n : [nom, x, y] en coordonnees normalisees ; e : aretes (indices) ;
// out : indices des porteuses (reliees a la sortie audio).
var ALGOS = [
  { n: [["M2",.50,.10],["C2",.50,.37],["M1",.50,.63],["C1",.50,.89]],
    e: [[0,1],[1,2],[2,3]], out: [3] },                                   // 1
  { n: [["C2",.26,.14],["M2",.74,.14],["M1",.50,.51],["C1",.50,.87]],
    e: [[0,2],[1,2],[2,3]], out: [3] },                                   // 2
  { n: [["C2",.26,.12],["M1",.26,.45],["M2",.76,.45],["C1",.42,.85]],
    e: [[0,1],[1,3],[2,3]], out: [3] },                                   // 3
  { n: [["M2",.74,.12],["C2",.74,.45],["M1",.24,.45],["C1",.48,.85]],
    e: [[0,1],[1,3],[2,3]], out: [3] },                                   // 4
  { n: [["M1",.27,.22],["M2",.73,.22],["C1",.27,.68],["C2",.73,.68]],
    e: [[0,2],[1,3]], out: [2,3] },                                       // 5
  { n: [["M2",.50,.18],["C1",.18,.68],["M1",.50,.68],["C2",.82,.68]],
    e: [[0,1],[0,2],[0,3]], out: [1,2,3] },                               // 6
  { n: [["M2",.82,.18],["C1",.18,.68],["M1",.50,.68],["C2",.82,.68]],
    e: [[0,3]], out: [1,2,3] },                                           // 7
  { n: [["C1",.14,.45],["M1",.38,.45],["C2",.62,.45],["M2",.86,.45]],
    e: [], out: [0,1,2,3] }                                               // 8
];

function paint() {
    var W = box.rect[2] - box.rect[0];
    var H = box.rect[3] - box.rect[1];
    var a = ALGOS[idx];
    var bw = Math.min(22, W * 0.30), bh = Math.min(13, H * 0.19);
    var pad = 3;
    var busY = H - pad;

    function cx(i) { return pad + a.n[i][1] * (W - 2 * pad); }
    function cy(i) { return pad + a.n[i][2] * (H - 2 * pad - 4); }

    with (mgraphics) {
        set_line_width(1);

        // liaisons de modulation
        set_source_rgba(0.62, 0.62, 0.62, 1);
        for (var k = 0; k < a.e.length; k++) {
            var s = a.e[k][0], d = a.e[k][1];
            var x1 = cx(s), y1 = cy(s) + bh / 2, x2 = cx(d), y2 = cy(d) - bh / 2;
            move_to(x1, y1);
            if (Math.abs(x1 - x2) < 1) { line_to(x2, y2); }
            else { var my = (y1 + y2) / 2; line_to(x1, my); line_to(x2, my); line_to(x2, y2); }
            stroke();
        }

        // bus de sortie : chaque porteuse descend vers une ligne commune
        set_source_rgba(0.843, 0.741, 0.431, 1);
        var minx = W, maxx = 0;
        for (var k = 0; k < a.out.length; k++) {
            var i = a.out[k], x = cx(i);
            move_to(x, cy(i) + bh / 2); line_to(x, busY); stroke();
            if (x < minx) minx = x;
            if (x > maxx) maxx = x;
        }
        if (a.out.length > 1) { move_to(minx, busY); line_to(maxx, busY); stroke(); }

        // boites
        for (var i = 0; i < a.n.length; i++) {
            var nm = a.n[i][0], x = cx(i), y = cy(i);
            var carrier = a.out.indexOf(i) >= 0;
            rectangle(x - bw / 2, y - bh / 2, bw, bh);
            set_source_rgba(carrier ? 0.843 : 0.30, carrier ? 0.741 : 0.30,
                            carrier ? 0.431 : 0.32, 1);
            fill();
            select_font_face("Arial"); set_font_size(Math.min(8, bh - 4));
            set_source_rgba(carrier ? 0.08 : 0.92, carrier ? 0.08 : 0.92,
                            carrier ? 0.08 : 0.92, 1);
            var tm = text_measure(nm);
            move_to(x - tm[0] / 2, y + tm[1] / 2 - 1);
            text_path(nm); fill();

            if (nm === "M2") {                       // rebouclage, toujours sur op4
                set_source_rgba(0.62, 0.62, 0.62, 1);
                var rx = x + bw / 2, ry = y;
                move_to(rx, ry - bh / 4);
                line_to(rx + 4, ry - bh / 4);
                line_to(rx + 4, ry + bh / 4);
                line_to(rx, ry + bh / 4);
                stroke();
            }
        }
    }
}
