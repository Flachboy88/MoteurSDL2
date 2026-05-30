#ifndef MOTEUR_H
#define MOTEUR_H

#include "core/core.h"
#include "core/window.h"
#include "core/timer.h"
#include "core/input.h"
#include "engine/state.h"
#include "engine/entity.h"
#include "engine/camera.h"
#include "engine/renderer.h"
#include "engine/sprite.h"
#include "engine/tilemap.h"
#include "engine/text.h"
#include "engine/flags.h"
#include "engine/save.h"
#include "engine/dialogue.h"

typedef struct Moteur {
    Fenetre fenetre;
    Rendu rendu;
    Camera camera;
    Timer timer;
    Entree entree;
    PileEtats etats;
    Flags flags;
    int en_cours;
} Moteur;

Moteur creer_moteur(const char *titre, int largeur, int hauteur);
void   ajouter_etat(Moteur *moteur, Etat etat);
void   lancer_moteur(Moteur *moteur);
void   detruire_moteur(Moteur *moteur);

#endif
