#include "moteur.h"

Moteur creer_moteur(const char *titre, int largeur, int hauteur) {
    Moteur m = {0};
    if (!initialiser_sdl()) return m;

    m.fenetre = creer_fenetre(titre, largeur, hauteur);
    if (!m.fenetre.window) return m;

    m.camera = creer_camera(largeur, hauteur);
    m.rendu.renderer = m.fenetre.renderer;
    m.rendu.camera = &m.camera;
    m.timer = creer_timer();
    m.entree = creer_entree();
    m.en_cours = 1;
    return m;
}

void ajouter_etat(Moteur *moteur, Etat etat) {
    empiler_etat(&moteur->etats, etat);
}

void lancer_moteur(Moteur *moteur) {
    while (moteur->en_cours) {
        float dt = calculer_delta_time(&moteur->timer);
        mettre_a_jour_entrees(&moteur->entree);

        if (moteur->entree.quitter) {
            moteur->en_cours = 0;
            break;
        }

        Etat *actuel = obtenir_etat_courant(&moteur->etats);
        if (!actuel) break;

        if (actuel->gerer_evenements)
            actuel->gerer_evenements(actuel, &moteur->entree);
        if (actuel->mettre_a_jour)
            actuel->mettre_a_jour(actuel, dt);

        effacer_ecran(&moteur->rendu);
        if (actuel->afficher)
            actuel->afficher(actuel, &moteur->rendu);
        presenter_rendu(&moteur->rendu);
    }
}

void detruire_moteur(Moteur *moteur) {
    while (moteur->etats.sommet > 0)
        depiler_etat(&moteur->etats);
    detruire_fenetre(&moteur->fenetre);
    quitter_sdl();
}
