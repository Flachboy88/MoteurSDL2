#include "test_framework.h"
#include "engine/flags.h"

static int test_flag_inexistant(void) {
    Flags flags = {0};
    ASSERT_FAUX(flag_actif(&flags, "test"));
    return 1;
}

static int test_definir_flag(void) {
    Flags flags = {0};
    definir_flag(&flags, "porte_ouverte", 1);
    ASSERT_VRAI(flag_actif(&flags, "porte_ouverte"));
    ASSERT_EGAL(1, flags.nb_flags);
    return 1;
}

static int test_modifier_flag(void) {
    Flags flags = {0};
    definir_flag(&flags, "porte_ouverte", 1);
    definir_flag(&flags, "porte_ouverte", 0);
    ASSERT_FAUX(flag_actif(&flags, "porte_ouverte"));
    ASSERT_EGAL(1, flags.nb_flags);
    return 1;
}

static int test_plusieurs_flags(void) {
    Flags flags = {0};
    definir_flag(&flags, "a", 1);
    definir_flag(&flags, "b", 0);
    definir_flag(&flags, "c", 1);
    ASSERT_VRAI(flag_actif(&flags, "a"));
    ASSERT_FAUX(flag_actif(&flags, "b"));
    ASSERT_VRAI(flag_actif(&flags, "c"));
    ASSERT_EGAL(3, flags.nb_flags);
    return 1;
}

static int test_sauvegarder_charger_flags(void) {
    Flags flags = {0};
    definir_flag(&flags, "quete_1", 1);
    definir_flag(&flags, "boss_vaincu", 1);

    Fichier_KV save = {0};
    sauvegarder_flags(&flags, &save);

    Flags flags2 = {0};
    charger_flags(&flags2, &save);
    ASSERT_VRAI(flag_actif(&flags2, "quete_1"));
    ASSERT_VRAI(flag_actif(&flags2, "boss_vaincu"));
    return 1;
}

static void suite_flags(void) {
    printf("--- Flags ---\n");
    LANCER_TEST(test_flag_inexistant);
    LANCER_TEST(test_definir_flag);
    LANCER_TEST(test_modifier_flag);
    LANCER_TEST(test_plusieurs_flags);
    LANCER_TEST(test_sauvegarder_charger_flags);
}
