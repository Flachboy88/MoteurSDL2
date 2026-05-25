#include "engine/sprite.h"
#include "engine/camera.h"
#include "engine/renderer.h"
#include <SDL2/SDL_image.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

Sprite charger_sprite(SDL_Renderer *renderer, const char *chemin,
                      int larg_frame, int haut_frame) {
    Sprite s = {0};
    s.largeur_frame = larg_frame;
    s.hauteur_frame = haut_frame;

    SDL_Surface *surface = IMG_Load(chemin);
    if (!surface) {
        fprintf(stderr, "IMG_Load(%s): %s\n", chemin, IMG_GetError());
        return s;
    }
    s.texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);
    if (!s.texture) {
        fprintf(stderr, "SDL_CreateTextureFromSurface: %s\n", SDL_GetError());
    }
    return s;
}

void detruire_sprite(Sprite *sprite) {
    if (sprite->texture) SDL_DestroyTexture(sprite->texture);
    for (int i = 0; i < sprite->nb_animations; i++) {
        free(sprite->animations[i].frames);
    }
    free(sprite->animations);
    sprite->texture = NULL;
    sprite->animations = NULL;
    sprite->nb_animations = 0;
}

void ajouter_animation(Sprite *sprite, const char *nom,
                        Frame *frames, int nb_frames, int boucle) {
    sprite->nb_animations++;
    sprite->animations = realloc(sprite->animations,
                                  sprite->nb_animations * sizeof(Animation));
    Animation *a = &sprite->animations[sprite->nb_animations - 1];
    strncpy(a->nom, nom, 31);
    a->nom[31] = '\0';
    a->frames = malloc(nb_frames * sizeof(Frame));
    memcpy(a->frames, frames, nb_frames * sizeof(Frame));
    a->nb_frames = nb_frames;
    a->boucle = boucle;
}

void jouer_animation(Sprite *sprite, const char *nom) {
    for (int i = 0; i < sprite->nb_animations; i++) {
        if (strcmp(sprite->animations[i].nom, nom) == 0) {
            if (sprite->animation_courante != i) {
                sprite->animation_courante = i;
                sprite->frame_courante = 0;
                sprite->chrono = 0.0f;
            }
            return;
        }
    }
}

void mettre_a_jour_sprite(Sprite *sprite, float delta_time) {
    if (sprite->nb_animations == 0) return;
    Animation *a = &sprite->animations[sprite->animation_courante];
    if (a->nb_frames <= 1) return;

    sprite->chrono += delta_time;
    Frame *f = &a->frames[sprite->frame_courante];
    if (sprite->chrono >= f->duree) {
        sprite->chrono -= f->duree;
        sprite->frame_courante++;
        if (sprite->frame_courante >= a->nb_frames) {
            sprite->frame_courante = a->boucle ? 0 : a->nb_frames - 1;
        }
    }
}

int animation_terminee(Sprite *sprite) {
    if (sprite->nb_animations == 0) return 1;
    Animation *a = &sprite->animations[sprite->animation_courante];
    return !a->boucle && sprite->frame_courante >= a->nb_frames - 1;
}

void afficher_sprite(Sprite *sprite, float x, float y,
                      struct Camera *camera, struct Rendu *rendu) {
    if (!sprite->texture) return;
    if (sprite->nb_animations == 0) return;
    Animation *a = &sprite->animations[sprite->animation_courante];
    Frame *f = &a->frames[sprite->frame_courante];

    SDL_Rect src = {
        f->colonne * sprite->largeur_frame,
        f->ligne * sprite->hauteur_frame,
        sprite->largeur_frame,
        sprite->hauteur_frame
    };

    float ox = camera ? camera->x : 0;
    float oy = camera ? camera->y : 0;
    SDL_Rect dst = {
        (int)(x - ox),
        (int)(y - oy),
        sprite->largeur_frame,
        sprite->hauteur_frame
    };

    SDL_RenderCopy(rendu->renderer, sprite->texture, &src, &dst);
}
