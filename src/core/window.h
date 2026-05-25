#ifndef WINDOW_H
#define WINDOW_H

#include <SDL2/SDL.h>

typedef struct Fenetre {
    SDL_Window   *window;
    SDL_Renderer *renderer;
    int largeur, hauteur;
} Fenetre;

Fenetre creer_fenetre(const char *titre, int largeur, int hauteur);
void    detruire_fenetre(Fenetre *fenetre);

#endif
