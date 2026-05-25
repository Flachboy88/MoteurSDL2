#include "core/window.h"
#include <stdio.h>

Fenetre creer_fenetre(const char *titre, int largeur, int hauteur) {
    Fenetre f = {0};
    f.largeur = largeur;
    f.hauteur = hauteur;

    f.window = SDL_CreateWindow(
        titre,
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        largeur, hauteur,
        SDL_WINDOW_SHOWN
    );
    if (!f.window) {
        fprintf(stderr, "SDL_CreateWindow: %s\n", SDL_GetError());
        return f;
    }

    f.renderer = SDL_CreateRenderer(f.window, -1, SDL_RENDERER_ACCELERATED);
    if (!f.renderer) {
        fprintf(stderr, "SDL_CreateRenderer: %s\n", SDL_GetError());
        SDL_DestroyWindow(f.window);
        f.window = NULL;
        return f;
    }
    return f;
}

void detruire_fenetre(Fenetre *fenetre) {
    if (fenetre->renderer) SDL_DestroyRenderer(fenetre->renderer);
    if (fenetre->window) SDL_DestroyWindow(fenetre->window);
    fenetre->renderer = NULL;
    fenetre->window = NULL;
}
