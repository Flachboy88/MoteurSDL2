#ifndef TILEMAP_H
#define TILEMAP_H

#include <SDL2/SDL.h>

struct Camera;
struct Rendu;
struct ListeEntites;

typedef struct Couche {
    int *tuiles;
    int colonnes, lignes;
} Couche;

typedef struct Objet {
    char nom[64];
    float x, y;
    int largeur, hauteur;
    char type[32];
} Objet;

typedef struct ListeObjets {
    Objet *objets;
    int nb;
} ListeObjets;

typedef struct Carte {
    int colonnes, lignes;
    int taille_tile;
    Couche sol;
    Couche objets_bas;
    Couche objets_haut;
    Couche collision;
    ListeObjets objets;
    SDL_Texture *tileset;
    int tileset_colonnes;
} Carte;

Carte   charger_carte(SDL_Renderer *renderer, const char *chemin_map);
void    detruire_carte(Carte *carte);
void    afficher_couche(Couche *couche, Carte *carte,
                         struct Camera *camera, struct Rendu *rendu);
int     case_praticable(Carte *carte, int colonne, int ligne);
Objet  *chercher_objet(Carte *carte, const char *nom);
void    afficher_scene(Carte *carte, struct ListeEntites *entites,
                        struct Camera *camera, struct Rendu *rendu);

#endif
