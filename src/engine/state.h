#ifndef STATE_H
#define STATE_H

struct Entree;
struct Rendu;

typedef struct Etat {
    void (*initialiser)(struct Etat *etat);
    void (*gerer_evenements)(struct Etat *etat, struct Entree *entree);
    void (*mettre_a_jour)(struct Etat *etat, float delta_time);
    void (*afficher)(struct Etat *etat, struct Rendu *rendu);
    void (*detruire)(struct Etat *etat);
    void *donnees;
} Etat;

typedef struct PileEtats {
    Etat etats[16];
    int sommet;
} PileEtats;

void  empiler_etat(PileEtats *pile, Etat etat);
void  depiler_etat(PileEtats *pile);
void  changer_etat(PileEtats *pile, Etat etat);
Etat *obtenir_etat_courant(PileEtats *pile);

#endif
