#include "engine/entity.h"
#include <stdlib.h>
#include <string.h>

Entite *creer_entite(float x, float y, int largeur, int hauteur) {
    Entite *e = calloc(1, sizeof(Entite));
    if (!e) return NULL;
    e->x = x;
    e->y = y;
    e->largeur = largeur;
    e->hauteur = hauteur;
    e->active = 1;
    return e;
}

void detruire_entite(Entite *entite) {
    if (!entite) return;
    if (entite->detruire) entite->detruire(entite);
    free(entite);
}

ListeEntites creer_liste_entites(int capacite) {
    ListeEntites l = {0};
    l.entites = calloc(capacite, sizeof(Entite *));
    l.capacite = capacite;
    return l;
}

void ajouter_entite(ListeEntites *liste, Entite *entite) {
    if (liste->nb >= liste->capacite) {
        liste->capacite *= 2;
        liste->entites = realloc(liste->entites, liste->capacite * sizeof(Entite *));
    }
    liste->entites[liste->nb++] = entite;
}

void maj_toutes_entites(ListeEntites *liste, float delta_time) {
    for (int i = 0; i < liste->nb; i++) {
        Entite *e = liste->entites[i];
        if (e->active && e->mettre_a_jour)
            e->mettre_a_jour(e, delta_time);
    }
}

static int comparer_entites(const void *a, const void *b) {
    Entite *ea = *(Entite **)a;
    Entite *eb = *(Entite **)b;
    if (ea->profondeur != eb->profondeur)
        return ea->profondeur - eb->profondeur;
    if (ea->y < eb->y) return -1;
    if (ea->y > eb->y) return 1;
    return 0;
}

void trier_entites_par_profondeur(ListeEntites *liste) {
    qsort(liste->entites, liste->nb, sizeof(Entite *), comparer_entites);
}

void afficher_toutes_entites(ListeEntites *liste, struct Rendu *rendu) {
    for (int i = 0; i < liste->nb; i++) {
        Entite *e = liste->entites[i];
        if (e->active && e->afficher)
            e->afficher(e, rendu);
    }
}

void detruire_liste_entites(ListeEntites *liste) {
    for (int i = 0; i < liste->nb; i++) {
        detruire_entite(liste->entites[i]);
    }
    free(liste->entites);
    liste->entites = NULL;
    liste->nb = 0;
    liste->capacite = 0;
}

int collision_rect(Entite *a, Entite *b) {
    return a->x < b->x + b->largeur &&
           a->x + a->largeur > b->x &&
           a->y < b->y + b->hauteur &&
           a->y + a->hauteur > b->y;
}
