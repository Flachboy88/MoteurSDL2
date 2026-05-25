#include "engine/flags.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

void definir_flag(Flags *flags, const char *nom, int valeur) {
    for (int i = 0; i < flags->nb_flags; i++) {
        if (strcmp(flags->noms[i], nom) == 0) {
            flags->valeurs[i] = valeur;
            return;
        }
    }
    if (flags->nb_flags < 512) {
        strncpy(flags->noms[flags->nb_flags], nom, 63);
        flags->noms[flags->nb_flags][63] = '\0';
        flags->valeurs[flags->nb_flags] = valeur;
        flags->nb_flags++;
    }
}

int flag_actif(Flags *flags, const char *nom) {
    for (int i = 0; i < flags->nb_flags; i++) {
        if (strcmp(flags->noms[i], nom) == 0)
            return flags->valeurs[i];
    }
    return 0;
}

void sauvegarder_flags(Flags *flags, Fichier_KV *save) {
    char cle[80];
    for (int i = 0; i < flags->nb_flags; i++) {
        snprintf(cle, sizeof(cle), "flag_%s", flags->noms[i]);
        definir_valeur_int(save, cle, flags->valeurs[i]);
    }
}

void charger_flags(Flags *flags, Fichier_KV *save) {
    for (int i = 0; i < save->nb_entrees; i++) {
        if (strncmp(save->cles[i], "flag_", 5) == 0) {
            const char *nom = save->cles[i] + 5;
            int val = atoi(save->valeurs[i]);
            definir_flag(flags, nom, val);
        }
    }
}
