#include "core/input.h"
#include <string.h>

Entree creer_entree(void) {
    Entree e = {0};
    e.touches[ACTION_HAUT]    = SDLK_UP;
    e.touches[ACTION_BAS]     = SDLK_DOWN;
    e.touches[ACTION_GAUCHE]  = SDLK_LEFT;
    e.touches[ACTION_DROITE]  = SDLK_RIGHT;
    e.touches[ACTION_VALIDER] = SDLK_RETURN;
    e.touches[ACTION_ANNULER] = SDLK_ESCAPE;
    e.touches[ACTION_MENU]    = SDLK_m;
    return e;
}

static Action trouver_action(Entree *entree, SDL_Keycode touche) {
    for (int i = 0; i < ACTION_MAX; i++) {
        if (entree->touches[i] == touche) return (Action)i;
    }
    return ACTION_MAX;
}

void mettre_a_jour_entrees(Entree *entree) {
    memset(entree->actions_pressees, 0, sizeof(entree->actions_pressees));
    entree->quitter = 0;

    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) {
            entree->quitter = 1;
        }
        if (event.type == SDL_KEYDOWN && !event.key.repeat) {
            Action a = trouver_action(entree, event.key.keysym.sym);
            if (a != ACTION_MAX) {
                entree->actions[a] = 1;
                entree->actions_pressees[a] = 1;
            }
        }
        if (event.type == SDL_KEYUP) {
            Action a = trouver_action(entree, event.key.keysym.sym);
            if (a != ACTION_MAX) {
                entree->actions[a] = 0;
            }
        }
    }
}

int action_active(Entree *entree, Action action) {
    if (action < 0 || action >= ACTION_MAX) return 0;
    return entree->actions[action];
}

int action_pressee(Entree *entree, Action action) {
    if (action < 0 || action >= ACTION_MAX) return 0;
    return entree->actions_pressees[action];
}

void configurer_touches(Entree *entree, Fichier_KV *config) {
    const char *noms[] = {
        "touche_haut", "touche_bas", "touche_gauche", "touche_droite",
        "touche_valider", "touche_annuler", "touche_menu"
    };
    for (int i = 0; i < ACTION_MAX; i++) {
        const char *val = obtenir_valeur(config, noms[i]);
        if (val && strlen(val) == 1) {
            entree->touches[i] = (SDL_Keycode)val[0];
        }
    }
}
