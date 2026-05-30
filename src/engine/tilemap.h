#ifndef TILEMAP_H
#define TILEMAP_H

#include <SDL2/SDL.h>

struct Camera;
struct Rendu;
struct ListeEntites;

typedef struct FrameAnim { int tile_local; int duree_ms; } FrameAnim;

typedef struct TuileAnimee {
    int tile_local;
    FrameAnim frames[16];
    int nb_frames;
} TuileAnimee;

typedef struct Tileset {
    int firstgid;
    int colonnes, taille_tile;
    int tilecount;
    SDL_Texture *texture;
    int img_largeur, img_hauteur;
    TuileAnimee animations[64];
    int nb_animations;
} Tileset;

typedef enum { PROP_STR, PROP_INT, PROP_FLOAT, PROP_BOOL } TypeProp;
typedef struct Propriete {
    char nom[64];
    TypeProp type;
    char valeur[128];
} Propriete;

typedef struct Objet {
    char nom[64];
    char classe[32];
    float x, y, largeur, hauteur;
    Propriete props[32];
    int nb_props;
} Objet;

typedef enum { ELEM_COUCHE_TUILES, ELEM_GROUPE_OBJETS } TypeElement;

typedef struct CoucheTuiles {
    char nom[64];
    int colonnes, lignes;
    int *gids;
} CoucheTuiles;

typedef struct GroupeObjets {
    char nom[64];
    char classe[32];
    Objet *objets;
    int nb_objets;
} GroupeObjets;

typedef struct ElementCarte {
    TypeElement type;
    CoucheTuiles couche;
    GroupeObjets groupe;
} ElementCarte;

typedef struct RectF { float x, y, largeur, hauteur; } RectF;

typedef struct Carte {
    int colonnes, lignes;
    int taille_tile;
    Tileset tilesets[8];
    int nb_tilesets;
    ElementCarte *elements;
    int nb_elements;
    RectF *collisions;
    int nb_collisions;
} Carte;

Carte   charger_carte(SDL_Renderer *renderer, const char *chemin_tmx);
void    detruire_carte(Carte *carte);

void    afficher_carte(Carte *carte, struct Camera *camera, struct Rendu *rendu);
void    afficher_scene(Carte *carte, struct ListeEntites *entites,
                       struct Camera *camera, struct Rendu *rendu);

Objet      *chercher_objet(Carte *carte, const char *nom);
const char *prop_str(Objet *o, const char *nom, const char *defaut);
int         prop_int(Objet *o, const char *nom, int defaut);
float       prop_float(Objet *o, const char *nom, float defaut);
int         prop_bool(Objet *o, const char *nom, int defaut);

int     carte_collision_point(Carte *carte, float x, float y);
int     carte_collision_rect(Carte *carte, RectF zone);

/* Helpers internes exposés pour les tests (logique pure, sans renderer) */
Tileset *tileset_pour_gid(Carte *carte, int gid);
int      frame_active(TuileAnimee *anim, int temps_ms);

#endif
