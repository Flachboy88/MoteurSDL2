# Tilemap TMX Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Remplacer le format de carte `.map` maison par un chargeur du format TMX de Tiled (couches arbitraires, tilesets multiples inline + `.tsx`, tuiles animées, objets bruts, collision AABB float).

**Architecture:** Un module `xml` (scanner SAX minimal, sans SDL, réutilisable) consommé par un module `tilemap` réécrit. `tilemap` construit une `Carte` (liste ordonnée d'éléments = ordre de rendu), résout les gid vers les tilesets, sélectionne les frames d'animation via une horloge globale, et teste la collision par rectangles flottants. Le rendu non testable unitairement est validé via la démo.

**Tech Stack:** C11, MinGW GCC 32-bit, SDL2 + SDL2_image, `ar` static lib, framework de test maison (`tests/test_framework.h`).

**Note de conception (déviation du spec) :** `entity.h` déclare déjà `int collision_rect(Entite*, Entite*)`. Pour éviter le conflit de symbole dans `libmoteur.a`, les fonctions de collision de la carte sont nommées **`carte_collision_rect`** et **`carte_collision_point`** (pas `collision_rect`/`collision_point`).

---

## Structure des fichiers

- Create: `src/engine/xml.h` — API du scanner SAX.
- Create: `src/engine/xml.c` — implémentation du scanner.
- Modify (réécriture complète): `src/engine/tilemap.h` — nouveau modèle de données + API.
- Modify (réécriture complète): `src/engine/tilemap.c` — chargement TMX, rendu, animation, collision.
- Create: `tests/test_xml.c` — tests du scanner (sans SDL).
- Create: `tests/test_tilemap.c` — tests de parsing/collision/animation (renderer NULL).
- Modify: `tests/main_tests.c` — inclure les nouveaux tests + appeler `suite_xml()` / `suite_tilemap()`.
- Modify: `Makefile` — ajouter `src/engine/xml.c` à `ENGINE_SRC` (préserver les ajouts SDL2_mixer existants).
- Modify: `main.c` — démo charge `resources/maps/map3.tmx` (validation visuelle).

---

## Task 1: Module xml (scanner SAX)

**Files:**
- Create: `src/engine/xml.h`
- Create: `src/engine/xml.c`
- Test: `tests/test_xml.c`

- [ ] **Step 1: Écrire le header `src/engine/xml.h`**

```c
#ifndef XML_H
#define XML_H

#define XML_MAX_ATTRS 32

typedef struct XmlAttr {
    char nom[64];
    char valeur[256];
} XmlAttr;

typedef enum {
    XML_DEBUT_ELEMENT,
    XML_FIN_ELEMENT,
    XML_TEXTE,
    XML_FIN
} XmlTypeEvenement;

typedef struct XmlEvenement {
    XmlTypeEvenement type;
    char nom[64];
    XmlAttr attrs[XML_MAX_ATTRS];
    int nb_attrs;
    const char *texte;
    int self_closing;
} XmlEvenement;

typedef struct XmlScanner {
    char *buffer;
    long taille;
    long pos;
    int  fin_en_attente;
    char nom_en_attente[64];
    char *restaurer_ptr;
    char restaurer_char;
} XmlScanner;

XmlScanner *xml_ouvrir(const char *chemin);
XmlScanner *xml_ouvrir_chaine(const char *contenu);
int         xml_suivant(XmlScanner *s, XmlEvenement *ev);
const char *xml_attr(XmlEvenement *ev, const char *nom);
void        xml_fermer(XmlScanner *s);

#endif
```

- [ ] **Step 2: Écrire le test qui échoue `tests/test_xml.c`**

```c
#include "../src/engine/xml.h"
#include <stdlib.h>

static int test_xml_sequence_simple(void) {
    XmlScanner *s = xml_ouvrir_chaine("<a><b>texte</b></a>");
    XmlEvenement ev;

    ASSERT_VRAI(xml_suivant(s, &ev)); ASSERT_EGAL(XML_DEBUT_ELEMENT, ev.type); ASSERT_EGAL_STR("a", ev.nom);
    ASSERT_VRAI(xml_suivant(s, &ev)); ASSERT_EGAL(XML_DEBUT_ELEMENT, ev.type); ASSERT_EGAL_STR("b", ev.nom);
    ASSERT_VRAI(xml_suivant(s, &ev)); ASSERT_EGAL(XML_TEXTE, ev.type); ASSERT_EGAL_STR("texte", ev.texte);
    ASSERT_VRAI(xml_suivant(s, &ev)); ASSERT_EGAL(XML_FIN_ELEMENT, ev.type); ASSERT_EGAL_STR("b", ev.nom);
    ASSERT_VRAI(xml_suivant(s, &ev)); ASSERT_EGAL(XML_FIN_ELEMENT, ev.type); ASSERT_EGAL_STR("a", ev.nom);
    ASSERT_FAUX(xml_suivant(s, &ev)); ASSERT_EGAL(XML_FIN, ev.type);

    xml_fermer(s);
    return 1;
}

static int test_xml_self_closing(void) {
    XmlScanner *s = xml_ouvrir_chaine("<x a=\"1\"/>");
    XmlEvenement ev;

    ASSERT_VRAI(xml_suivant(s, &ev));
    ASSERT_EGAL(XML_DEBUT_ELEMENT, ev.type);
    ASSERT_EGAL_STR("x", ev.nom);
    ASSERT_EGAL(1, ev.self_closing);
    ASSERT_VRAI(xml_suivant(s, &ev));
    ASSERT_EGAL(XML_FIN_ELEMENT, ev.type);
    ASSERT_EGAL_STR("x", ev.nom);

    xml_fermer(s);
    return 1;
}

static int test_xml_attributs(void) {
    XmlScanner *s = xml_ouvrir_chaine("<t name=\"Walls and Floors\" cols=\"8\"/>");
    XmlEvenement ev;
    ASSERT_VRAI(xml_suivant(s, &ev));
    ASSERT_EGAL_STR("Walls and Floors", xml_attr(&ev, "name"));
    ASSERT_EGAL_STR("8", xml_attr(&ev, "cols"));
    ASSERT_NULL(xml_attr(&ev, "absent"));
    xml_fermer(s);
    return 1;
}

static int test_xml_ignore_prologue_commentaire(void) {
    XmlScanner *s = xml_ouvrir_chaine("<?xml version=\"1.0\"?><!-- hi --><r/>");
    XmlEvenement ev;
    ASSERT_VRAI(xml_suivant(s, &ev));
    ASSERT_EGAL(XML_DEBUT_ELEMENT, ev.type);
    ASSERT_EGAL_STR("r", ev.nom);
    xml_fermer(s);
    return 1;
}

static int test_xml_decode_entites(void) {
    XmlScanner *s = xml_ouvrir_chaine("<v t=\"a&amp;b&lt;c&gt;d\"/>");
    XmlEvenement ev;
    ASSERT_VRAI(xml_suivant(s, &ev));
    ASSERT_EGAL_STR("a&b<c>d", xml_attr(&ev, "t"));
    xml_fermer(s);
    return 1;
}

static void suite_xml(void) {
    LANCER_TEST(test_xml_sequence_simple);
    LANCER_TEST(test_xml_self_closing);
    LANCER_TEST(test_xml_attributs);
    LANCER_TEST(test_xml_ignore_prologue_commentaire);
    LANCER_TEST(test_xml_decode_entites);
}
```

- [ ] **Step 3: Vérifier l'échec de compilation/édition de lien**

Run: ajouter temporairement `#include "test_xml.c"` + `suite_xml();` dans `main_tests.c` (sera fait en Task 8), ou compiler `tests/test_xml.c` seul. Attendu : lien échoue (`xml_ouvrir_chaine` undefined). Si tu suis l'ordre du plan, passe à Step 4 et la vérification réelle a lieu à Task 8.

- [ ] **Step 4: Écrire l'implémentation `src/engine/xml.c`**

```c
#include "engine/xml.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

static char *lire_fichier(const char *chemin, long *taille_out) {
    FILE *fp = fopen(chemin, "rb");
    if (!fp) return NULL;
    fseek(fp, 0, SEEK_END);
    long taille = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    if (taille < 0) { fclose(fp); return NULL; }
    char *buf = malloc(taille + 1);
    if (!buf) { fclose(fp); return NULL; }
    long lus = (long)fread(buf, 1, (size_t)taille, fp);
    buf[lus] = '\0';
    fclose(fp);
    if (taille_out) *taille_out = lus;
    return buf;
}

static void decoder_entites(char *s) {
    char *r = s, *w = s;
    while (*r) {
        if (*r == '&') {
            if      (strncmp(r, "&amp;",  5) == 0) { *w++ = '&';  r += 5; }
            else if (strncmp(r, "&lt;",   4) == 0) { *w++ = '<';  r += 4; }
            else if (strncmp(r, "&gt;",   4) == 0) { *w++ = '>';  r += 4; }
            else if (strncmp(r, "&quot;", 6) == 0) { *w++ = '"';  r += 6; }
            else if (strncmp(r, "&apos;", 6) == 0) { *w++ = '\''; r += 6; }
            else { *w++ = *r++; }
        } else {
            *w++ = *r++;
        }
    }
    *w = '\0';
}

XmlScanner *xml_ouvrir(const char *chemin) {
    long taille = 0;
    char *buf = lire_fichier(chemin, &taille);
    if (!buf) return NULL;
    XmlScanner *s = calloc(1, sizeof(XmlScanner));
    s->buffer = buf;
    s->taille = taille;
    return s;
}

XmlScanner *xml_ouvrir_chaine(const char *contenu) {
    XmlScanner *s = calloc(1, sizeof(XmlScanner));
    s->taille = (long)strlen(contenu);
    s->buffer = malloc(s->taille + 1);
    memcpy(s->buffer, contenu, (size_t)s->taille + 1);
    return s;
}

void xml_fermer(XmlScanner *s) {
    if (!s) return;
    free(s->buffer);
    free(s);
}

const char *xml_attr(XmlEvenement *ev, const char *nom) {
    for (int i = 0; i < ev->nb_attrs; i++)
        if (strcmp(ev->attrs[i].nom, nom) == 0) return ev->attrs[i].valeur;
    return NULL;
}

int xml_suivant(XmlScanner *s, XmlEvenement *ev) {
    memset(ev, 0, sizeof(*ev));

    if (s->restaurer_ptr) {
        *s->restaurer_ptr = s->restaurer_char;
        s->restaurer_ptr = NULL;
    }

    if (s->fin_en_attente) {
        s->fin_en_attente = 0;
        ev->type = XML_FIN_ELEMENT;
        strncpy(ev->nom, s->nom_en_attente, sizeof(ev->nom) - 1);
        return 1;
    }

    char *b = s->buffer;
    long n = s->taille;

    if (s->pos < n && b[s->pos] != '<') {
        long debut = s->pos;
        while (s->pos < n && b[s->pos] != '<') s->pos++;
        int non_blanc = 0;
        for (long i = debut; i < s->pos; i++)
            if (!isspace((unsigned char)b[i])) { non_blanc = 1; break; }
        if (non_blanc) {
            s->restaurer_ptr = &b[s->pos];
            s->restaurer_char = b[s->pos];
            b[s->pos] = '\0';
            decoder_entites(&b[debut]);
            ev->type = XML_TEXTE;
            ev->texte = &b[debut];
            return 1;
        }
    }

    while (s->pos < n && b[s->pos] != '<') s->pos++;
    if (s->pos >= n) { ev->type = XML_FIN; return 0; }

    if (strncmp(&b[s->pos], "<!--", 4) == 0) {
        char *fin = strstr(&b[s->pos], "-->");
        if (!fin) { ev->type = XML_FIN; return 0; }
        s->pos = (fin - b) + 3;
        return xml_suivant(s, ev);
    }
    if (b[s->pos + 1] == '?') {
        char *fin = strstr(&b[s->pos], "?>");
        if (!fin) { ev->type = XML_FIN; return 0; }
        s->pos = (fin - b) + 2;
        return xml_suivant(s, ev);
    }
    if (b[s->pos + 1] == '/') {
        s->pos += 2;
        long debut = s->pos;
        while (s->pos < n && b[s->pos] != '>') s->pos++;
        long len = s->pos - debut;
        while (len > 0 && isspace((unsigned char)b[debut + len - 1])) len--;
        if (len > 63) len = 63;
        memcpy(ev->nom, &b[debut], (size_t)len);
        ev->nom[len] = '\0';
        if (s->pos < n) s->pos++;
        ev->type = XML_FIN_ELEMENT;
        return 1;
    }

    s->pos++; /* passer '<' */
    long debut = s->pos;
    while (s->pos < n && b[s->pos] != '>' && b[s->pos] != '/' &&
           !isspace((unsigned char)b[s->pos]))
        s->pos++;
    long len = s->pos - debut;
    if (len > 63) len = 63;
    memcpy(ev->nom, &b[debut], (size_t)len);
    ev->nom[len] = '\0';
    ev->type = XML_DEBUT_ELEMENT;

    while (s->pos < n) {
        while (s->pos < n && isspace((unsigned char)b[s->pos])) s->pos++;
        if (s->pos >= n) break;
        if (b[s->pos] == '/') {
            ev->self_closing = 1;
            s->fin_en_attente = 1;
            strncpy(s->nom_en_attente, ev->nom, sizeof(s->nom_en_attente) - 1);
            while (s->pos < n && b[s->pos] != '>') s->pos++;
            if (s->pos < n) s->pos++;
            return 1;
        }
        if (b[s->pos] == '>') { s->pos++; return 1; }

        long ndeb = s->pos;
        while (s->pos < n && b[s->pos] != '=' && b[s->pos] != '>' &&
               !isspace((unsigned char)b[s->pos]))
            s->pos++;
        long nlen = s->pos - ndeb;
        while (s->pos < n && (isspace((unsigned char)b[s->pos]) || b[s->pos] == '=')) s->pos++;
        if (s->pos >= n || b[s->pos] != '"') continue;
        s->pos++;
        long vdeb = s->pos;
        while (s->pos < n && b[s->pos] != '"') s->pos++;
        long vlen = s->pos - vdeb;
        if (s->pos < n) s->pos++;
        if (ev->nb_attrs < XML_MAX_ATTRS) {
            XmlAttr *a = &ev->attrs[ev->nb_attrs++];
            long cn = nlen < 63 ? nlen : 63;
            memcpy(a->nom, &b[ndeb], (size_t)cn); a->nom[cn] = '\0';
            long cv = vlen < 255 ? vlen : 255;
            memcpy(a->valeur, &b[vdeb], (size_t)cv); a->valeur[cv] = '\0';
            decoder_entites(a->valeur);
        }
    }
    return 1;
}
```

- [ ] **Step 5: Vérifier que les tests xml passent (après intégration Task 8)**

Run: `make test`
Expected: les 5 tests `test_xml_*` PASS.

- [ ] **Step 6: Commit**

```bash
git add src/engine/xml.h src/engine/xml.c tests/test_xml.c
git commit -m "feat: module xml — scanner SAX minimal + tests"
```

---

## Task 2: Modèle de données tilemap + chargement couches CSV

**Files:**
- Modify (réécriture): `src/engine/tilemap.h`
- Modify (réécriture): `src/engine/tilemap.c`
- Test: `tests/test_tilemap.c`

- [ ] **Step 1: Réécrire `src/engine/tilemap.h`**

```c
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

#endif
```

- [ ] **Step 2: Écrire le test qui échoue `tests/test_tilemap.c` (couche CSV)**

```c
#include "../src/engine/tilemap.h"
#include <stdio.h>
#include <stdlib.h>

static void ecrire_fichier(const char *chemin, const char *contenu) {
    FILE *fp = fopen(chemin, "wb");
    fputs(contenu, fp);
    fclose(fp);
}

static int test_tilemap_couche_csv(void) {
    const char *tmx =
        "<?xml version=\"1.0\"?>\n"
        "<map orientation=\"orthogonal\" width=\"3\" height=\"2\" tilewidth=\"16\" tileheight=\"16\">\n"
        " <layer id=\"1\" name=\"Sol\" width=\"3\" height=\"2\">\n"
        "  <data encoding=\"csv\">\n"
        "1,2,3,\n"
        "4,5,0\n"
        "</data>\n"
        " </layer>\n"
        "</map>\n";
    ecrire_fichier("bin/t_couche.tmx", tmx);

    Carte c = charger_carte(NULL, "bin/t_couche.tmx");
    ASSERT_EGAL(3, c.colonnes);
    ASSERT_EGAL(2, c.lignes);
    ASSERT_EGAL(16, c.taille_tile);
    ASSERT_EGAL(1, c.nb_elements);
    ASSERT_EGAL(ELEM_COUCHE_TUILES, c.elements[0].type);
    ASSERT_EGAL_STR("Sol", c.elements[0].couche.nom);

    int *g = c.elements[0].couche.gids;
    ASSERT_EGAL(1, g[0]); ASSERT_EGAL(2, g[1]); ASSERT_EGAL(3, g[2]);
    ASSERT_EGAL(4, g[3]); ASSERT_EGAL(5, g[4]); ASSERT_EGAL(0, g[5]);

    detruire_carte(&c);
    remove("bin/t_couche.tmx");
    return 1;
}

static void suite_tilemap(void) {
    LANCER_TEST(test_tilemap_couche_csv);
}
```

- [ ] **Step 3: Écrire `src/engine/tilemap.c` (chargement + couches + détruire + stubs)**

Implémentation initiale couvrant : lecture du `<map>`, dispatch `<layer>`/`<tileset>`/`<objectgroup>`, parsing CSV, `detruire_carte`. Les helpers tileset/objet/collision/animation/rendu sont remplis dans les tasks suivantes ; ici on stub ceux non encore testés mais on écrit déjà le squelette de dispatch.

```c
#include "engine/tilemap.h"
#include "engine/xml.h"
#include "engine/camera.h"
#include "engine/renderer.h"
#include "engine/entity.h"
#include <SDL2/SDL_image.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

/* déclarations des parsers remplis dans les tasks suivantes */
static void parser_tileset(Carte *c, XmlScanner *s, XmlEvenement *ev, const char *dossier_tmx);
static void parser_objectgroup(Carte *c, XmlScanner *s, XmlEvenement *ev);

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

Carte charger_carte(SDL_Renderer *renderer, const char *chemin_tmx) {
    (void)renderer;
    Carte c = {0};
    XmlScanner *s = xml_ouvrir(chemin_tmx);
    if (!s) {
        fprintf(stderr, "charger_carte: impossible d'ouvrir %s\n", chemin_tmx);
        return c;
    }

    char dossier_tmx[256];
    dossier_de(chemin_tmx, dossier_tmx, sizeof(dossier_tmx));
    (void)renderer;

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

    /* renderer + texture + collisions agrégées : remplis dans les tasks suivantes */
    return c;
}

/* stubs temporaires (remplacés en Task 3 et 4) */
static void parser_tileset(Carte *c, XmlScanner *s, XmlEvenement *ev, const char *dossier_tmx) {
    (void)c; (void)dossier_tmx;
    if (ev->self_closing) return;
    XmlEvenement sous;
    while (xml_suivant(s, &sous))
        if (sous.type == XML_FIN_ELEMENT && strcmp(sous.nom, "tileset") == 0) break;
}

static void parser_objectgroup(Carte *c, XmlScanner *s, XmlEvenement *ev) {
    (void)c;
    if (ev->self_closing) return;
    XmlEvenement sous;
    while (xml_suivant(s, &sous))
        if (sous.type == XML_FIN_ELEMENT && strcmp(sous.nom, "objectgroup") == 0) break;
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

/* stubs API (remplis tasks suivantes) */
Objet *chercher_objet(Carte *carte, const char *nom) { (void)carte; (void)nom; return NULL; }
const char *prop_str(Objet *o, const char *nom, const char *defaut) { (void)o; (void)nom; return defaut; }
int prop_int(Objet *o, const char *nom, int defaut) { (void)o; (void)nom; return defaut; }
float prop_float(Objet *o, const char *nom, float defaut) { (void)o; (void)nom; return defaut; }
int prop_bool(Objet *o, const char *nom, int defaut) { (void)o; (void)nom; return defaut; }
int carte_collision_point(Carte *carte, float x, float y) { (void)carte; (void)x; (void)y; return 0; }
int carte_collision_rect(Carte *carte, RectF zone) { (void)carte; (void)zone; return 0; }
void afficher_carte(Carte *carte, struct Camera *camera, struct Rendu *rendu) { (void)carte; (void)camera; (void)rendu; }
void afficher_scene(Carte *carte, struct ListeEntites *entites, struct Camera *camera, struct Rendu *rendu) {
    (void)carte; (void)entites; (void)camera; (void)rendu;
}
```

- [ ] **Step 4: Vérifier que `test_tilemap_couche_csv` passe (après Task 8)**

Run: `make test`
Expected: PASS.

- [ ] **Step 5: Commit**

```bash
git add src/engine/tilemap.h src/engine/tilemap.c tests/test_tilemap.c
git commit -m "feat: tilemap TMX — modèle de données + chargement couches CSV"
```

---

## Task 3: Tilesets (inline + .tsx externe) et résolution gid

**Files:**
- Modify: `src/engine/tilemap.c`
- Test: `tests/test_tilemap.c`

- [ ] **Step 1: Ajouter le test (résolution gid + tileset externe)**

Ajoute dans `tests/test_tilemap.c`. Crée un `.tsx` externe et un `.tmx` qui le référence + un tileset inline.

```c
static int test_tilemap_tilesets(void) {
    /* tileset externe .tsx (image factice, renderer NULL => pas de texture) */
    const char *tsx =
        "<?xml version=\"1.0\"?>\n"
        "<tileset name=\"ext\" tilewidth=\"16\" tileheight=\"16\" tilecount=\"32\" columns=\"8\">\n"
        " <image source=\"ext.png\" width=\"128\" height=\"64\"/>\n"
        "</tileset>\n";
    ecrire_fichier("bin/ext.tsx", tsx);

    const char *tmx =
        "<?xml version=\"1.0\"?>\n"
        "<map width=\"2\" height=\"1\" tilewidth=\"16\" tileheight=\"16\">\n"
        " <tileset firstgid=\"1\" name=\"in\" tilewidth=\"16\" tileheight=\"16\" tilecount=\"4\" columns=\"4\">\n"
        "  <image source=\"in.png\" width=\"64\" height=\"16\"/>\n"
        " </tileset>\n"
        " <tileset firstgid=\"5\" source=\"ext.tsx\"/>\n"
        " <layer name=\"L\" width=\"2\" height=\"1\"><data encoding=\"csv\">1,6</data></layer>\n"
        "</map>\n";
    ecrire_fichier("bin/t_ts.tmx", tmx);

    Carte c = charger_carte(NULL, "bin/t_ts.tmx");
    ASSERT_EGAL(2, c.nb_tilesets);
    ASSERT_EGAL(1, c.tilesets[0].firstgid);
    ASSERT_EGAL(4, c.tilesets[0].colonnes);
    ASSERT_EGAL(5, c.tilesets[1].firstgid);
    ASSERT_EGAL(8, c.tilesets[1].colonnes);
    ASSERT_EGAL(32, c.tilesets[1].tilecount);

    detruire_carte(&c);
    remove("bin/t_ts.tmx");
    remove("bin/ext.tsx");
    return 1;
}
```

Et ajoute `LANCER_TEST(test_tilemap_tilesets);` dans `suite_tilemap`.

- [ ] **Step 2: Vérifier l'échec**

Run: `make test`
Expected: FAIL (`nb_tilesets` == 0, stub).

- [ ] **Step 3: Remplacer le stub `parser_tileset` + ajouter helpers**

Remplace le stub `parser_tileset` par l'implémentation réelle. Ajoute aussi `parser_corps_tileset` (parse le contenu commun inline/tsx) et la résolution d'image (chargée seulement si `renderer` non NULL — passe `renderer` via un champ statique de fichier ou un paramètre). Pour rester simple, on stocke le renderer dans une variable static de fichier le temps du chargement.

Ajoute en haut du fichier, après les includes :

```c
static SDL_Renderer *g_renderer_chargement = NULL;
```

Dans `charger_carte`, remplace `(void)renderer;` (le second) par :

```c
    g_renderer_chargement = renderer;
```

et après `xml_fermer(s);` ajoute `g_renderer_chargement = NULL;` (avant le `return`, après l'agrégation collision de Task 5).

Implémentation (remplace le stub) :

```c
static void parser_animation(Tileset *t, XmlScanner *s, int tile_local) {
    if (t->nb_animations >= 64) return;
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

/* parse le contenu d'un <tileset> (inline ou racine de .tsx) jusqu'à sa fin.
   dossier = dossier du fichier contenant l'<image>. */
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
        /* tileset externe .tsx */
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
        /* consommer la fin du <tileset .../> dans le .tmx si self-closing déjà géré */
        if (!ev->self_closing) {
            XmlEvenement sous;
            while (xml_suivant(s, &sous))
                if (sous.type == XML_FIN_ELEMENT && strcmp(sous.nom, "tileset") == 0) break;
        }
    } else {
        /* tileset inline */
        parser_corps_tileset(t, s, ev, dossier_tmx, "tileset");
    }
    c->nb_tilesets++;
}
```

- [ ] **Step 4: Vérifier que les tests passent**

Run: `make test`
Expected: `test_tilemap_tilesets` PASS, anciens tests toujours PASS.

- [ ] **Step 5: Commit**

```bash
git add src/engine/tilemap.c tests/test_tilemap.c
git commit -m "feat: tilemap TMX — tilesets inline + .tsx externe + animations"
```

---

## Task 4: Objets, propriétés typées et accesseurs

**Files:**
- Modify: `src/engine/tilemap.c`
- Test: `tests/test_tilemap.c`

- [ ] **Step 1: Ajouter le test objets/propriétés**

```c
static int test_tilemap_objets(void) {
    const char *tmx =
        "<?xml version=\"1.0\"?>\n"
        "<map width=\"4\" height=\"4\" tilewidth=\"16\" tileheight=\"16\">\n"
        " <objectgroup name=\"PNJ\" class=\"PNJ\">\n"
        "  <object id=\"1\" name=\"Bob\" type=\"PNJ\" x=\"32\" y=\"48\" width=\"16\" height=\"24\">\n"
        "   <properties>\n"
        "    <property name=\"speed\" type=\"float\" value=\"1.5\"/>\n"
        "    <property name=\"hp\" type=\"int\" value=\"10\"/>\n"
        "    <property name=\"boss\" type=\"bool\" value=\"true\"/>\n"
        "    <property name=\"dir\" value=\"bas\"/>\n"
        "   </properties>\n"
        "  </object>\n"
        " </objectgroup>\n"
        "</map>\n";
    ecrire_fichier("bin/t_obj.tmx", tmx);

    Carte c = charger_carte(NULL, "bin/t_obj.tmx");
    ASSERT_EGAL(1, c.nb_elements);
    ASSERT_EGAL(ELEM_GROUPE_OBJETS, c.elements[0].type);
    ASSERT_EGAL(1, c.elements[0].groupe.nb_objets);

    Objet *o = chercher_objet(&c, "Bob");
    ASSERT_NON_NULL(o);
    ASSERT_EGAL_STR("PNJ", o->classe);
    ASSERT_EGAL_FLOAT(32.0f, o->x, 0.01f);
    ASSERT_EGAL_FLOAT(48.0f, o->y, 0.01f);
    ASSERT_EGAL_FLOAT(16.0f, o->largeur, 0.01f);
    ASSERT_EGAL_FLOAT(24.0f, o->hauteur, 0.01f);
    ASSERT_EGAL_FLOAT(1.5f, prop_float(o, "speed", 0.0f), 0.01f);
    ASSERT_EGAL(10, prop_int(o, "hp", 0));
    ASSERT_EGAL(1, prop_bool(o, "boss", 0));
    ASSERT_EGAL_STR("bas", prop_str(o, "dir", "x"));
    ASSERT_EGAL_STR("def", prop_str(o, "absent", "def"));

    detruire_carte(&c);
    remove("bin/t_obj.tmx");
    return 1;
}
```

Ajoute `LANCER_TEST(test_tilemap_objets);` dans `suite_tilemap`.

- [ ] **Step 2: Vérifier l'échec**

Run: `make test`
Expected: FAIL (stub objectgroup ignore tout, `chercher_objet` retourne NULL).

- [ ] **Step 3: Remplacer le stub `parser_objectgroup` + implémenter accesseurs**

```c
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
                if (!type)                       p->type = PROP_STR;
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
```

Remplace les stubs des accesseurs :

```c
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
```

Supprime les anciens stubs correspondants.

- [ ] **Step 4: Vérifier que les tests passent**

Run: `make test`
Expected: `test_tilemap_objets` PASS.

- [ ] **Step 5: Commit**

```bash
git add src/engine/tilemap.c tests/test_tilemap.c
git commit -m "feat: tilemap TMX — objets + propriétés typées + accesseurs"
```

---

## Task 5: Collision AABB

**Files:**
- Modify: `src/engine/tilemap.c`
- Test: `tests/test_tilemap.c`

- [ ] **Step 1: Ajouter le test collision**

```c
static int test_tilemap_collision(void) {
    const char *tmx =
        "<?xml version=\"1.0\"?>\n"
        "<map width=\"10\" height=\"10\" tilewidth=\"16\" tileheight=\"16\">\n"
        " <objectgroup name=\"Coll\" class=\"Collision\">\n"
        "  <object id=\"1\" type=\"Collision\" x=\"10\" y=\"10\" width=\"20\" height=\"20\"/>\n"
        "  <object id=\"2\" type=\"Collision\" x=\"100\" y=\"100\" width=\"10\" height=\"10\"/>\n"
        " </objectgroup>\n"
        "</map>\n";
    ecrire_fichier("bin/t_coll.tmx", tmx);

    Carte c = charger_carte(NULL, "bin/t_coll.tmx");
    ASSERT_EGAL(2, c.nb_collisions);

    ASSERT_VRAI(carte_collision_point(&c, 15.0f, 15.0f));
    ASSERT_FAUX(carte_collision_point(&c, 50.0f, 50.0f));

    RectF chevauche = {20, 20, 30, 30};
    ASSERT_VRAI(carte_collision_rect(&c, chevauche));
    RectF loin = {200, 200, 5, 5};
    ASSERT_FAUX(carte_collision_rect(&c, loin));
    RectF adjacent = {30, 10, 5, 5}; /* bord droit du 1er rect (x=10..30) */
    ASSERT_FAUX(carte_collision_rect(&c, adjacent));

    detruire_carte(&c);
    remove("bin/t_coll.tmx");
    return 1;
}
```

Ajoute `LANCER_TEST(test_tilemap_collision);`.

- [ ] **Step 2: Vérifier l'échec**

Run: `make test`
Expected: FAIL (`nb_collisions` == 0).

- [ ] **Step 3: Agréger les collisions + implémenter les tests AABB**

Dans `charger_carte`, après `xml_fermer(s);` et avant `return c;`, ajoute l'agrégation :

```c
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
```

Remplace les stubs collision :

```c
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
```

- [ ] **Step 4: Vérifier que les tests passent**

Run: `make test`
Expected: `test_tilemap_collision` PASS.

- [ ] **Step 5: Commit**

```bash
git add src/engine/tilemap.c tests/test_tilemap.c
git commit -m "feat: tilemap TMX — agrégation + tests collision AABB"
```

---

## Task 6: Résolution gid → tileset et sélection de frame d'animation

**Files:**
- Modify: `src/engine/tilemap.c`
- Test: `tests/test_tilemap.c`

- [ ] **Step 1: Exposer les helpers pour les tests**

Ces fonctions sont pures et testables. Déclare-les dans `tilemap.h` (section interne mais publique pour les tests) après l'API collision :

```c
/* Helpers internes exposés pour les tests (logique pure, sans renderer) */
Tileset *tileset_pour_gid(Carte *carte, int gid);
int      frame_active(TuileAnimee *anim, int temps_ms);
```

- [ ] **Step 2: Ajouter le test gid + animation**

```c
static int test_tilemap_gid_resolution(void) {
    const char *tsx =
        "<?xml version=\"1.0\"?>\n"
        "<tileset name=\"e\" tilewidth=\"16\" tileheight=\"16\" tilecount=\"8\" columns=\"8\">\n"
        " <image source=\"e.png\" width=\"128\" height=\"16\"/>\n"
        "</tileset>\n";
    ecrire_fichier("bin/e.tsx", tsx);
    const char *tmx =
        "<?xml version=\"1.0\"?>\n"
        "<map width=\"1\" height=\"1\" tilewidth=\"16\" tileheight=\"16\">\n"
        " <tileset firstgid=\"1\" name=\"a\" tilewidth=\"16\" tileheight=\"16\" tilecount=\"4\" columns=\"4\">\n"
        "  <image source=\"a.png\" width=\"64\" height=\"16\"/>\n"
        " </tileset>\n"
        " <tileset firstgid=\"5\" source=\"e.tsx\"/>\n"
        "</map>\n";
    ecrire_fichier("bin/t_gid.tmx", tmx);

    Carte c = charger_carte(NULL, "bin/t_gid.tmx");
    ASSERT_EGAL(1, tileset_pour_gid(&c, 1)->firstgid);
    ASSERT_EGAL(1, tileset_pour_gid(&c, 4)->firstgid);
    ASSERT_EGAL(5, tileset_pour_gid(&c, 5)->firstgid);
    ASSERT_EGAL(5, tileset_pour_gid(&c, 8)->firstgid);
    ASSERT_NULL(tileset_pour_gid(&c, 0));

    detruire_carte(&c);
    remove("bin/t_gid.tmx"); remove("bin/e.tsx");
    return 1;
}

static int test_tilemap_animation(void) {
    TuileAnimee a = {0};
    a.tile_local = 14;
    a.nb_frames = 3;
    a.frames[0] = (FrameAnim){14, 100};
    a.frames[1] = (FrameAnim){13, 100};
    a.frames[2] = (FrameAnim){12, 100};

    ASSERT_EGAL(14, frame_active(&a, 0));    /* début */
    ASSERT_EGAL(14, frame_active(&a, 50));
    ASSERT_EGAL(13, frame_active(&a, 150));
    ASSERT_EGAL(12, frame_active(&a, 250));
    ASSERT_EGAL(14, frame_active(&a, 300));  /* bouclage (cycle=300) */
    ASSERT_EGAL(13, frame_active(&a, 450));
    return 1;
}
```

Ajoute les deux `LANCER_TEST`.

- [ ] **Step 3: Vérifier l'échec**

Run: `make test`
Expected: lien échoue (`tileset_pour_gid`, `frame_active` non définis).

- [ ] **Step 4: Implémenter les helpers**

```c
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
```

- [ ] **Step 5: Vérifier que les tests passent**

Run: `make test`
Expected: `test_tilemap_gid_resolution` + `test_tilemap_animation` PASS.

- [ ] **Step 6: Commit**

```bash
git add src/engine/tilemap.h src/engine/tilemap.c tests/test_tilemap.c
git commit -m "feat: tilemap TMX — résolution gid + sélection frame animée"
```

---

## Task 7: Rendu (afficher_carte / afficher_scene)

**Files:**
- Modify: `src/engine/tilemap.c`

Le rendu n'est pas testé unitairement (besoin d'un renderer). Validation visuelle via la démo (Task 9).

- [ ] **Step 1: Implémenter le rendu (remplace les stubs)**

```c
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
```

Supprime les anciens stubs `afficher_carte`/`afficher_scene`.

- [ ] **Step 2: Vérifier que ça compile et que les tests passent toujours**

Run: `make test`
Expected: tous les tests PASS (le rendu n'a pas de test mais ne doit pas régresser le build).

- [ ] **Step 3: Commit**

```bash
git add src/engine/tilemap.c
git commit -m "feat: tilemap TMX — rendu couches/animations + ancre entités"
```

---

## Task 8: Intégration build et tests

**Files:**
- Modify: `Makefile`
- Modify: `tests/main_tests.c`

- [ ] **Step 1: Ajouter `xml.c` à ENGINE_SRC**

Dans `Makefile`, modifie `ENGINE_SRC` pour inclure `src/engine/xml.c` (préserve toutes les lignes SDL2_mixer existantes). Résultat de `ENGINE_SRC` :

```make
ENGINE_SRC  = src/engine/state.c src/engine/entity.c src/engine/camera.c \
              src/engine/renderer.c src/engine/sprite.c src/engine/tilemap.c \
              src/engine/text.c src/engine/flags.c src/engine/save.c \
              src/engine/dialogue.c src/engine/xml.c src/engine/moteur.c
```

- [ ] **Step 2: Intégrer les suites de tests dans `tests/main_tests.c`**

Ajoute les includes (après `test_dialogue.c`) :

```c
#include "test_xml.c"
#include "test_tilemap.c"
```

Et les appels (après `suite_dialogue();`) :

```c
    suite_xml();
    suite_tilemap();
```

- [ ] **Step 3: Build complet**

Run: `make clean && make test`
Expected: compile sans warning, tous les tests PASS (anciens + nouveaux xml + tilemap).

- [ ] **Step 4: Commit**

```bash
git add Makefile tests/main_tests.c
git commit -m "build: intégration module xml + suites de tests TMX"
```

---

## Task 9: Démo charge map3.tmx (validation visuelle)

**Files:**
- Modify: `main.c`

- [ ] **Step 1: Charger et afficher la carte dans la démo**

Réécris `main.c` pour charger `resources/maps/map3.tmx`, centrer la caméra, et afficher la carte chaque frame. Stocke la `Carte` dans `etat.donnees`.

```c
#include "moteur.h"
#include <stdio.h>

static void demo_init(Etat *etat) {
    Carte *carte = (Carte *)etat->donnees;
    (void)carte;
}
static void demo_events(Etat *etat, Entree *entree) {
    (void)etat;
    if (action_pressee(entree, ACTION_ANNULER))
        printf("Echap presse !\n");
}
static void demo_update(Etat *etat, float dt) { (void)etat; (void)dt; }
static void demo_render(Etat *etat, Rendu *rendu) {
    Carte *carte = (Carte *)etat->donnees;
    if (carte) afficher_carte(carte, rendu->camera, rendu);
}
static void demo_destroy(Etat *etat) { (void)etat; }

int main(int argc, char *argv[]) {
    (void)argc; (void)argv;

    Moteur moteur = creer_moteur("MoteurSDL2 — Demo TMX", 800, 600);
    if (!moteur.en_cours) return 1;

    Carte carte = charger_carte(moteur.rendu.renderer, "resources/maps/map3.tmx");

    Etat demo = {0};
    demo.donnees = &carte;
    demo.initialiser = demo_init;
    demo.gerer_evenements = demo_events;
    demo.mettre_a_jour = demo_update;
    demo.afficher = demo_render;
    demo.detruire = demo_destroy;

    ajouter_etat(&moteur, demo);
    lancer_moteur(&moteur);

    detruire_carte(&carte);
    detruire_moteur(&moteur);
    return 0;
}
```

Vérifie que `Etat` possède bien un champ `donnees` (sinon, adapte : variable globale ou champ existant). Vérifie aussi que `moteur.rendu` est le bon nom de champ (voir `include/moteur.h`).

- [ ] **Step 2: Build + lancement visuel**

Run: `make && make dlls && make run`
Expected: la fenêtre affiche les couches de `map3.tmx` (sol + premier plan + second plan), tuiles animées visibles. Vérifie qu'aucune erreur `image introuvable` n'apparaît dans la console.

- [ ] **Step 3: Commit**

```bash
git add main.c
git commit -m "demo: chargement et rendu de resources/maps/map3.tmx"
```

---

## Self-Review

- **Couverture spec :** xml SAX (Task 1) ✓ ; couches CSV (Task 2) ✓ ; tilesets inline+tsx (Task 3) ✓ ; objets+props (Task 4) ✓ ; collision AABB (Task 5) ✓ ; gid+animation (Task 6) ✓ ; rendu+Z-level entités (Task 7) ✓ ; Makefile+tests (Task 8) ✓ ; démo (Task 9) ✓.
- **Hors périmètre respecté :** pas de base64/zlib, pas d'infinite, orthogonal uniquement, pas de création d'entités.
- **Déviation documentée :** `collision_rect`/`collision_point` renommés `carte_collision_rect`/`carte_collision_point` pour éviter le conflit avec `entity.h`.
- **Cohérence des types :** `tileset_pour_gid`/`frame_active` déclarés en Task 6 et utilisés en Task 7 avec les mêmes signatures ; `g_renderer_chargement` introduit Task 3, remis à NULL Task 5.
- **Point d'attention build :** la chaîne de dispatch de `charger_carte` est écrite en Task 2 avec des forward-declarations de `parser_tileset`/`parser_objectgroup` ; leurs stubs sont définis dans le même fichier (Task 2) puis remplacés (Tasks 3/4). Le build reste vert à chaque task.
