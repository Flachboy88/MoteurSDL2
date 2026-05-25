#include "engine/renderer.h"

void effacer_ecran(Rendu *rendu) {
    SDL_SetRenderDrawColor(rendu->renderer, 0, 0, 0, 255);
    SDL_RenderClear(rendu->renderer);
}

void presenter_rendu(Rendu *rendu) {
    SDL_RenderPresent(rendu->renderer);
}

void dessiner_rect(Rendu *rendu, int x, int y, int w, int h, Couleur couleur) {
    int ox = rendu->camera ? (int)rendu->camera->x : 0;
    int oy = rendu->camera ? (int)rendu->camera->y : 0;
    SDL_Rect rect = { x - ox, y - oy, w, h };
    SDL_SetRenderDrawColor(rendu->renderer, couleur.r, couleur.g, couleur.b, couleur.a);
    SDL_RenderFillRect(rendu->renderer, &rect);
}
