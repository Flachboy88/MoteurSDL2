#ifndef SPRITE_H
#define SPRITE_H

#include <SDL2/SDL.h>

struct Camera;
struct Rendu;

typedef struct Frame {
    int colonne, ligne;
    float duree;
} Frame;

typedef struct Animation {
    char nom[32];
    Frame *frames;
    int nb_frames;
    int boucle;
} Animation;

typedef struct Sprite {
    SDL_Texture *texture;
    int largeur_frame, hauteur_frame;
    Animation *animations;
    int nb_animations;
    int animation_courante;
    int frame_courante;
    float chrono;
} Sprite;

Sprite charger_sprite(SDL_Renderer *renderer, const char *chemin,
                      int larg_frame, int haut_frame);
void   detruire_sprite(Sprite *sprite);
void   ajouter_animation(Sprite *sprite, const char *nom,
                          Frame *frames, int nb_frames, int boucle);
void   jouer_animation(Sprite *sprite, const char *nom);
void   mettre_a_jour_sprite(Sprite *sprite, float delta_time);
int    animation_terminee(Sprite *sprite);
void   afficher_sprite(Sprite *sprite, float x, float y,
                        struct Camera *camera, struct Rendu *rendu);

#endif
