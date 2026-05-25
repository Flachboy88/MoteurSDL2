#ifndef TEXT_H
#define TEXT_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include "engine/renderer.h"

typedef struct Police {
    TTF_Font *font;
    int taille;
} Police;

Police charger_police(const char *chemin, int taille);
void   detruire_police(Police *police);
void   afficher_texte(Rendu *rendu, Police *police, const char *texte,
                       int x, int y, Couleur couleur);

#endif
