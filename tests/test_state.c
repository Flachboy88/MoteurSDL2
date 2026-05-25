#include "test_framework.h"
#include "engine/state.h"

static int init_appele = 0;
static int detruit_appele = 0;

static void etat_test_init(struct Etat *etat) { (void)etat; init_appele++; }
static void etat_test_detruire(struct Etat *etat) { (void)etat; detruit_appele++; }

static Etat creer_etat_test(void) {
    Etat e = {0};
    e.initialiser = etat_test_init;
    e.detruire = etat_test_detruire;
    return e;
}

static int test_pile_vide(void) {
    PileEtats pile = {0};
    ASSERT_NULL(obtenir_etat_courant(&pile));
    ASSERT_EGAL(0, pile.sommet);
    return 1;
}

static int test_empiler_etat(void) {
    PileEtats pile = {0};
    init_appele = 0;
    empiler_etat(&pile, creer_etat_test());
    ASSERT_EGAL(1, pile.sommet);
    ASSERT_NON_NULL(obtenir_etat_courant(&pile));
    ASSERT_EGAL(1, init_appele);
    return 1;
}

static int test_depiler_etat(void) {
    PileEtats pile = {0};
    detruit_appele = 0;
    empiler_etat(&pile, creer_etat_test());
    depiler_etat(&pile);
    ASSERT_EGAL(0, pile.sommet);
    ASSERT_NULL(obtenir_etat_courant(&pile));
    ASSERT_EGAL(1, detruit_appele);
    return 1;
}

static int test_changer_etat(void) {
    PileEtats pile = {0};
    init_appele = 0;
    detruit_appele = 0;
    empiler_etat(&pile, creer_etat_test());
    changer_etat(&pile, creer_etat_test());
    ASSERT_EGAL(1, pile.sommet);
    ASSERT_EGAL(2, init_appele);
    ASSERT_EGAL(1, detruit_appele);
    return 1;
}

static int test_empiler_plusieurs(void) {
    PileEtats pile = {0};
    empiler_etat(&pile, creer_etat_test());
    empiler_etat(&pile, creer_etat_test());
    empiler_etat(&pile, creer_etat_test());
    ASSERT_EGAL(3, pile.sommet);
    depiler_etat(&pile);
    ASSERT_EGAL(2, pile.sommet);
    return 1;
}

static void suite_state(void) {
    printf("--- State Machine ---\n");
    LANCER_TEST(test_pile_vide);
    LANCER_TEST(test_empiler_etat);
    LANCER_TEST(test_depiler_etat);
    LANCER_TEST(test_changer_etat);
    LANCER_TEST(test_empiler_plusieurs);
}
