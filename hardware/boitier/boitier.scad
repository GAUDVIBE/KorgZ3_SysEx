// =====================================================================
//  Boîtier pupitre — shield Korg Z3 SysEx v1.2
//
//  Toutes les positions sont RELEVÉES sur SysEx_Patcher.kicad_pcb.
//  `check_boitier.py` les revérifie contre le fichier KiCad : ne pas
//  modifier le bloc « relevé sur le PCB » à la main.
//
//  Repère du modèle :
//     X = x_kicad − 20      (0 = bord gauche de la carte)
//     Y = 192 − y_kicad     (0 = bord joueur, 172 = bord des connecteurs)
//     Z = 0                 (dessous du boîtier, posé sur la table)
//
//  La carte est INCLINÉE. La façade et les parois lui sont perpendiculaires ;
//  seul le dessous est aplani pour poser à plat.
//
//  DEUX NIVEAUX. Les potentiomètres à axe court imposent une façade à
//  8,5 mm de la carte, alors que le mini-XLR mesure 13,7 mm de haut et les
//  embases MIDI davantage. Les connecteurs sont donc logés sous un
//  DOSSERET surélevé à l'arrière, comme sur un synthétiseur de bureau.
// =====================================================================

/* [Pièce à générer] */
piece = "assemblage"; // [assemblage, fond, facade, plaque_arriere, capot_arriere, capuchon]

/* [À MESURER sur tes composants — les seules cotes non déduites du PCB] */

// ⚠️ SEULE COTE ENCORE MANQUANTE : hauteur du corps du potentiomètre
// au-dessus de la carte, jusqu'à l'épaulement du canon fileté. C'est elle
// qui fixe la hauteur de la façade — la mesurer au pied à coulisse.
pot_corps_h = 7.0;
// Longueur de l'axe, depuis l'épaulement du canon (convention des fiches)
pot_axe_h = 15.0;
// Longueur du canon fileté
pot_filetage_h = 5.0;
// Diamètre extérieur du canon fileté (M7 sur un potentiomètre 9 mm)
pot_canon_d = 7.0;
// Hauteur du bouton poussoir au-dessus de la carte
bouton_h = 7.3;
// Hauteur de l'axe des embases MIDI au-dessus de la carte
midi_z = 11;
// Diamètre de dégagement d'une fiche MIDI DIN 5
midi_d = 23;
// Mini-XLR : embase de PANNEAU Switchcraft TB5M, câblée jusqu'aux
// pastilles de J10. Elle n'est plus tenue par la carte, donc libre en X
// comme en Z. L'axe est volontairement haut : le corps de l'embase
// s'enfonce vers l'intérieur et doit passer AU-DESSUS de la carte.
// Perçage « D » recommandé, relevé sur la fiche TB_M SERIES rév. S :
// Ø 0.444" avec un méplat à 0.422". Le méplat empêche l'embase de tourner.
// Fixation par écrou sur filetage 7/16-32 UNS-2A ; épaisseur de paroi
// admissible 0.250" = 6,35 mm, la plaque en fait 3.
xlr_z = 12;
xlr_percage = 11.28;   // 0.444"
xlr_meplat  = 10.72;   // 0.422", du méplat au bord opposé
// Hauteur de l'axe du jack d'alimentation au-dessus de la carte
alim_z = 6;

/* [Réglages du boîtier] */

// Inclinaison du pupitre, en degrés
pente = 15;
// Épaisseur des parois et du fond
paroi = 3.0;
// Épaisseur de la façade. NE PAS DÉPASSER pot_filetage_h − 1,6 : au-delà, le
// canon ne ressort plus assez pour recevoir son écrou.
facade_ep = 3.0;
// Hauteur du dessus du dosseret au-dessus de la carte
dosseret_h = 24.0;
// Hauteur du dessus de la carte au bord avant
z_avant = 10.0;
// Débord du Mega au-delà du bord gauche de la carte, jeu compris
debord_mega = 4.0;
// Jeu entre la carte et les parois
jeu = 0.5;

/* [Hidden] */
$fn = 48;
PROF = 400;   // profondeur de travail des solides tronqués

// --- Hauteur de la façade : elle vient s'appuyer sur l'épaulement des
// potentiomètres, dont les écrous la tiennent. Ce sont eux, et non les quatre
// vis, qui font la rigidité de l'ensemble.
ecart_facade = pot_corps_h;
pot_percage  = pot_canon_d + 0.5;

// Les boutons poussoirs sont plus hauts que l'épaulement des potentiomètres :
// on lamé la façade par-dessous en regard de chacun pour leur faire place.
LAMAGE_H = max(0, bouton_h + 0.7 - ecart_facade);
LAMAGE_D = 9.0;

// Contrôles, affichés à chaque compilation
axe_emergent  = pot_axe_h - facade_ep;
filet_restant = pot_filetage_h - facade_ep;
echo(str("Axe emergent au-dessus de la facade : ", axe_emergent, " mm"));
echo(str("Filetage restant pour l'ecrou       : ", filet_restant, " mm"));
echo(str("Lamage sous la facade pour boutons  : ", LAMAGE_H, " mm"));
assert(filet_restant >= 1.6,
       "Facade trop epaisse : le canon ne ressort pas assez pour son ecrou.");

// ---------------------------------------------------------------------
//  Relevé sur le PCB — NE PAS ÉDITER À LA MAIN
// ---------------------------------------------------------------------
CARTE   = 172;
CARTE_E = 1.6;

// Axes des 16 potentiomètres : grille régulière
POT_X = [ 27.5,  59.5,  91.5, 123.5];
POT_Y = [ 32.5,  56.5,  80.5, 104.5];

// Centres des 7 boutons — 5 à droite, 2 à gauche
BOUTONS = [[151.25, 129.75], [151.25, 105.75], [151.25,  81.75],
           [151.25,  57.75], [151.25,  33.75],
           [  9.25,  89.75], [  9.25,  29.75]];

// Centres des 7 LEDs, en regard de chaque bouton
LEDS = [[159.27, 132], [159.27, 108], [159.27, 84],
        [159.27,  60], [159.27,  36],
        [ 16.27,  92], [ 16.27,  32]];

// Trous M3 de la carte
VIS = [[8, 8], [164, 8], [8, 164], [164, 164]];

// Emprise du Mega sous la carte ; il déborde à gauche de 2,8 mm
MEGA_Y0 = 55.2;
MEGA_Y1 = 108.5;

// Connecteurs du bord arrière : X du centre
MIDI_IN_X  = 30;
MIDI_OUT_X = 67;
XLR_X      = 90.8;
ALIM_X     = 117.9;
ALIM_L     = 13;
ALIM_H     = 13;

// Le corps des connecteurs commence à Y = 149.7 : la marche est en deçà
Y_MARCHE = 146;

// Fenêtre de l'écran OLED — connecteur J5 ; à ajuster au module réel
OLED   = [142, 128];
OLED_L = 30;
OLED_H = 16;

// ---------------------------------------------------------------------
//  Géométrie dérivée
// ---------------------------------------------------------------------
IX0 = -debord_mega;         IX1 = CARTE + jeu;
IY0 = -jeu;                 IY1 = CARTE + jeu;
OX0 = IX0 - paroi;          OX1 = IX1 + paroi;
OY0 = IY0 - paroi;          OY1 = IY1 + paroi;

// Fenêtre de la plaque arrière, en repère carte
FEN_X0 = 12;   FEN_X1 = 140;
FEN_Z0 = -3;   FEN_Z1 = dosseret_h - 1;
FEUIL_M = 4;    // débord de la feuillure autour de la fenêtre
FEUIL_E = 3.4;  // profondeur : épaisseur de la plaque + jeu

// Ouverture du flanc gauche : USB et jack du Mega
FLANC_Y0 = MEGA_Y0 - 4;
FLANC_Y1 = MEGA_Y1 + 4;

ENTRETOISE_D = 8;
TARAUD_D     = 2.9;
TARAUD_P     = 8;

// ---------------------------------------------------------------------
//  Outils de repère
// ---------------------------------------------------------------------

// Place ses enfants dans le plan de la carte : z local = hauteur au-dessus
// de la surface de la carte.
module repere_carte() { translate([0, 0, z_avant]) rotate([pente, 0, 0]) children(); }

module demi_espace(z) { translate([-600, -600, z]) cube([1200, 1200, 1200]); }

// ---------------------------------------------------------------------
//  Fond
// ---------------------------------------------------------------------

module corps_exterieur() {
    intersection() {
        repere_carte() union() {
            // partie avant, au niveau de la façade
            translate([OX0, OY0, -PROF])
                cube([OX1-OX0, Y_MARCHE-OY0, PROF + ecart_facade]);
            // dosseret arrière, plus haut
            translate([OX0, Y_MARCHE, -PROF])
                cube([OX1-OX0, OY1-Y_MARCHE, PROF + dosseret_h]);
        }
        demi_espace(0);
    }
}

module cavite() {
    intersection() {
        repere_carte() union() {
            translate([IX0, IY0, -PROF])
                cube([IX1-IX0, Y_MARCHE-IY0, PROF + ecart_facade + 1]);
            translate([IX0, Y_MARCHE - 1, -PROF])
                cube([IX1-IX0, IY1-Y_MARCHE + 1, PROF + dosseret_h]);
        }
        demi_espace(paroi);
    }
}

module entretoises() {
    intersection() {
        repere_carte()
            for (p = VIS)
                translate([p[0], p[1], -PROF - CARTE_E]) cylinder(h = PROF, d = ENTRETOISE_D);
        demi_espace(paroi);
    }
}

module taraudages() {
    repere_carte()
        for (p = VIS)
            translate([p[0], p[1], -CARTE_E - TARAUD_P]) cylinder(h = TARAUD_P + 2, d = TARAUD_D);
}

// Fenêtre arrière et sa feuillure, ouverte vers le haut : la plaque s'y glisse
module ouverture_arriere() {
    repere_carte() {
        translate([FEN_X0, IY1 - 1, FEN_Z0])
            cube([FEN_X1-FEN_X0, paroi + 2, FEN_Z1-FEN_Z0]);
        translate([FEN_X0 - FEUIL_M, IY1 - FEUIL_E, FEN_Z0 - FEUIL_M])
            cube([FEN_X1-FEN_X0 + 2*FEUIL_M, FEUIL_E + 1, dosseret_h - FEN_Z0 + FEUIL_M]);
    }
}

// Le Mega déborde de 2,8 mm : le flanc gauche s'ouvre sur son USB et son jack
module ouverture_flanc() {
    repere_carte()
        translate([IX0 - paroi - 1, FLANC_Y0, -PROF])
            cube([paroi + 2, FLANC_Y1-FLANC_Y0, PROF - 2]);
}

// Appuis du capot arrière, dans l'épaisseur des parois latérales
module appuis_capot() {
    repere_carte()
        for (x = [IX0 + paroi/2, IX1 - paroi/2])
            translate([x, (Y_MARCHE + IY1)/2, dosseret_h - 8]) cylinder(h = 10, d = TARAUD_D);
}

module fond() {
    difference() {
        union() { difference() { corps_exterieur(); cavite(); } entretoises(); }
        taraudages();
        ouverture_arriere();
        ouverture_flanc();
        appuis_capot();
    }
}

// ---------------------------------------------------------------------
//  Façade — commandes seulement, jusqu'à la marche
// ---------------------------------------------------------------------

// Repère local de la façade : z = 0 au dessous de la plaque, facade_ep au-dessus.
// Les traversées ordinaires percent la plaque ; celles des vis descendent en
// plus dans les bossages, qui plongent jusqu'à la carte.
module percages_facade() {
    h  = facade_ep + 2;
    hv = facade_ep + ecart_facade + 2;
    for (x = POT_X) for (y = POT_Y)
        translate([x, y, -1]) cylinder(h = h, d = pot_percage);
    for (b = BOUTONS) {
        translate([b[0], b[1], -1]) cylinder(h = h, d = 6.4);
        // lamage par-dessous : le bouton dépasse l'épaulement des pots
        if (LAMAGE_H > 0)
            translate([b[0], b[1], -0.01]) cylinder(h = LAMAGE_H, d = LAMAGE_D);
    }
    for (l = LEDS)
        translate([l[0], l[1], -1]) cylinder(h = h, d = 3.2);
    translate([OLED[0] - OLED_L/2, OLED[1] - OLED_H/2, -1]) cube([OLED_L, OLED_H, h]);
    for (p = VIS) if (p[1] < Y_MARCHE)
        translate([p[0], p[1], -ecart_facade - 1]) cylinder(h = hv, d = 3.4);
}

module facade_locale() {
    difference() {
        union() {
            translate([OX0, OY0, 0]) cube([OX1-OX0, Y_MARCHE-OY0, facade_ep]);
            // lèvre de centrage sur trois côtés
            difference() {
                translate([IX0 + 0.3, IY0 + 0.3, -4])
                    cube([IX1-IX0 - 0.6, Y_MARCHE-IY0 - 0.6, 4]);
                translate([IX0 + 2.3, IY0 + 2.3, -5])
                    cube([IX1-IX0 - 4.6, Y_MARCHE-IY0, 6]);
            }
            // bossages qui font entretoise jusqu'à la carte
            for (p = VIS) if (p[1] < Y_MARCHE)
                translate([p[0], p[1], -ecart_facade]) cylinder(h = ecart_facade, d = 7);
        }
        percages_facade();
        for (p = VIS) if (p[1] < Y_MARCHE)
            translate([p[0], p[1], facade_ep - 1.8]) cylinder(h = 3, d1 = 3.4, d2 = 6.4);
    }
}

module facade() { repere_carte() translate([0, 0, ecart_facade]) facade_locale(); }

// à plat sur le plateau, face visible contre le verre
module facade_a_plat() { translate([0, 0, facade_ep]) rotate([180, 0, 0]) facade_locale(); }

// ---------------------------------------------------------------------
//  Plaque arrière — porte les découpes des connecteurs. Pièce sacrificielle :
//  si une hauteur est fausse, on ne réimprime qu'elle.
// ---------------------------------------------------------------------

// Perçage « D » : cylindre tronqué par un méplat. `meplat` est la cote de la
// fiche, mesurée du méplat au bord opposé de l'arc.
module trou_d(diam, meplat, h) {
    r = diam / 2;
    intersection() {
        cylinder(h = h, d = diam);
        translate([-r - 1, -r - 1, -1]) cube([diam + 2, meplat + 1, h + 2]);
    }
}

module plaque_arriere() {
    l = FEN_X1-FEN_X0 + 2*FEUIL_M - 0.4;
    h = dosseret_h - FEN_Z0 + FEUIL_M - 0.4;
    difference() {
        cube([l, h, 3]);
        // origine ramenée au repère carte
        translate([-(FEN_X0 - FEUIL_M), -(FEN_Z0 - FEUIL_M), 0]) {
            translate([MIDI_IN_X,  midi_z, -1]) cylinder(h = 5, d = midi_d);
            translate([MIDI_OUT_X, midi_z, -1]) cylinder(h = 5, d = midi_d);
            translate([XLR_X,      xlr_z,  -1]) trou_d(xlr_percage, xlr_meplat, 5);
            translate([ALIM_X - ALIM_L/2, alim_z - ALIM_H/2, -1]) cube([ALIM_L, ALIM_H, 5]);
        }
    }
}

// ---------------------------------------------------------------------
//  Capot du dosseret — en L, imprimé couché sur son tablier
// ---------------------------------------------------------------------

module capot_arriere_local() {
    lg = IX1-IX0 - 0.4;
    pf = IY1 - Y_MARCHE + paroi;
    union() {
        // dessus
        translate([IX0 + 0.2, Y_MARCHE, dosseret_h]) cube([lg, pf, facade_ep]);
        // tablier avant, qui ferme la marche
        translate([IX0 + 0.2, Y_MARCHE, 2]) cube([lg, facade_ep, dosseret_h - 2]);
    }
}

module capot_arriere() {
    difference() {
        capot_arriere_local();
        for (x = [IX0 + paroi/2, IX1 - paroi/2])
            translate([x, (Y_MARCHE + IY1)/2, dosseret_h - 1])
                cylinder(h = facade_ep + 3, d = 3.4);
    }
}

// couché sur son tablier, le dessus dressé : aucun support nécessaire
module capot_a_plat() { translate([0, 0, -Y_MARCHE]) rotate([90, 0, 0]) capot_arriere(); }

// ---------------------------------------------------------------------
//  Capuchon de bouton
// ---------------------------------------------------------------------

// Repose sur le poussoir. La collerette est captive dans le lamage de la
// façade : le capuchon ne peut ni tomber ni ressortir.
module capuchon() {
    collerette = ecart_facade + LAMAGE_H - bouton_h;
    total      = ecart_facade + facade_ep - bouton_h + 2;   // 2 mm de dépassement
    union() {
        cylinder(h = collerette, d = LAMAGE_D - 0.6);
        cylinder(h = total, d = 6.0);
    }
}

// ---------------------------------------------------------------------
//  Aperçu
// ---------------------------------------------------------------------

module carte_temoin() {
    color("darkgreen", 0.6)
        repere_carte() translate([0, 0, -CARTE_E]) cube([CARTE, CARTE, CARTE_E]);
    color("silver", 0.8)
        repere_carte() for (x = POT_X) for (y = POT_Y) {
            translate([x, y, 0]) cylinder(h = pot_corps_h, d = 10);
            translate([x, y, 0]) cylinder(h = pot_axe_h,  d = 6);
        }
    color("black", 0.9)
        repere_carte() {
            for (x = [MIDI_IN_X, MIDI_OUT_X])
                translate([x, IY1 - 15, midi_z]) rotate([-90, 0, 0]) cylinder(h = 15, d = 20);
            translate([XLR_X, IY1 - 20, xlr_z]) rotate([-90, 0, 0]) cylinder(h = 22, d = 13);
        }
    color("dimgray", 0.85)
        repere_carte() translate([IX0 + 1.2, MEGA_Y0, -13]) cube([101.6, MEGA_Y1-MEGA_Y0, 1.6]);
}

// ---------------------------------------------------------------------
if      (piece == "assemblage")     { fond(); color("steelblue", 0.55) { facade(); capot_arriere(); } carte_temoin(); }
else if (piece == "fond")           fond();
else if (piece == "facade")         facade_a_plat();
else if (piece == "plaque_arriere") plaque_arriere();
else if (piece == "capot_arriere")  capot_a_plat();
else if (piece == "capuchon")       capuchon();
