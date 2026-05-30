#include "moteur.h"
#include <stdio.h>

static void demo_init(Etat *etat) { (void)etat; }
static void demo_events(Etat *etat, Entree *entree) {
    (void)etat;
    if (action_pressee(entree, ACTION_ANNULER))
        printf("Echap presse !\n");
}
static void demo_update(Etat *etat, float dt) { (void)etat; (void)dt; }
static void demo_render(Etat *etat, Rendu *rendu) {
    (void)etat;
    Couleur bleu = {50, 50, 200, 255};
    dessiner_rect(rendu, 350, 250, 100, 100, bleu);
}
static void demo_destroy(Etat *etat) { (void)etat; }

int main(int argc, char *argv[]) {
    (void)argc; (void)argv;

    Moteur moteur = creer_moteur("MoteurSDL2 — Demo", 800, 600);
    if (!moteur.en_cours) return 1;

    Etat demo = {0};
    demo.initialiser = demo_init;
    demo.gerer_evenements = demo_events;
    demo.mettre_a_jour = demo_update;
    demo.afficher = demo_render;
    demo.detruire = demo_destroy;

    ajouter_etat(&moteur, demo);
    lancer_moteur(&moteur);
    detruire_moteur(&moteur);

    return 0;
}
