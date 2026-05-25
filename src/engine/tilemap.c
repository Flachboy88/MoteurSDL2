#include "engine/tilemap.h"
#include "engine/camera.h"
#include "engine/renderer.h"
#include "engine/entity.h"
#include <SDL2/SDL_image.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static Couche creer_couche(int colonnes, int lignes) {
    Couche c = {0};
    c.colonnes = colonnes;
    c.lignes = lignes;
    c.tuiles = calloc(colonnes * lignes, sizeof(int));
    return c;
}

static void parser_couche_csv(Couche *couche, FILE *fp) {
    char ligne[4096];
    int row = 0;
    while (row < couche->lignes && fgets(ligne, sizeof(ligne), fp)) {
        if (ligne[0] == '#' || ligne[0] == '\n' || ligne[0] == '\r') break;
        char *tok = strtok(ligne, ",\n\r");
        int col = 0;
        while (tok && col < couche->colonnes) {
            couche->tuiles[row * couche->colonnes + col] = atoi(tok);
            tok = strtok(NULL, ",\n\r");
            col++;
        }
        row++;
    }
}

Carte charger_carte(SDL_Renderer *renderer, const char *chemin) {
    Carte carte = {0};
    FILE *fp = fopen(chemin, "r");
    if (!fp) {
        fprintf(stderr, "charger_carte: impossible d'ouvrir %s\n", chemin);
        return carte;
    }

    char ligne[4096];
    char chemin_tileset[256] = {0};

    while (fgets(ligne, sizeof(ligne), fp)) {
        char *nl = strchr(ligne, '\n');
        if (nl) *nl = '\0';
        char *cr = strchr(ligne, '\r');
        if (cr) *cr = '\0';

        if (strncmp(ligne, "colonnes:", 9) == 0)
            carte.colonnes = atoi(ligne + 9);
        else if (strncmp(ligne, "lignes:", 7) == 0)
            carte.lignes = atoi(ligne + 7);
        else if (strncmp(ligne, "taille_tile:", 12) == 0)
            carte.taille_tile = atoi(ligne + 12);
        else if (strncmp(ligne, "tileset:", 8) == 0)
            strncpy(chemin_tileset, ligne + 8, 255);
        else if (strcmp(ligne, "#sol") == 0) {
            carte.sol = creer_couche(carte.colonnes, carte.lignes);
            parser_couche_csv(&carte.sol, fp);
        }
        else if (strcmp(ligne, "#objets_bas") == 0) {
            carte.objets_bas = creer_couche(carte.colonnes, carte.lignes);
            parser_couche_csv(&carte.objets_bas, fp);
        }
        else if (strcmp(ligne, "#objets_haut") == 0) {
            carte.objets_haut = creer_couche(carte.colonnes, carte.lignes);
            parser_couche_csv(&carte.objets_haut, fp);
        }
        else if (strcmp(ligne, "#collision") == 0) {
            carte.collision = creer_couche(carte.colonnes, carte.lignes);
            parser_couche_csv(&carte.collision, fp);
        }
        else if (strcmp(ligne, "#objets_liste") == 0) {
            while (fgets(ligne, sizeof(ligne), fp)) {
                if (ligne[0] == '#' || ligne[0] == '\n' || ligne[0] == '\r') break;
                char *nl2 = strchr(ligne, '\n');
                if (nl2) *nl2 = '\0';

                Objet obj = {0};
                char *tok = strtok(ligne, ",");
                if (tok) strncpy(obj.nom, tok, 63);
                tok = strtok(NULL, ","); if (tok) obj.x = (float)atof(tok);
                tok = strtok(NULL, ","); if (tok) obj.y = (float)atof(tok);
                tok = strtok(NULL, ","); if (tok) obj.largeur = atoi(tok);
                tok = strtok(NULL, ","); if (tok) obj.hauteur = atoi(tok);
                tok = strtok(NULL, ","); if (tok) strncpy(obj.type, tok, 31);

                carte.objets.nb++;
                carte.objets.objets = realloc(carte.objets.objets,
                                               carte.objets.nb * sizeof(Objet));
                carte.objets.objets[carte.objets.nb - 1] = obj;
            }
        }
    }
    fclose(fp);

    if (chemin_tileset[0] && renderer) {
        SDL_Surface *s = IMG_Load(chemin_tileset);
        if (s) {
            carte.tileset = SDL_CreateTextureFromSurface(renderer, s);
            carte.tileset_colonnes = s->w / carte.taille_tile;
            SDL_FreeSurface(s);
        }
    }
    return carte;
}

void detruire_carte(Carte *carte) {
    free(carte->sol.tuiles);
    free(carte->objets_bas.tuiles);
    free(carte->objets_haut.tuiles);
    free(carte->collision.tuiles);
    free(carte->objets.objets);
    if (carte->tileset) SDL_DestroyTexture(carte->tileset);
    memset(carte, 0, sizeof(Carte));
}

void afficher_couche(Couche *couche, Carte *carte,
                      struct Camera *camera, struct Rendu *rendu) {
    if (!couche->tuiles || !carte->tileset) return;
    int ox = camera ? (int)camera->x : 0;
    int oy = camera ? (int)camera->y : 0;
    int ts = carte->taille_tile;

    for (int row = 0; row < couche->lignes; row++) {
        for (int col = 0; col < couche->colonnes; col++) {
            int tile = couche->tuiles[row * couche->colonnes + col];
            if (tile <= 0) continue;
            tile--;
            int src_col = tile % carte->tileset_colonnes;
            int src_row = tile / carte->tileset_colonnes;
            SDL_Rect src = { src_col * ts, src_row * ts, ts, ts };
            SDL_Rect dst = { col * ts - ox, row * ts - oy, ts, ts };
            SDL_RenderCopy(rendu->renderer, carte->tileset, &src, &dst);
        }
    }
}

int case_praticable(Carte *carte, int colonne, int ligne) {
    if (colonne < 0 || colonne >= carte->colonnes) return 0;
    if (ligne < 0 || ligne >= carte->lignes) return 0;
    if (!carte->collision.tuiles) return 1;
    return carte->collision.tuiles[ligne * carte->colonnes + colonne] == 0;
}

Objet *chercher_objet(Carte *carte, const char *nom) {
    for (int i = 0; i < carte->objets.nb; i++) {
        if (strcmp(carte->objets.objets[i].nom, nom) == 0)
            return &carte->objets.objets[i];
    }
    return NULL;
}

void afficher_scene(Carte *carte, struct ListeEntites *entites,
                     struct Camera *camera, struct Rendu *rendu) {
    afficher_couche(&carte->sol, carte, camera, rendu);
    afficher_couche(&carte->objets_bas, carte, camera, rendu);
    if (entites) {
        trier_entites_par_profondeur(entites);
        afficher_toutes_entites(entites, rendu);
    }
    afficher_couche(&carte->objets_haut, carte, camera, rendu);
}
