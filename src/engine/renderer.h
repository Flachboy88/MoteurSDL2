#ifndef RENDERER_H
#define RENDERER_H

#include <SDL2/SDL.h>
#include "engine/camera.h"

typedef struct Couleur {
    Uint8 r, g, b, a;
} Couleur;

typedef struct Rendu {
    SDL_Renderer *renderer;
    Camera *camera;
} Rendu;

void effacer_ecran(Rendu *rendu);
void presenter_rendu(Rendu *rendu);
void dessiner_rect(Rendu *rendu, int x, int y, int w, int h, Couleur couleur);

#endif
