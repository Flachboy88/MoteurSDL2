#ifndef FLAGS_H
#define FLAGS_H

#include "engine/save.h"

typedef struct Flags {
    char noms[512][64];
    int  valeurs[512];
    int  nb_flags;
} Flags;

void definir_flag(Flags *flags, const char *nom, int valeur);
int  flag_actif(Flags *flags, const char *nom);
void sauvegarder_flags(Flags *flags, Fichier_KV *save);
void charger_flags(Flags *flags, Fichier_KV *save);

#endif
