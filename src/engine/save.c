#include "engine/save.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

Fichier_KV lire_fichier_kv(const char *chemin) {
    Fichier_KV f = {0};
    FILE *fp = fopen(chemin, "r");
    if (!fp) return f;

    char ligne[256];
    while (fgets(ligne, sizeof(ligne), fp) && f.nb_entrees < 256) {
        char *nl = strchr(ligne, '\n');
        if (nl) *nl = '\0';
        char *cr = strchr(ligne, '\r');
        if (cr) *cr = '\0';

        if (ligne[0] == '\0' || ligne[0] == '#') continue;

        char *eq = strchr(ligne, '=');
        if (!eq) continue;

        *eq = '\0';
        strncpy(f.cles[f.nb_entrees], ligne, 63);
        f.cles[f.nb_entrees][63] = '\0';
        strncpy(f.valeurs[f.nb_entrees], eq + 1, 127);
        f.valeurs[f.nb_entrees][127] = '\0';
        f.nb_entrees++;
    }
    fclose(fp);
    return f;
}

void ecrire_fichier_kv(const char *chemin, Fichier_KV *fichier) {
    FILE *fp = fopen(chemin, "w");
    if (!fp) return;
    for (int i = 0; i < fichier->nb_entrees; i++) {
        fprintf(fp, "%s=%s\n", fichier->cles[i], fichier->valeurs[i]);
    }
    fclose(fp);
}

const char *obtenir_valeur(Fichier_KV *fichier, const char *cle) {
    for (int i = 0; i < fichier->nb_entrees; i++) {
        if (strcmp(fichier->cles[i], cle) == 0)
            return fichier->valeurs[i];
    }
    return NULL;
}

int obtenir_valeur_int(Fichier_KV *fichier, const char *cle, int defaut) {
    const char *val = obtenir_valeur(fichier, cle);
    if (!val) return defaut;
    return atoi(val);
}

void definir_valeur(Fichier_KV *fichier, const char *cle, const char *valeur) {
    for (int i = 0; i < fichier->nb_entrees; i++) {
        if (strcmp(fichier->cles[i], cle) == 0) {
            strncpy(fichier->valeurs[i], valeur, 127);
            fichier->valeurs[i][127] = '\0';
            return;
        }
    }
    if (fichier->nb_entrees < 256) {
        strncpy(fichier->cles[fichier->nb_entrees], cle, 63);
        fichier->cles[fichier->nb_entrees][63] = '\0';
        strncpy(fichier->valeurs[fichier->nb_entrees], valeur, 127);
        fichier->valeurs[fichier->nb_entrees][127] = '\0';
        fichier->nb_entrees++;
    }
}

void definir_valeur_int(Fichier_KV *fichier, const char *cle, int valeur) {
    char buf[32];
    snprintf(buf, sizeof(buf), "%d", valeur);
    definir_valeur(fichier, cle, buf);
}

void sauvegarder_partie(const char *chemin, Fichier_KV *donnees) {
    ecrire_fichier_kv(chemin, donnees);
}

Fichier_KV charger_partie(const char *chemin) {
    return lire_fichier_kv(chemin);
}

int sauvegarde_existe(const char *chemin) {
    FILE *fp = fopen(chemin, "r");
    if (fp) { fclose(fp); return 1; }
    return 0;
}
