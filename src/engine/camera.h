#ifndef CAMERA_H
#define CAMERA_H

typedef struct Camera {
    float x, y;
    int largeur, hauteur;
    int limite_x, limite_y;
} Camera;

Camera creer_camera(int largeur_fenetre, int hauteur_fenetre);
void   centrer_camera(Camera *camera, float cible_x, float cible_y);
void   limiter_camera(Camera *camera, int largeur_carte, int hauteur_carte);
void   fixer_camera(Camera *camera, float x, float y);

#endif
