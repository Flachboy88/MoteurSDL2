#ifndef ENTITY_H
#define ENTITY_H

struct Sprite;
struct Rendu;

typedef struct Entite {
    float x, y;
    int largeur, hauteur;
    int direction;
    int profondeur;
    struct Sprite *sprite;
    int active;

    void (*mettre_a_jour)(struct Entite *self, float delta_time);
    void (*afficher)(struct Entite *self, struct Rendu *rendu);
    void (*detruire)(struct Entite *self);
    void *donnees;
} Entite;

typedef struct ListeEntites {
    Entite **entites;
    int nb;
    int capacite;
} ListeEntites;

Entite       *creer_entite(float x, float y, int largeur, int hauteur);
void          detruire_entite(Entite *entite);
ListeEntites  creer_liste_entites(int capacite);
void          ajouter_entite(ListeEntites *liste, Entite *entite);
void          maj_toutes_entites(ListeEntites *liste, float delta_time);
void          trier_entites_par_profondeur(ListeEntites *liste);
void          afficher_toutes_entites(ListeEntites *liste, struct Rendu *rendu);
void          detruire_liste_entites(ListeEntites *liste);
int           collision_rect(Entite *a, Entite *b);

#endif
