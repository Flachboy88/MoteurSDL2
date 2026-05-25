#ifndef SAVE_H
#define SAVE_H

typedef struct Fichier_KV {
    char cles[256][64];
    char valeurs[256][128];
    int  nb_entrees;
} Fichier_KV;

Fichier_KV  lire_fichier_kv(const char *chemin);
void        ecrire_fichier_kv(const char *chemin, Fichier_KV *fichier);
const char *obtenir_valeur(Fichier_KV *fichier, const char *cle);
int         obtenir_valeur_int(Fichier_KV *fichier, const char *cle, int defaut);
void        definir_valeur(Fichier_KV *fichier, const char *cle, const char *valeur);
void        definir_valeur_int(Fichier_KV *fichier, const char *cle, int valeur);
void        sauvegarder_partie(const char *chemin, Fichier_KV *donnees);
Fichier_KV  charger_partie(const char *chemin);
int         sauvegarde_existe(const char *chemin);

#endif
