#include "engine/text.h"
#include <stdio.h>

Police charger_police(const char *chemin, int taille) {
    Police p = {0};
    p.taille = taille;
    p.font = TTF_OpenFont(chemin, taille);
    if (!p.font) {
        fprintf(stderr, "TTF_OpenFont(%s): %s\n", chemin, TTF_GetError());
    }
    return p;
}

void detruire_police(Police *police) {
    if (police->font) TTF_CloseFont(police->font);
    police->font = NULL;
}

void afficher_texte(Rendu *rendu, Police *police, const char *texte,
                     int x, int y, Couleur couleur) {
    if (!police->font || !texte || texte[0] == '\0') return;

    SDL_Color sdl_couleur = { couleur.r, couleur.g, couleur.b, couleur.a };
    SDL_Surface *surface = TTF_RenderUTF8_Blended(police->font, texte, sdl_couleur);
    if (!surface) return;

    SDL_Texture *texture = SDL_CreateTextureFromSurface(rendu->renderer, surface);
    SDL_Rect dst = { x, y, surface->w, surface->h };
    SDL_FreeSurface(surface);

    if (texture) {
        SDL_RenderCopy(rendu->renderer, texture, NULL, &dst);
        SDL_DestroyTexture(texture);
    }
}
