# Tilemap TMX — Design

**Date :** 2026-05-31
**Objectif :** Remplacer le format de carte `.map` maison par un chargeur du format **TMX** de Tiled, avec couches arbitraires, tilesets multiples (inline + `.tsx` externes), tuiles animées, objets exposés bruts et collision par rectangles AABB.

**Contexte :** Le module `src/engine/tilemap.c` actuel lit un format texte `.map` (4 couches fixes, collision sur grille). Aucun code ne l'utilise (la démo `main.c` ne charge pas de carte). Les vraies cartes du projet sont des `.tmx` (voir `resources/maps/map3.tmx`). Ce design remplace entièrement le format `.map`.

---

## Décisions actées (issues du brainstorming)

- Parser XML **maison**, approche **scanner SAX minimal** réutilisable (module `xml`), pas de dépendance externe.
- **Couches arbitraires** : l'ordre des éléments dans le fichier = ordre de rendu (modèle Tiled).
- Tilesets **inline** (`<tileset>` avec `<image>`) **et externes** (`<tileset source="x.tsx"/>`).
- Chemins d'images/`.tsx` résolus **relativement au fichier référent** (le `.tmx` pour un tileset inline ou pour le `source` d'un `.tsx` ; le `.tsx` pour l'`<image>` qu'il contient).
- Objets exposés **bruts** (classe + propriétés typées) ; le jeu instancie ses entités. Le moteur ne crée pas d'entités de gameplay.
- **Z-level par l'ordre des calques** : les entités sont dessinées à l'ancre du **premier** groupe d'objets rencontré.
- **Tuiles animées** incluses (horloge globale, rendu sans état stocké).
- **Collision** = objets de **classe `Collision`**, test **AABB float**. La grille `case_praticable` est supprimée.
- Encodage des données de couche : **CSV uniquement**.

### Hors périmètre
- Encodage `<data>` base64 / base64+zlib.
- Cartes infinies (`infinite="1"`).
- Orientations isométrique / hexagonale (orthogonal uniquement).
- Création d'entités de gameplay par le moteur.

---

## Architecture / modules

- `src/engine/xml.h` + `xml.c` — **scanner XML SAX minimal**, générique, testable sans SDL. Itère sur un flux d'événements : début d'élément (+ attributs), fin d'élément, texte. Utilisé pour `.tmx` et `.tsx`.
- `src/engine/tilemap.h` + `tilemap.c` — **réécrit**. Consomme le scanner XML, construit la `Carte`, gère le rendu, les animations et la collision.

---

## Module `xml` (scanner SAX minimal)

Suffisant pour le sous-ensemble TMX/TSX bien formé produit par Tiled.

```c
#ifndef XML_H
#define XML_H

#define XML_MAX_ATTRS 32

typedef struct XmlAttr {
    char nom[64];
    char valeur[256];
} XmlAttr;

typedef enum {
    XML_DEBUT_ELEMENT,   /* <tag attr="..."> ou <tag .../> */
    XML_FIN_ELEMENT,     /* </tag> ou fin d'un self-closing */
    XML_TEXTE,           /* texte entre balises (ex: données CSV) */
    XML_FIN              /* fin du document */
} XmlTypeEvenement;

typedef struct XmlEvenement {
    XmlTypeEvenement type;
    char nom[64];                  /* nom de l'élément (DEBUT/FIN) */
    XmlAttr attrs[XML_MAX_ATTRS];
    int nb_attrs;
    const char *texte;             /* pointeur dans le buffer (TEXTE) */
    int self_closing;              /* 1 si <tag/> */
} XmlEvenement;

typedef struct XmlScanner {
    char *buffer;                  /* contenu du fichier, possédé */
    long taille;
    long pos;
} XmlScanner;

XmlScanner *xml_ouvrir(const char *chemin);   /* lit le fichier en mémoire */
XmlScanner *xml_ouvrir_chaine(const char *contenu); /* pour les tests */
int  xml_suivant(XmlScanner *s, XmlEvenement *ev);  /* 0 quand XML_FIN */
const char *xml_attr(XmlEvenement *ev, const char *nom); /* NULL si absent */
void xml_fermer(XmlScanner *s);

#endif
```

Comportement :
- Ignore la déclaration `<?xml ... ?>` et les commentaires `<!-- ... -->`.
- Gère `<tag a="1" b="2">`, `</tag>`, `<tag .../>` (émet DEBUT puis FIN, `self_closing=1`).
- Valeurs d'attributs entre guillemets doubles, espaces autorisés (`name="Walls and Floors"`).
- Décode les entités XML de base dans les valeurs/texte : `&amp; &lt; &gt; &quot; &apos;`.
- Le texte entre balises (données CSV) est exposé via `XML_TEXTE` ; le consommateur le parse.

---

## Modèle de données (`tilemap.h`)

```c
#include <SDL2/SDL.h>

struct Camera;
struct Rendu;
struct ListeEntites;

/* ---- Tilesets ---- */
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

/* ---- Propriétés typées ---- */
typedef enum { PROP_STR, PROP_INT, PROP_FLOAT, PROP_BOOL } TypeProp;
typedef struct Propriete {
    char nom[64];
    TypeProp type;
    char valeur[128];
} Propriete;

/* ---- Objets ---- */
typedef struct Objet {
    char nom[64];
    char classe[32];          /* type="..." ou class="..." */
    float x, y, largeur, hauteur;
    Propriete props[32];
    int nb_props;
} Objet;

/* ---- Éléments ordonnés ---- */
typedef enum { ELEM_COUCHE_TUILES, ELEM_GROUPE_OBJETS } TypeElement;

typedef struct CoucheTuiles {
    char nom[64];
    int colonnes, lignes;
    int *gids;                /* [colonnes*lignes], 0 = vide */
} CoucheTuiles;

typedef struct GroupeObjets {
    char nom[64];
    char classe[32];
    Objet *objets;
    int nb_objets;
} GroupeObjets;

typedef struct ElementCarte {
    TypeElement type;
    CoucheTuiles couche;      /* si ELEM_COUCHE_TUILES */
    GroupeObjets groupe;      /* si ELEM_GROUPE_OBJETS */
} ElementCarte;

/* ---- Collision ---- */
typedef struct RectF { float x, y, largeur, hauteur; } RectF;

/* ---- Carte ---- */
typedef struct Carte {
    int colonnes, lignes;
    int taille_tile;
    Tileset tilesets[8];
    int nb_tilesets;
    ElementCarte *elements;   /* ordre du fichier = ordre de rendu */
    int nb_elements;
    RectF *collisions;        /* agrégé au chargement depuis objets classe "Collision" */
    int nb_collisions;
} Carte;
```

---

## API publique (`tilemap.h`)

```c
Carte   charger_carte(SDL_Renderer *renderer, const char *chemin_tmx);
void    detruire_carte(Carte *carte);

/* Rendu */
void    afficher_carte(Carte *carte, struct Camera *camera, struct Rendu *rendu);
void    afficher_scene(Carte *carte, struct ListeEntites *entites,
                       struct Camera *camera, struct Rendu *rendu);

/* Objets */
Objet  *chercher_objet(Carte *carte, const char *nom);
const char *prop_str(Objet *o, const char *nom, const char *defaut);
int     prop_int(Objet *o, const char *nom, int defaut);
float   prop_float(Objet *o, const char *nom, float defaut);
int     prop_bool(Objet *o, const char *nom, int defaut);

/* Collision */
int     collision_point(Carte *carte, float x, float y);
int     collision_rect(Carte *carte, RectF zone);
```

---

## Flux de chargement

1. `xml_ouvrir(chemin_tmx)` ; mémoriser le dossier du `.tmx` (pour résoudre les chemins relatifs).
2. À chaque `<tileset>` :
   - **inline** (`<image source=...>` enfant) : charger l'image (chemin relatif au `.tmx`), remplir `colonnes/tilecount` ; parser les `<tile id><animation><frame .../>` enfants en `TuileAnimee`.
   - **externe** (`source="x.tsx"`) : ouvrir le `.tsx` (chemin relatif au `.tmx`), parser comme un tileset inline ; l'`<image>` y est résolue relativement au **`.tsx`**. Conserver le `firstgid` venu du `.tmx`.
3. À chaque `<layer>` : créer `CoucheTuiles`, lire `<data encoding="csv">`, parser le texte CSV → `gids[]`. Ajouter un `ElementCarte` (ordre préservé).
4. À chaque `<objectgroup>` : créer `GroupeObjets`, parser chaque `<object>` (nom, type/class, x/y/width/height, `<properties>`). Ajouter un `ElementCarte`.
5. Après parsing : agréger dans `collisions[]` tous les objets de classe `Collision` (en `RectF`).

Résolution de chemin : fonction utilitaire `joindre_chemin(dossier_base, source)` qui concatène et normalise (le `source` est relatif au fichier qui le référence).

---

## Rendu et Z-level

`afficher_scene(carte, entites, camera, rendu)` parcourt `elements[]` dans l'ordre :
- `ELEM_COUCHE_TUILES` → pour chaque gid non nul : résoudre `gid → tileset` + id local, calculer la frame d'animation courante, `SDL_RenderCopy` avec offset caméra.
- `ELEM_GROUPE_OBJETS` → au **premier** groupe rencontré seulement : `trier_entites_par_profondeur(entites)` puis `afficher_toutes_entites(entites, rendu)`. Les groupes suivants sont ignorés (données, non visuels).

`afficher_carte` fait la même chose mais sans entités (utile pour debug / cartes sans acteurs).

**Résolution gid → tileset :** tileset dont `firstgid` est le plus grand parmi ceux ≤ gid. Id local = `gid - firstgid`. Colonne/ligne source = `local % colonnes`, `local / colonnes`.

**Animation :** helper interne `gid_src_rect(carte, gid, temps_ms, *tileset_out, *src_out)`. Si le tileset a une `TuileAnimee` pour l'id local : `cycle = somme(durées)`, `t = temps_ms % cycle`, parcourir les frames en accumulant les durées pour trouver la frame active → utiliser `frame.tile_local` pour le rect source. Sinon rect source statique. `temps_ms = SDL_GetTicks()` lu dans la fonction de rendu (aucun état stocké dans la carte).

---

## Collision

- Au chargement, `collisions[]` reçoit un `RectF` par objet de classe `Collision`.
- `collision_rect(carte, zone)` : vrai si `zone` chevauche au moins un `collisions[i]` (AABB float, chevauchement strict, l'adjacence ne compte pas).
- `collision_point(carte, x, y)` : vrai si `(x, y)` est dans au moins un rectangle.

---

## Tests

**`tests/test_xml.c`** (sans SDL) :
- séquence d'événements sur un XML imbriqué simple
- self-closing `<x/>` → DEBUT puis FIN avec `self_closing=1`
- `xml_attr` : présent → valeur, absent → NULL, valeur avec espaces
- texte entre balises récupéré
- décodage `&amp;`/`&lt;`/`&gt;`

**`tests/test_tilemap.c`** (parsing sans renderer ; `charger_carte(NULL, ...)` → pas de texture chargée mais structures remplies) :
- chargement d'un petit `.tmx` de test (créé dans le test, encodage CSV) → `colonnes/lignes/taille_tile`, `nb_elements` et ordre corrects
- tilesets multiples : `gid → tileset` + id local
- parsing `.tsx` externe (chemin relatif)
- couche CSV : gids attendus à des positions précises
- objets : classe, x/y/w/h float, propriétés typées + `prop_int/float/bool/str`
- collision : agrégation des objets `Collision`, `collision_rect` (chevauche / chevauche pas / adjacent)
- animation : `gid_src_rect` à t=0 (frame 0), t après durée frame 0 (frame 1), bouclage

Le rendu (`SDL_RenderCopy`) n'est pas testé unitairement (besoin d'un renderer) ; validation visuelle via la démo. La logique testable (résolution gid, sélection de frame, AABB) est extraite en fonctions pures.

Ajouter `suite_xml()` et `suite_tilemap()` dans `tests/main_tests.c`, et `test_xml.c` / `test_tilemap.c` (et `xml.c` dans `ENGINE_SRC`).

---

## Impact Makefile

- Ajouter `src/engine/xml.c` à `ENGINE_SRC`.
- (SDL2_mixer déjà ajouté par l'utilisateur ; sans rapport avec ce module.)

## Préparation utilisateur

Ré-exporter les `.tmx` depuis Tiled avec des chemins relatifs corrects (layout : `resources/maps/*.tmx`, `resources/tileset/*.png`, `resources/TSX/*.tsx`). Fait pour `map3.tmx`.
