#include "test_framework.h"
#include "engine/save.h"
#include <stdio.h>

static int test_definir_et_obtenir_valeur(void) {
    Fichier_KV fkv = {0};
    definir_valeur(&fkv, "nom", "test");
    ASSERT_EGAL_STR("test", obtenir_valeur(&fkv, "nom"));
    ASSERT_EGAL(1, fkv.nb_entrees);
    return 1;
}

static int test_definir_valeur_int(void) {
    Fichier_KV fkv = {0};
    definir_valeur_int(&fkv, "score", 42);
    ASSERT_EGAL(42, obtenir_valeur_int(&fkv, "score", 0));
    return 1;
}

static int test_valeur_defaut(void) {
    Fichier_KV fkv = {0};
    ASSERT_EGAL(99, obtenir_valeur_int(&fkv, "inexistant", 99));
    ASSERT_NULL(obtenir_valeur(&fkv, "inexistant"));
    return 1;
}

static int test_ecrire_et_lire_fichier(void) {
    Fichier_KV fkv = {0};
    definir_valeur(&fkv, "nom", "joueur1");
    definir_valeur_int(&fkv, "niveau", 5);
    ecrire_fichier_kv("bin/test_temp.kv", &fkv);

    Fichier_KV lu = lire_fichier_kv("bin/test_temp.kv");
    ASSERT_EGAL(2, lu.nb_entrees);
    ASSERT_EGAL_STR("joueur1", obtenir_valeur(&lu, "nom"));
    ASSERT_EGAL(5, obtenir_valeur_int(&lu, "niveau", 0));

    remove("bin/test_temp.kv");
    return 1;
}

static int test_mise_a_jour_valeur(void) {
    Fichier_KV fkv = {0};
    definir_valeur(&fkv, "cle", "ancien");
    definir_valeur(&fkv, "cle", "nouveau");
    ASSERT_EGAL(1, fkv.nb_entrees);
    ASSERT_EGAL_STR("nouveau", obtenir_valeur(&fkv, "cle"));
    return 1;
}

static int test_sauvegarde_existe(void) {
    Fichier_KV fkv = {0};
    definir_valeur(&fkv, "test", "ok");
    ecrire_fichier_kv("bin/test_save_exist.sav", &fkv);
    ASSERT_VRAI(sauvegarde_existe("bin/test_save_exist.sav"));
    ASSERT_FAUX(sauvegarde_existe("bin/inexistant.sav"));
    remove("bin/test_save_exist.sav");
    return 1;
}

static void suite_save(void) {
    printf("--- Save/KV ---\n");
    LANCER_TEST(test_definir_et_obtenir_valeur);
    LANCER_TEST(test_definir_valeur_int);
    LANCER_TEST(test_valeur_defaut);
    LANCER_TEST(test_ecrire_et_lire_fichier);
    LANCER_TEST(test_mise_a_jour_valeur);
    LANCER_TEST(test_sauvegarde_existe);
}
