# Câblage côté guitare — molette de pitch et sélecteur de plage

Ce document décrit le montage à l'intérieur de la Stratocaster et le câble
mini-XLR qui la relie au shield. Relevé sur le PCB v1.2.

---

## Brochage du mini-XLR

Identique aux deux extrémités. Relevé sur `J10` dans `SysEx_Patcher.kicad_pcb`.

| Broche | Signal | Rôle |
|---|---|---|
| 1 | `GND` | masse du circuit de pitch — **pas celle de la guitare**, voir plus bas |
| 2 | `+5V` | alimente le pot et l'échelle de résistances |
| 3 | `JACK_W` | curseur de la molette → canal 1 du multiplexeur |
| 4 | `SW_BEND` | prise du sélecteur → canal 2 du multiplexeur |
| 5 | — | **libre**, disponible pour une commande future |

> ⚠️ Le brochage des fiches est donné **vue de face côté accouplement**. Une
> femelle vue de face est le miroir d'un mâle vu de face. Contrôler la
> **continuité fil par fil au multimètre** avant le premier branchement : une
> inversion enverrait le +5 V sur le curseur de la molette.

---

## Le montage dans la guitare

### La molette

Potentiomètre **10 kΩ linéaire à ressort de rappel**, monté en diviseur :
extrémités sur +5 V et GND, curseur sur la broche 3. Au repos le ressort le
ramène au centre, soit environ 512 pas de convertisseur — le firmware y
applique une zone morte.

### Le sélecteur de plage

Quatre résistances de **2,2 kΩ en série** entre +5 V et GND forment une échelle
à trois prises. Le `46311LDRX` en sélectionne une et la renvoie sur la broche 4.

| Position | Prise | Lecture attendue | Fenêtre du firmware |
|---|---|---|---|
| 1 | ¼ Vcc | ~256 | 176 – 336 |
| 2 | ½ Vcc | ~512 | 432 – 592 |
| 3 | ¾ Vcc | ~768 | 688 – 848 |
| *(câble débranché)* | — | ~0 | 0 – 100 |

**Le zéro est laissé libre exprès.** Une résistance de 470 kΩ sur la carte tire
la ligne à la masse en l'absence de guitare : le firmware y reconnaît un câble
débranché et suspend l'envoi de pitch bend. Sans elle, le Z3 recevrait du pitch
fantôme dès que la guitare est retirée.

Consommation totale : ~0,5 mA pour le pot, ~0,6 mA pour l'échelle.

### Découplage

**Un condensateur de 100 nF entre +5 V et GND, au plus près de la molette**,
à l'intérieur de la guitare. C'est la mesure la plus efficace contre le bruit
rayonné vers les micros — bien davantage que le blindage du connecteur.

---

## Les masses : ne jamais les relier

**La masse du circuit de pitch (broche 1) ne doit en aucun cas être reliée à la
masse audio de la guitare.**

Les deux se rejoignent déjà par la terre du secteur : la guitare par l'ampli, la
carte par son alimentation. Une seconde liaison dans la guitare referme une
boucle de plusieurs mètres carrés —

```
guitare → jack → ampli → terre secteur → alim → carte → câble mini-XLR → guitare
```

— qui capte le 50 Hz et le restitue dans l'ampli. Elle injecterait de surcroît
le bruit de commutation du microcontrôleur dans la référence de masse du signal
audio.

Le circuit de pitch est **fermé sur la carte** : deux fils d'alimentation, deux
retours de mesure. Il ne demande rien à l'instrument.

### Où la liaison se fait toute seule

Le pickguard d'une Stratocaster est presque toujours blindé, et ce blindage
**est** la masse de la guitare.

| Pièce | Risque | Consigne |
|---|---|---|
| Potentiomètre | réflexe de souder la masse au dos du boîtier | **ne rien souder au boîtier.** Le corps peut toucher le blindage : il n'est pas relié aux pistes |
| Sélecteur `46311LDRX` | boîtier acier vissé au pickguard | sans conséquence, contacts isolés du boîtier |
| **Embase mini-XLR** | **le vrai piège** | **mesurer la continuité fût ↔ broche 1 avant de visser** |

Si l'embase relie sa broche 1 à son fût et que le fût touche le blindage, la
boucle est faite sans qu'on s'en aperçoive. Dans ce cas, **rondelles isolantes**
en fibre ou nylon de part et d'autre du pickguard.

### Blindage du câble

Le connecteur de la carte, `TRAPC5M1X`, a un **corps entièrement
thermoplastique** : aucune pièce métallique ne peut y reprendre un blindage.
Celui-ci doit donc voyager sur un conducteur.

Raccorder le drain à la **broche 1** aux deux extrémités. La broche 1 reliant
déjà les masses de bout en bout, cela n'ajoute aucune boucle.

C'est le blindage **du câble** qui compte, pas celui des connecteurs : une fiche
non blindée est sans conséquence ici.

---

## Si du ronflement apparaît

Dans cet ordre — et **jamais** en reliant les masses :

1. Débrancher le drain du blindage côté guitare, le garder côté carte seulement.
2. Vérifier que le 100 nF est bien au plus près de la molette.
3. Éloigner le cheminement du câble des micros.

---

## Pourquoi le bruit capté n'est pas un souci

Ce qui circule n'est pas de l'audio :

| | |
|---|---|
| Impédance de source de la molette | ≤ 2,5 kΩ |
| Impédance de source du sélecteur | ~1,6 kΩ |
| Filtre RC sur la carte | 1 kΩ + 100 nF |
| Un pas de convertisseur | 4,9 mV |
| Largeur d'une fenêtre du sélecteur | 100 pas ≈ 490 mV |

Il faudrait des centaines de millivolts de parasites, en continu, pour faire
basculer une fenêtre. Ajouté à la zone morte de la molette et aux 300 ms de
confirmation du sélecteur, la lecture est insensible au ronflement induit.

Le risque réel va dans l'autre sens : le rail +5 V d'un microcontrôleur court
jusqu'à quelques centimètres de micros simple bobinage. D'où le 100 nF.
