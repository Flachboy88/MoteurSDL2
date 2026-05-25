#include "engine/state.h"
#include <stddef.h>

void empiler_etat(PileEtats *pile, Etat etat) {
    if (pile->sommet >= 16) return;
    pile->etats[pile->sommet] = etat;
    pile->sommet++;
    if (etat.initialiser)
        etat.initialiser(&pile->etats[pile->sommet - 1]);
}

void depiler_etat(PileEtats *pile) {
    if (pile->sommet <= 0) return;
    pile->sommet--;
    Etat *e = &pile->etats[pile->sommet];
    if (e->detruire) e->detruire(e);
}

void changer_etat(PileEtats *pile, Etat etat) {
    depiler_etat(pile);
    empiler_etat(pile, etat);
}

Etat *obtenir_etat_courant(PileEtats *pile) {
    if (pile->sommet <= 0) return NULL;
    return &pile->etats[pile->sommet - 1];
}
