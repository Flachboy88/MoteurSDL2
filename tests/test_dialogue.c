#include "test_framework.h"
#include "engine/dialogue.h"
#include "engine/flags.h"
#include <stdio.h>

static void ecrire_fichier_test_dlg(const char *chemin) {
    FILE *fp = fopen(chemin, "w");
    fprintf(fp, "[pnj_maman]\n");
    fprintf(fp, "condition: !potion_recue\n");
    fprintf(fp, "texte: \"Prends ca pour la route !\"\n");
    fprintf(fp, "action: donner_objet potion 1\n");
    fprintf(fp, "action: set_flag potion_recue\n");
    fprintf(fp, "\n");
    fprintf(fp, "[pnj_maman]\n");
    fprintf(fp, "condition: potion_recue\n");
    fprintf(fp, "texte: \"Sois prudent dehors !\"\n");
    fclose(fp);
}

static int test_charger_dialogues(void) {
    ecrire_fichier_test_dlg("bin/test_dialogue.dlg");
    FichierDialogues *d = charger_dialogues("bin/test_dialogue.dlg");
    ASSERT_NON_NULL(d);
    ASSERT_EGAL_STR("pnj_maman", d->nom_pnj);
    ASSERT_EGAL(2, d->nb_blocs);
    remove("bin/test_dialogue.dlg");
    detruire_dialogues(d);
    return 1;
}

static int test_bloc_condition(void) {
    ecrire_fichier_test_dlg("bin/test_dialogue2.dlg");
    FichierDialogues *d = charger_dialogues("bin/test_dialogue2.dlg");
    ASSERT_EGAL_STR("!potion_recue", d->blocs[0].condition);
    ASSERT_EGAL(1, d->blocs[0].nb_lignes);
    ASSERT_EGAL_STR("Prends ca pour la route !", d->blocs[0].lignes_texte[0]);
    ASSERT_EGAL(2, d->blocs[0].nb_actions);
    ASSERT_EGAL(ACTION_DLG_DONNER_OBJET, d->blocs[0].actions[0].type);
    ASSERT_EGAL(ACTION_DLG_SET_FLAG, d->blocs[0].actions[1].type);
    remove("bin/test_dialogue2.dlg");
    detruire_dialogues(d);
    return 1;
}

static int test_trouver_dialogue_sans_flag(void) {
    ecrire_fichier_test_dlg("bin/test_dialogue3.dlg");
    FichierDialogues *d = charger_dialogues("bin/test_dialogue3.dlg");
    Flags flags = {0};
    BlocDialogue *b = trouver_dialogue(d, "pnj_maman", &flags);
    ASSERT_NON_NULL(b);
    ASSERT_EGAL_STR("Prends ca pour la route !", b->lignes_texte[0]);
    remove("bin/test_dialogue3.dlg");
    detruire_dialogues(d);
    return 1;
}

static int test_trouver_dialogue_avec_flag(void) {
    ecrire_fichier_test_dlg("bin/test_dialogue4.dlg");
    FichierDialogues *d = charger_dialogues("bin/test_dialogue4.dlg");
    Flags flags = {0};
    definir_flag(&flags, "potion_recue", 1);
    BlocDialogue *b = trouver_dialogue(d, "pnj_maman", &flags);
    ASSERT_NON_NULL(b);
    ASSERT_EGAL_STR("Sois prudent dehors !", b->lignes_texte[0]);
    remove("bin/test_dialogue4.dlg");
    detruire_dialogues(d);
    return 1;
}

static int test_trouver_dialogue_mauvais_pnj(void) {
    ecrire_fichier_test_dlg("bin/test_dialogue5.dlg");
    FichierDialogues *d = charger_dialogues("bin/test_dialogue5.dlg");
    Flags flags = {0};
    BlocDialogue *b = trouver_dialogue(d, "pnj_inconnu", &flags);
    ASSERT_NULL(b);
    remove("bin/test_dialogue5.dlg");
    detruire_dialogues(d);
    return 1;
}

static void suite_dialogue(void) {
    printf("--- Dialogue ---\n");
    LANCER_TEST(test_charger_dialogues);
    LANCER_TEST(test_bloc_condition);
    LANCER_TEST(test_trouver_dialogue_sans_flag);
    LANCER_TEST(test_trouver_dialogue_avec_flag);
    LANCER_TEST(test_trouver_dialogue_mauvais_pnj);
}
