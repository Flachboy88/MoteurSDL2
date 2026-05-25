#include "engine/camera.h"

Camera creer_camera(int largeur_fenetre, int hauteur_fenetre) {
    Camera c = {0};
    c.largeur = largeur_fenetre;
    c.hauteur = hauteur_fenetre;
    return c;
}

void centrer_camera(Camera *cam, float cible_x, float cible_y) {
    cam->x = cible_x - cam->largeur / 2.0f;
    cam->y = cible_y - cam->hauteur / 2.0f;
}

void limiter_camera(Camera *cam, int largeur_carte, int hauteur_carte) {
    cam->limite_x = largeur_carte;
    cam->limite_y = hauteur_carte;
    if (cam->x < 0) cam->x = 0;
    if (cam->y < 0) cam->y = 0;
    if (cam->x > largeur_carte - cam->largeur)
        cam->x = (float)(largeur_carte - cam->largeur);
    if (cam->y > hauteur_carte - cam->hauteur)
        cam->y = (float)(hauteur_carte - cam->hauteur);
}

void fixer_camera(Camera *cam, float x, float y) {
    cam->x = x;
    cam->y = y;
}
