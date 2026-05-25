#ifndef INPUT_H
#define INPUT_H

#include <SDL2/SDL.h>
#include "engine/save.h"

typedef enum {
    ACTION_HAUT, ACTION_BAS, ACTION_GAUCHE, ACTION_DROITE,
    ACTION_VALIDER, ACTION_ANNULER, ACTION_MENU,
    ACTION_MAX
} Action;

typedef struct Entree {
    int actions[ACTION_MAX];
    int actions_pressees[ACTION_MAX];
    SDL_Keycode touches[ACTION_MAX];
    int quitter;
} Entree;

Entree  creer_entree(void);
void    mettre_a_jour_entrees(Entree *entree);
int     action_active(Entree *entree, Action action);
int     action_pressee(Entree *entree, Action action);
void    configurer_touches(Entree *entree, Fichier_KV *config);

#endif
