#include "engine/dialogue.h"
#include "engine/flags.h"
#include "engine/text.h"
#include "engine/renderer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int evaluer_condition(const char *condition, struct Flags *flags) {
    if (condition[0] == '\0') return 1;
    if (condition[0] == '!') {
        return !flag_actif(flags, condition + 1);
    }
    return flag_actif(flags, condition);
}

FichierDialogues *charger_dialogues(const char *chemin) {
    FILE *fp = fopen(chemin, "r");
    if (!fp) return NULL;

    FichierDialogues *d = calloc(1, sizeof(FichierDialogues));
    char ligne[512];
    BlocDialogue *bloc = NULL;

    while (fgets(ligne, sizeof(ligne), fp)) {
        char *nl = strchr(ligne, '\n');
        if (nl) *nl = '\0';
        char *cr = strchr(ligne, '\r');
        if (cr) *cr = '\0';
        if (ligne[0] == '\0') continue;

        if (ligne[0] == '[') {
            char *end = strchr(ligne, ']');
            if (!end) continue;
            *end = '\0';
            if (d->nom_pnj[0] == '\0')
                strncpy(d->nom_pnj, ligne + 1, 63);

            d->nb_blocs++;
            d->blocs = realloc(d->blocs, d->nb_blocs * sizeof(BlocDialogue));
            bloc = &d->blocs[d->nb_blocs - 1];
            memset(bloc, 0, sizeof(BlocDialogue));
        }
        else if (bloc && strncmp(ligne, "condition: ", 11) == 0) {
            strncpy(bloc->condition, ligne + 11, 127);
        }
        else if (bloc && strncmp(ligne, "texte: ", 7) == 0) {
            char *texte = ligne + 7;
            if (texte[0] == '"') texte++;
            char *eq = strrchr(texte, '"');
            if (eq) *eq = '\0';

            bloc->nb_lignes++;
            bloc->lignes_texte = realloc(bloc->lignes_texte,
                                          bloc->nb_lignes * sizeof(char *));
            int len = (int)strlen(texte);
            char *copie = malloc(len + 1);
            memcpy(copie, texte, len + 1);
            bloc->lignes_texte[bloc->nb_lignes - 1] = copie;
        }
        else if (bloc && strncmp(ligne, "action: ", 8) == 0) {
            char *params = ligne + 8;
            LigneAction action = {0};

            if (strncmp(params, "donner_objet ", 13) == 0) {
                action.type = ACTION_DLG_DONNER_OBJET;
                strncpy(action.params, params + 13, 127);
            } else if (strncmp(params, "set_flag ", 9) == 0) {
                action.type = ACTION_DLG_SET_FLAG;
                strncpy(action.params, params + 9, 127);
            } else if (strncmp(params, "set_flag", 8) == 0) {
                action.type = ACTION_DLG_SET_FLAG;
                strncpy(action.params, params + 9, 127);
            } else if (strncmp(params, "jouer_son ", 10) == 0) {
                action.type = ACTION_DLG_JOUER_SON;
                strncpy(action.params, params + 10, 127);
            } else if (strncmp(params, "soigner", 7) == 0) {
                action.type = ACTION_DLG_SOIGNER;
            }

            bloc->nb_actions++;
            bloc->actions = realloc(bloc->actions,
                                     bloc->nb_actions * sizeof(LigneAction));
            bloc->actions[bloc->nb_actions - 1] = action;
        }
    }
    fclose(fp);
    return d;
}

void detruire_dialogues(FichierDialogues *dialogues) {
    if (!dialogues) return;
    for (int i = 0; i < dialogues->nb_blocs; i++) {
        BlocDialogue *b = &dialogues->blocs[i];
        for (int j = 0; j < b->nb_lignes; j++)
            free(b->lignes_texte[j]);
        free(b->lignes_texte);
        free(b->actions);
    }
    free(dialogues->blocs);
    free(dialogues);
}

BlocDialogue *trouver_dialogue(FichierDialogues *dialogues,
                                const char *nom_pnj,
                                struct Flags *flags) {
    if (strcmp(dialogues->nom_pnj, nom_pnj) != 0) return NULL;
    for (int i = 0; i < dialogues->nb_blocs; i++) {
        BlocDialogue *b = &dialogues->blocs[i];
        if (evaluer_condition(b->condition, flags))
            return b;
    }
    return NULL;
}

BoiteDialogue creer_boite_dialogue(struct Police *police, float vitesse) {
    BoiteDialogue b = {0};
    b.police = police;
    b.vitesse = vitesse;
    return b;
}

void afficher_dialogue_texte(BoiteDialogue *boite, const char *texte) {
    strncpy(boite->texte_complet, texte, 511);
    boite->texte_complet[511] = '\0';
    boite->caracteres_affiches = 0;
    boite->chrono = 0.0f;
    boite->terminee = 0;
}

void mettre_a_jour_dialogue(BoiteDialogue *boite, float delta_time) {
    if (boite->terminee) return;
    int total = (int)strlen(boite->texte_complet);
    boite->chrono += delta_time;
    if (boite->chrono >= boite->vitesse) {
        boite->chrono -= boite->vitesse;
        boite->caracteres_affiches++;
        if (boite->caracteres_affiches >= total)
            boite->terminee = 1;
    }
}

void dessiner_dialogue(BoiteDialogue *boite, struct Rendu *rendu) {
    if (!boite->police || boite->caracteres_affiches <= 0) return;
    char buf[512];
    int n = boite->caracteres_affiches;
    if (n > 511) n = 511;
    strncpy(buf, boite->texte_complet, n);
    buf[n] = '\0';

    Couleur blanc = {255, 255, 255, 255};
    Couleur fond = {0, 0, 0, 200};
    dessiner_rect(rendu, 20, 400, 760, 150, fond);
    afficher_texte(rendu, boite->police, buf, 30, 420, blanc);
}

void passer_dialogue(BoiteDialogue *boite) {
    boite->caracteres_affiches = (int)strlen(boite->texte_complet);
    boite->terminee = 1;
}

int dialogue_termine(BoiteDialogue *boite) {
    return boite->terminee;
}
