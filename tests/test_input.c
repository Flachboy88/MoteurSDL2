#include "test_framework.h"
#include "core/input.h"

static int test_creer_entree(void) {
    Entree e = creer_entree();
    ASSERT_FAUX(action_active(&e, ACTION_HAUT));
    ASSERT_FAUX(action_active(&e, ACTION_VALIDER));
    ASSERT_FAUX(e.quitter);
    return 1;
}

static int test_touches_par_defaut(void) {
    Entree e = creer_entree();
    ASSERT_EGAL(SDLK_UP, e.touches[ACTION_HAUT]);
    ASSERT_EGAL(SDLK_DOWN, e.touches[ACTION_BAS]);
    ASSERT_EGAL(SDLK_LEFT, e.touches[ACTION_GAUCHE]);
    ASSERT_EGAL(SDLK_RIGHT, e.touches[ACTION_DROITE]);
    ASSERT_EGAL(SDLK_RETURN, e.touches[ACTION_VALIDER]);
    ASSERT_EGAL(SDLK_ESCAPE, e.touches[ACTION_ANNULER]);
    return 1;
}

static int test_action_manuelle(void) {
    Entree e = creer_entree();
    e.actions[ACTION_HAUT] = 1;
    ASSERT_VRAI(action_active(&e, ACTION_HAUT));
    ASSERT_FAUX(action_active(&e, ACTION_BAS));
    return 1;
}

static int test_action_pressee(void) {
    Entree e = creer_entree();
    e.actions_pressees[ACTION_VALIDER] = 1;
    ASSERT_VRAI(action_pressee(&e, ACTION_VALIDER));
    ASSERT_FAUX(action_pressee(&e, ACTION_ANNULER));
    return 1;
}

static int test_configurer_touches(void) {
    Entree e = creer_entree();
    Fichier_KV config = {0};
    definir_valeur(&config, "touche_haut", "w");
    definir_valeur(&config, "touche_bas", "s");
    configurer_touches(&e, &config);
    ASSERT_EGAL(SDLK_w, e.touches[ACTION_HAUT]);
    ASSERT_EGAL(SDLK_s, e.touches[ACTION_BAS]);
    return 1;
}

static void suite_input(void) {
    printf("--- Input ---\n");
    LANCER_TEST(test_creer_entree);
    LANCER_TEST(test_touches_par_defaut);
    LANCER_TEST(test_action_manuelle);
    LANCER_TEST(test_action_pressee);
    LANCER_TEST(test_configurer_touches);
}
