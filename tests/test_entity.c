#include "test_framework.h"
#include "engine/entity.h"

static int test_creer_entite(void) {
    Entite *e = creer_entite(10.0f, 20.0f, 32, 32);
    ASSERT_NON_NULL(e);
    ASSERT_EGAL_FLOAT(10.0f, e->x, 0.001f);
    ASSERT_EGAL_FLOAT(20.0f, e->y, 0.001f);
    ASSERT_EGAL(32, e->largeur);
    ASSERT_EGAL(32, e->hauteur);
    ASSERT_VRAI(e->active);
    detruire_entite(e);
    return 1;
}

static int test_collision_rect_overlap(void) {
    Entite *a = creer_entite(0.0f, 0.0f, 32, 32);
    Entite *b = creer_entite(16.0f, 16.0f, 32, 32);
    ASSERT_VRAI(collision_rect(a, b));
    detruire_entite(a);
    detruire_entite(b);
    return 1;
}

static int test_collision_rect_no_overlap(void) {
    Entite *a = creer_entite(0.0f, 0.0f, 32, 32);
    Entite *b = creer_entite(100.0f, 100.0f, 32, 32);
    ASSERT_FAUX(collision_rect(a, b));
    detruire_entite(a);
    detruire_entite(b);
    return 1;
}

static int test_collision_rect_adjacent(void) {
    Entite *a = creer_entite(0.0f, 0.0f, 32, 32);
    Entite *b = creer_entite(32.0f, 0.0f, 32, 32);
    ASSERT_FAUX(collision_rect(a, b));
    detruire_entite(a);
    detruire_entite(b);
    return 1;
}

static int test_liste_entites(void) {
    ListeEntites liste = creer_liste_entites(4);
    Entite *e1 = creer_entite(0, 0, 16, 16);
    Entite *e2 = creer_entite(50, 50, 16, 16);
    ajouter_entite(&liste, e1);
    ajouter_entite(&liste, e2);
    ASSERT_EGAL(2, liste.nb);
    detruire_liste_entites(&liste);
    return 1;
}

static int test_tri_profondeur(void) {
    ListeEntites liste = creer_liste_entites(4);
    Entite *e1 = creer_entite(0, 100, 16, 16);
    e1->profondeur = 2;
    Entite *e2 = creer_entite(0, 50, 16, 16);
    e2->profondeur = 1;
    Entite *e3 = creer_entite(0, 200, 16, 16);
    e3->profondeur = 1;
    ajouter_entite(&liste, e1);
    ajouter_entite(&liste, e2);
    ajouter_entite(&liste, e3);
    trier_entites_par_profondeur(&liste);
    ASSERT_EGAL(1, liste.entites[0]->profondeur);
    ASSERT_EGAL(1, liste.entites[1]->profondeur);
    ASSERT_EGAL(2, liste.entites[2]->profondeur);
    ASSERT_VRAI(liste.entites[0]->y <= liste.entites[1]->y);
    detruire_liste_entites(&liste);
    return 1;
}

static void suite_entity(void) {
    printf("--- Entity ---\n");
    LANCER_TEST(test_creer_entite);
    LANCER_TEST(test_collision_rect_overlap);
    LANCER_TEST(test_collision_rect_no_overlap);
    LANCER_TEST(test_collision_rect_adjacent);
    LANCER_TEST(test_liste_entites);
    LANCER_TEST(test_tri_profondeur);
}
