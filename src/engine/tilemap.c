#include "engine/tilemap.h"
#include "engine/xml.h"
#include "engine/camera.h"
#include "engine/renderer.h"
#include "engine/entity.h"
#include <SDL2/SDL_image.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static SDL_Renderer *g_renderer_chargement = NULL;

/* ---------- utilitaires chemins ---------- */
static void dossier_de(const char *chemin, char *sortie, size_t taille) {
    strncpy(sortie, chemin, taille - 1);
    sortie[taille - 1] = '\0';
    char *slash = strrchr(sortie, '/');
    char *bslash = strrchr(sortie, '\\');
    char *dernier = slash > bslash ? slash : bslash;
    if (dernier) dernier[1] = '\0';
    else sortie[0] = '\0';
}

static void joindre_chemin(const char *base, const char *source, char *sortie, size_t taille) {
    snprintf(sortie, taille, "%s%s", base, source);
}

/* ---------- ajout d'éléments ---------- */
static ElementCarte *ajouter_element(Carte *c) {
    c->nb_elements++;
    c->elements = realloc(c->elements, c->nb_elements * sizeof(ElementCarte));
    ElementCarte *e = &c->elements[c->nb_elements - 1];
    memset(e, 0, sizeof(*e));
    return e;
}

/* ---------- CSV ---------- */
static void parser_csv(const char *texte, int *gids, int total) {
    int i = 0;
    const char *p = texte;
    while (*p && i < total) {
        while (*p && (*p < '0' || *p > '9') && *p != '-') p++;
        if (!*p) break;
        gids[i++] = atoi(p);
        while (*p && ((*p >= '0' && *p <= '9') || *p == '-')) p++;
    }
}

/* ---------- tilesets ---------- */
static void parser_animation(Tileset *t, XmlScanner *s, int tile_local) {
    if (t->nb_animations >= 64) {
        XmlEvenement ev;
        while (xml_suivant(s, &ev))
            if (ev.type == XML_FIN_ELEMENT && strcmp(ev.nom, "animation") == 0) break;
        return;
    }
    TuileAnimee *a = &t->animations[t->nb_animations];
    a->tile_local = tile_local;
    a->nb_frames = 0;
    XmlEvenement ev;
    while (xml_suivant(s, &ev)) {
        if (ev.type == XML_DEBUT_ELEMENT && strcmp(ev.nom, "frame") == 0) {
            if (a->nb_frames < 16) {
                const char *tid = xml_attr(&ev, "tileid");
                const char *dur = xml_attr(&ev, "duration");
                a->frames[a->nb_frames].tile_local = tid ? atoi(tid) : 0;
                a->frames[a->nb_frames].duree_ms = dur ? atoi(dur) : 100;
                a->nb_frames++;
            }
        } else if (ev.type == XML_FIN_ELEMENT && strcmp(ev.nom, "animation") == 0) {
            break;
        }
    }
    if (a->nb_frames > 0) t->nb_animations++;
}

static void parser_corps_tileset(Tileset *t, XmlScanner *s, XmlEvenement *ouvrant,
                                 const char *dossier, const char *balise_fin) {
    const char *tw = xml_attr(ouvrant, "tilewidth");
    const char *cols = xml_attr(ouvrant, "columns");
    const char *cnt = xml_attr(ouvrant, "tilecount");
    if (tw) t->taille_tile = atoi(tw);
    if (cols) t->colonnes = atoi(cols);
    if (cnt) t->tilecount = atoi(cnt);
    if (ouvrant->self_closing) return;

    int tile_courant = 0;
    XmlEvenement ev;
    while (xml_suivant(s, &ev)) {
        if (ev.type == XML_DEBUT_ELEMENT && strcmp(ev.nom, "image") == 0) {
            const char *src = xml_attr(&ev, "source");
            const char *w = xml_attr(&ev, "width");
            const char *h = xml_attr(&ev, "height");
            if (w) t->img_largeur = atoi(w);
            if (h) t->img_hauteur = atoi(h);
            if (src && g_renderer_chargement) {
                char chemin[512];
                joindre_chemin(dossier, src, chemin, sizeof(chemin));
                SDL_Surface *surf = IMG_Load(chemin);
                if (surf) {
                    t->texture = SDL_CreateTextureFromSurface(g_renderer_chargement, surf);
                    if (!t->img_largeur) t->img_largeur = surf->w;
                    if (!t->img_hauteur) t->img_hauteur = surf->h;
                    SDL_FreeSurface(surf);
                } else {
                    fprintf(stderr, "tileset: image introuvable %s\n", chemin);
                }
            }
        } else if (ev.type == XML_DEBUT_ELEMENT && strcmp(ev.nom, "tile") == 0) {
            const char *id = xml_attr(&ev, "id");
            tile_courant = id ? atoi(id) : 0;
        } else if (ev.type == XML_DEBUT_ELEMENT && strcmp(ev.nom, "animation") == 0) {
            parser_animation(t, s, tile_courant);
        } else if (ev.type == XML_FIN_ELEMENT && strcmp(ev.nom, balise_fin) == 0) {
            break;
        }
    }
    if (t->colonnes == 0 && t->taille_tile > 0 && t->img_largeur > 0)
        t->colonnes = t->img_largeur / t->taille_tile;
}

static void parser_tileset(Carte *c, XmlScanner *s, XmlEvenement *ev, const char *dossier_tmx) {
    if (c->nb_tilesets >= 8) {
        if (!ev->self_closing) {
            XmlEvenement sous;
            while (xml_suivant(s, &sous))
                if (sous.type == XML_FIN_ELEMENT && strcmp(sous.nom, "tileset") == 0) break;
        }
        return;
    }
    Tileset *t = &c->tilesets[c->nb_tilesets];
    memset(t, 0, sizeof(*t));
    const char *fg = xml_attr(ev, "firstgid");
    t->firstgid = fg ? atoi(fg) : 1;

    const char *source = xml_attr(ev, "source");
    if (source) {
        char chemin_tsx[512];
        joindre_chemin(dossier_tmx, source, chemin_tsx, sizeof(chemin_tsx));
        char dossier_tsx[256];
        dossier_de(chemin_tsx, dossier_tsx, sizeof(dossier_tsx));
        XmlScanner *st = xml_ouvrir(chemin_tsx);
        if (st) {
            XmlEvenement evt;
            while (xml_suivant(st, &evt)) {
                if (evt.type == XML_DEBUT_ELEMENT && strcmp(evt.nom, "tileset") == 0) {
                    parser_corps_tileset(t, st, &evt, dossier_tsx, "tileset");
                    break;
                }
            }
            xml_fermer(st);
        } else {
            fprintf(stderr, "tileset externe introuvable: %s\n", chemin_tsx);
        }
        if (!ev->self_closing) {
            XmlEvenement sous;
            while (xml_suivant(s, &sous))
                if (sous.type == XML_FIN_ELEMENT && strcmp(sous.nom, "tileset") == 0) break;
        }
    } else {
        parser_corps_tileset(t, s, ev, dossier_tmx, "tileset");
    }
    c->nb_tilesets++;
}

/* ---------- couches ---------- */
static void parser_layer(Carte *c, XmlScanner *s, XmlEvenement *ev) {
    ElementCarte *e = ajouter_element(c);
    e->type = ELEM_COUCHE_TUILES;
    const char *nom = xml_attr(ev, "name");
    const char *w = xml_attr(ev, "width");
    const char *h = xml_attr(ev, "height");
    if (nom) strncpy(e->couche.nom, nom, sizeof(e->couche.nom) - 1);
    e->couche.colonnes = w ? atoi(w) : c->colonnes;
    e->couche.lignes   = h ? atoi(h) : c->lignes;
    int total = e->couche.colonnes * e->couche.lignes;
    e->couche.gids = calloc(total > 0 ? total : 1, sizeof(int));

    XmlEvenement sous;
    while (xml_suivant(s, &sous)) {
        if (sous.type == XML_TEXTE) {
            parser_csv(sous.texte, e->couche.gids, total);
        } else if (sous.type == XML_FIN_ELEMENT && strcmp(sous.nom, "layer") == 0) {
            break;
        }
    }
}

/* ---------- objets ---------- */
static void parser_properties(Objet *o, XmlScanner *s) {
    XmlEvenement ev;
    while (xml_suivant(s, &ev)) {
        if (ev.type == XML_DEBUT_ELEMENT && strcmp(ev.nom, "property") == 0) {
            if (o->nb_props < 32) {
                Propriete *p = &o->props[o->nb_props];
                const char *nom = xml_attr(&ev, "name");
                const char *type = xml_attr(&ev, "type");
                const char *val = xml_attr(&ev, "value");
                memset(p, 0, sizeof(*p));
                if (nom) strncpy(p->nom, nom, sizeof(p->nom) - 1);
                if (val) strncpy(p->valeur, val, sizeof(p->valeur) - 1);
                if (!type)                           p->type = PROP_STR;
                else if (strcmp(type, "int") == 0)   p->type = PROP_INT;
                else if (strcmp(type, "float") == 0) p->type = PROP_FLOAT;
                else if (strcmp(type, "bool") == 0)  p->type = PROP_BOOL;
                else                                 p->type = PROP_STR;
                o->nb_props++;
            }
        } else if (ev.type == XML_FIN_ELEMENT && strcmp(ev.nom, "properties") == 0) {
            break;
        }
    }
}

static void parser_object(GroupeObjets *g, XmlScanner *s, XmlEvenement *ouvrant) {
    g->nb_objets++;
    g->objets = realloc(g->objets, g->nb_objets * sizeof(Objet));
    Objet *o = &g->objets[g->nb_objets - 1];
    memset(o, 0, sizeof(*o));
    const char *nom = xml_attr(ouvrant, "name");
    const char *type = xml_attr(ouvrant, "type");
    const char *cls = xml_attr(ouvrant, "class");
    const char *x = xml_attr(ouvrant, "x");
    const char *y = xml_attr(ouvrant, "y");
    const char *w = xml_attr(ouvrant, "width");
    const char *h = xml_attr(ouvrant, "height");
    if (nom) strncpy(o->nom, nom, sizeof(o->nom) - 1);
    if (type) strncpy(o->classe, type, sizeof(o->classe) - 1);
    else if (cls) strncpy(o->classe, cls, sizeof(o->classe) - 1);
    if (x) o->x = (float)atof(x);
    if (y) o->y = (float)atof(y);
    if (w) o->largeur = (float)atof(w);
    if (h) o->hauteur = (float)atof(h);

    if (ouvrant->self_closing) return;
    XmlEvenement ev;
    while (xml_suivant(s, &ev)) {
        if (ev.type == XML_DEBUT_ELEMENT && strcmp(ev.nom, "properties") == 0) {
            parser_properties(o, s);
        } else if (ev.type == XML_FIN_ELEMENT && strcmp(ev.nom, "object") == 0) {
            break;
        }
    }
}

static void parser_objectgroup(Carte *c, XmlScanner *s, XmlEvenement *ev) {
    ElementCarte *e = ajouter_element(c);
    e->type = ELEM_GROUPE_OBJETS;
    const char *nom = xml_attr(ev, "name");
    const char *cls = xml_attr(ev, "class");
    if (nom) strncpy(e->groupe.nom, nom, sizeof(e->groupe.nom) - 1);
    if (cls) strncpy(e->groupe.classe, cls, sizeof(e->groupe.classe) - 1);
    if (ev->self_closing) return;

    XmlEvenement sous;
    while (xml_suivant(s, &sous)) {
        if (sous.type == XML_DEBUT_ELEMENT && strcmp(sous.nom, "object") == 0) {
            parser_object(&e->groupe, s, &sous);
        } else if (sous.type == XML_FIN_ELEMENT && strcmp(sous.nom, "objectgroup") == 0) {
            break;
        }
    }
}

/* ---------- chargement ---------- */
Carte charger_carte(SDL_Renderer *renderer, const char *chemin_tmx) {
    Carte c = {0};
    XmlScanner *s = xml_ouvrir(chemin_tmx);
    if (!s) {
        fprintf(stderr, "charger_carte: impossible d'ouvrir %s\n", chemin_tmx);
        return c;
    }

    char dossier_tmx[256];
    dossier_de(chemin_tmx, dossier_tmx, sizeof(dossier_tmx));
    g_renderer_chargement = renderer;

    XmlEvenement ev;
    while (xml_suivant(s, &ev)) {
        if (ev.type != XML_DEBUT_ELEMENT) continue;
        if (strcmp(ev.nom, "map") == 0) {
            const char *w = xml_attr(&ev, "width");
            const char *h = xml_attr(&ev, "height");
            const char *tw = xml_attr(&ev, "tilewidth");
            if (w) c.colonnes = atoi(w);
            if (h) c.lignes = atoi(h);
            if (tw) c.taille_tile = atoi(tw);
        } else if (strcmp(ev.nom, "tileset") == 0) {
            parser_tileset(&c, s, &ev, dossier_tmx);
        } else if (strcmp(ev.nom, "layer") == 0) {
            parser_layer(&c, s, &ev);
        } else if (strcmp(ev.nom, "objectgroup") == 0) {
            parser_objectgroup(&c, s, &ev);
        }
    }
    xml_fermer(s);

    for (int i = 0; i < c.nb_elements; i++) {
        if (c.elements[i].type != ELEM_GROUPE_OBJETS) continue;
        GroupeObjets *g = &c.elements[i].groupe;
        for (int j = 0; j < g->nb_objets; j++) {
            if (strcmp(g->objets[j].classe, "Collision") != 0) continue;
            c.nb_collisions++;
            c.collisions = realloc(c.collisions, c.nb_collisions * sizeof(RectF));
            RectF *r = &c.collisions[c.nb_collisions - 1];
            r->x = g->objets[j].x;
            r->y = g->objets[j].y;
            r->largeur = g->objets[j].largeur;
            r->hauteur = g->objets[j].hauteur;
        }
    }

    g_renderer_chargement = NULL;
    return c;
}

void detruire_carte(Carte *carte) {
    for (int i = 0; i < carte->nb_elements; i++) {
        if (carte->elements[i].type == ELEM_COUCHE_TUILES)
            free(carte->elements[i].couche.gids);
        else
            free(carte->elements[i].groupe.objets);
    }
    free(carte->elements);
    free(carte->collisions);
    for (int i = 0; i < carte->nb_tilesets; i++)
        if (carte->tilesets[i].texture) SDL_DestroyTexture(carte->tilesets[i].texture);
    memset(carte, 0, sizeof(Carte));
}

/* ---------- objets : recherche + accesseurs ---------- */
static Propriete *trouver_prop(Objet *o, const char *nom) {
    if (!o) return NULL;
    for (int i = 0; i < o->nb_props; i++)
        if (strcmp(o->props[i].nom, nom) == 0) return &o->props[i];
    return NULL;
}

Objet *chercher_objet(Carte *carte, const char *nom) {
    for (int i = 0; i < carte->nb_elements; i++) {
        if (carte->elements[i].type != ELEM_GROUPE_OBJETS) continue;
        GroupeObjets *g = &carte->elements[i].groupe;
        for (int j = 0; j < g->nb_objets; j++)
            if (strcmp(g->objets[j].nom, nom) == 0) return &g->objets[j];
    }
    return NULL;
}

const char *prop_str(Objet *o, const char *nom, const char *defaut) {
    Propriete *p = trouver_prop(o, nom);
    return p ? p->valeur : defaut;
}
int prop_int(Objet *o, const char *nom, int defaut) {
    Propriete *p = trouver_prop(o, nom);
    return p ? atoi(p->valeur) : defaut;
}
float prop_float(Objet *o, const char *nom, float defaut) {
    Propriete *p = trouver_prop(o, nom);
    return p ? (float)atof(p->valeur) : defaut;
}
int prop_bool(Objet *o, const char *nom, int defaut) {
    Propriete *p = trouver_prop(o, nom);
    if (!p) return defaut;
    return (strcmp(p->valeur, "true") == 0 || strcmp(p->valeur, "1") == 0) ? 1 : 0;
}

/* ---------- collision ---------- */
static int rect_chevauche(RectF a, RectF b) {
    return a.x < b.x + b.largeur && a.x + a.largeur > b.x &&
           a.y < b.y + b.hauteur && a.y + a.hauteur > b.y;
}

int carte_collision_rect(Carte *carte, RectF zone) {
    for (int i = 0; i < carte->nb_collisions; i++)
        if (rect_chevauche(zone, carte->collisions[i])) return 1;
    return 0;
}

int carte_collision_point(Carte *carte, float x, float y) {
    for (int i = 0; i < carte->nb_collisions; i++) {
        RectF r = carte->collisions[i];
        if (x >= r.x && x < r.x + r.largeur && y >= r.y && y < r.y + r.hauteur)
            return 1;
    }
    return 0;
}

/* ---------- résolution gid + animation ---------- */
Tileset *tileset_pour_gid(Carte *carte, int gid) {
    if (gid <= 0) return NULL;
    Tileset *meilleur = NULL;
    for (int i = 0; i < carte->nb_tilesets; i++) {
        if (carte->tilesets[i].firstgid <= gid) {
            if (!meilleur || carte->tilesets[i].firstgid > meilleur->firstgid)
                meilleur = &carte->tilesets[i];
        }
    }
    return meilleur;
}

int frame_active(TuileAnimee *anim, int temps_ms) {
    if (anim->nb_frames == 0) return anim->tile_local;
    int cycle = 0;
    for (int i = 0; i < anim->nb_frames; i++) cycle += anim->frames[i].duree_ms;
    if (cycle <= 0) return anim->frames[0].tile_local;
    int t = temps_ms % cycle;
    int acc = 0;
    for (int i = 0; i < anim->nb_frames; i++) {
        acc += anim->frames[i].duree_ms;
        if (t < acc) return anim->frames[i].tile_local;
    }
    return anim->frames[anim->nb_frames - 1].tile_local;
}

/* ---------- rendu ---------- */
static int gid_local_anime(Tileset *t, int local, int temps_ms) {
    for (int i = 0; i < t->nb_animations; i++)
        if (t->animations[i].tile_local == local)
            return frame_active(&t->animations[i], temps_ms);
    return local;
}

void afficher_carte(Carte *carte, struct Camera *camera, struct Rendu *rendu) {
    afficher_scene(carte, NULL, camera, rendu);
}

void afficher_scene(Carte *carte, struct ListeEntites *entites,
                    struct Camera *camera, struct Rendu *rendu) {
    int ox = camera ? (int)camera->x : 0;
    int oy = camera ? (int)camera->y : 0;
    int ts = carte->taille_tile;
    int temps_ms = (int)SDL_GetTicks();
    int premier_groupe_fait = 0;

    for (int e = 0; e < carte->nb_elements; e++) {
        ElementCarte *el = &carte->elements[e];
        if (el->type == ELEM_COUCHE_TUILES) {
            CoucheTuiles *cc = &el->couche;
            for (int row = 0; row < cc->lignes; row++) {
                for (int col = 0; col < cc->colonnes; col++) {
                    int gid = cc->gids[row * cc->colonnes + col];
                    if (gid <= 0) continue;
                    Tileset *t = tileset_pour_gid(carte, gid);
                    if (!t || !t->texture || t->colonnes <= 0) continue;
                    int local = gid - t->firstgid;
                    local = gid_local_anime(t, local, temps_ms);
                    int src_col = local % t->colonnes;
                    int src_row = local / t->colonnes;
                    SDL_Rect src = { src_col * t->taille_tile, src_row * t->taille_tile,
                                     t->taille_tile, t->taille_tile };
                    SDL_Rect dst = { col * ts - ox, row * ts - oy, ts, ts };
                    SDL_RenderCopy(rendu->renderer, t->texture, &src, &dst);
                }
            }
        } else if (el->type == ELEM_GROUPE_OBJETS) {
            if (!premier_groupe_fait && entites) {
                trier_entites_par_profondeur(entites);
                afficher_toutes_entites(entites, rendu);
                premier_groupe_fait = 1;
            }
        }
    }
}
