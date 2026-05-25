# Spec — Moteur SDL2 (2D Game Engine en C)

## Vue d'ensemble

Moteur de jeu 2D en C pur (C11), basé sur SDL2, conçu pour créer des jeux 2D de type Pokémon, Tetris, etc. Le moteur est une bibliothèque statique (`libmoteur.a`) que chaque jeu linke indépendamment.

**Objectif :** Apprendre en construisant quelque chose de qualité et réutilisable.  
**Premier jeu cible :** Pokémon-like (RPG avec carte, combats au tour par tour, dialogues, inventaire).  
**Langage :** C11 pur, compilé avec MinGW 32-bit.  
**Conventions :** Noms de fonctions en français, verbes à l'infinitif.

---

## Architecture — 3 couches

```
[Jeu]       → États spécifiques, entités du jeu, assets, fichiers .dlg
[Moteur]    → Systèmes réutilisables (rendu, audio, sprites, maps, dialogues...)
[Core]      → Wrapper SDL2 (fenêtre, renderer, timer, entrées)
```

Les dépendances vont toujours vers le bas : Jeu → Moteur → Core. Jamais l'inverse.

---

## Structure de fichiers

```
D:/Perso/programmes perso/C/
│
├── MoteurSDL2/                     ← LE MOTEUR (produit libmoteur.a)
│   ├── src/
│   │   ├── core/
│   │   │   ├── core.h/.c             init/quit SDL2 global
│   │   │   ├── window.h/.c           création fenêtre + renderer
│   │   │   ├── timer.h/.c            delta time, FPS
│   │   │   └── input.h/.c            abstraction touches → actions
│   │   └── engine/
│   │       ├── renderer.h/.c         dessiner sprites/tiles, caméra
│   │       ├── sprite.h/.c           chargement spritesheet, animations
│   │       ├── tilemap.h/.c          parsing TMX, rendu couches, collisions
│   │       ├── audio.h/.c            SDL2_mixer : musique + effets
│   │       ├── text.h/.c             SDL2_ttf : affichage texte
│   │       ├── dialogue.h/.c         dialogue manager, parser .dlg, actions
│   │       ├── flags.h/.c            gestionnaire de flags d'événements
│   │       ├── save.h/.c             lecture/écriture fichiers clé=valeur
│   │       ├── config.h/.c           chargement config.cfg
│   │       ├── state.h/.c            state machine (pile d'états)
│   │       └── entity.h/.c           struct Entite de base + liste
│   ├── include/
│   │   └── moteur.h                  header public unique
│   ├── lib/
│   │   └── libmoteur.a              résultat de compilation
│   └── Makefile
│
├── PokemonLike/                    ← UN JEU
│   ├── src/
│   │   ├── main.c                    ~20 lignes
│   │   ├── player.h/.c
│   │   ├── npc.h/.c
│   │   └── states/
│   │       ├── state_overworld.h/.c
│   │       ├── state_combat.h/.c
│   │       ├── state_menu.h/.c
│   │       └── state_dialogue.h/.c
│   ├── assets/
│   │   ├── maps/          fichiers .tmx
│   │   ├── sprites/       images PNG
│   │   ├── fonts/         polices .ttf
│   │   ├── audio/         musique + sfx
│   │   └── dialogues/     fichiers .dlg
│   ├── bin/
│   └── Makefile            link vers libmoteur.a
```

---

## Module 1 — Core (wrapper SDL2) [V]

### core.h/.c

```c
int     initialiser_sdl(void);
void    quitter_sdl(void);
```

### window.h/.c

```c
typedef struct Fenetre {
    SDL_Window   *window;
    SDL_Renderer *renderer;
    int largeur, hauteur;
} Fenetre;

Fenetre creer_fenetre(const char *titre, int largeur, int hauteur);
void    detruire_fenetre(Fenetre *fenetre);
```

### timer.h/.c

```c
typedef struct Timer {
    Uint32 dernier_tick;
    float  delta_time;
} Timer;

Timer   creer_timer(void);
float   calculer_delta_time(Timer *timer);
```

### input.h/.c

```c
typedef enum {
    ACTION_HAUT, ACTION_BAS, ACTION_GAUCHE, ACTION_DROITE,
    ACTION_VALIDER, ACTION_ANNULER, ACTION_MENU,
    ACTION_MAX
} Action;

typedef struct Entree {
    int actions[ACTION_MAX];
    int actions_pressees[ACTION_MAX];
} Entree;

void    mettre_a_jour_entrees(Entree *entree);
int     action_active(Entree *entree, Action action);
int     action_pressee(Entree *entree, Action action);
void    configurer_touches(Entree *entree, Fichier_KV *config);
```

---

## Module 2 — State Machine [V]

```c
typedef struct Etat {
    void (*initialiser)(struct Etat *etat);
    void (*gerer_evenements)(struct Etat *etat, Entree *entree);
    void (*mettre_a_jour)(struct Etat *etat, float delta_time);
    void (*afficher)(struct Etat *etat, Rendu *rendu);
    void (*detruire)(struct Etat *etat);
    void *donnees;
} Etat;

typedef struct PileEtats {
    Etat etats[16];
    int sommet;
} PileEtats;

void    empiler_etat(PileEtats *pile, Etat etat);
void    depiler_etat(PileEtats *pile);
void    changer_etat(PileEtats *pile, Etat etat);
Etat   *obtenir_etat_courant(PileEtats *pile);
```

---

## Module 3 — Entités [V]

```c
typedef struct Entite {
    float x, y;
    int largeur, hauteur;
    int direction;          // 0=bas, 1=haut, 2=gauche, 3=droite
    int profondeur;         // override tri rendu
    Sprite *sprite;
    int active;

    void (*mettre_a_jour)(struct Entite *self, float delta_time);
    void (*afficher)(struct Entite *self, Rendu *rendu);
    void (*detruire)(struct Entite *self);
    void *donnees;
} Entite;

typedef struct ListeEntites {
    Entite **entites;
    int nb;
    int capacite;
} ListeEntites;

Entite        *creer_entite(float x, float y, int largeur, int hauteur);
void           detruire_entite(Entite *entite);
ListeEntites   creer_liste_entites(int capacite);
void           ajouter_entite(ListeEntites *liste, Entite *entite);
void           maj_toutes_entites(ListeEntites *liste, float delta_time);
void           trier_entites_par_profondeur(ListeEntites *liste);
void           afficher_toutes_entites(ListeEntites *liste, Rendu *rendu);
void           detruire_liste_entites(ListeEntites *liste);
int            collision_rect(Entite *a, Entite *b);
```

**Profondeur de rendu :** tri par `profondeur` puis par `y`. Les couches Tiled gèrent le décor avant/après les entités.

---

## Module 4 — Caméra [V]

```c
typedef struct Camera {
    float x, y;
    int largeur, hauteur;
    int limite_x, limite_y;
} Camera;

Camera  creer_camera(int largeur_fenetre, int hauteur_fenetre);
void    centrer_camera(Camera *camera, float cible_x, float cible_y);
void    limiter_camera(Camera *camera, int largeur_carte, int hauteur_carte);
void    fixer_camera(Camera *camera, float x, float y);
```

---

## Module 5 — Renderer [V]

```c
typedef struct Rendu {
    SDL_Renderer *renderer;
    Camera *camera;
} Rendu;

void    effacer_ecran(Rendu *rendu);
void    presenter_rendu(Rendu *rendu);
void    dessiner_rect(Rendu *rendu, int x, int y, int w, int h, Couleur couleur);
```

---

## Module 6 — Sprites et animations [V]

```c
typedef struct Frame {
    int colonne, ligne;
    float duree;
} Frame;

typedef struct Animation {
    const char *nom;
    Frame *frames;
    int nb_frames;
    int boucle;
} Animation;

typedef struct Sprite {
    SDL_Texture *texture;
    int largeur_frame, hauteur_frame;
    Animation *animations;
    int nb_animations;
    int animation_courante;
    int frame_courante;
    float chrono;
} Sprite;

Sprite  charger_sprite(const char *chemin, int larg_frame, int haut_frame);
void    detruire_sprite(Sprite *sprite);
void    ajouter_animation(Sprite *sprite, const char *nom, Frame *frames, int nb_frames, int boucle);
void    jouer_animation(Sprite *sprite, const char *nom);
void    mettre_a_jour_sprite(Sprite *sprite, float delta_time);
int     animation_terminee(Sprite *sprite);
void    afficher_sprite(Sprite *sprite, float x, float y, Camera *camera, Rendu *rendu);
```

---

## Module 7 — Tilemap (cartes .map custom) [V]

```c
typedef struct Couche {
    int *tuiles;        // tableau 1D [colonnes * lignes]
    int colonnes, lignes;
} Couche;

typedef struct Objet {
    char nom[64];
    float x, y;
    int largeur, hauteur;
    char type[32];
} Objet;

typedef struct ListeObjets {
    Objet *objets;
    int nb;
} ListeObjets;

typedef struct Carte {
    int colonnes, lignes;
    int taille_tile;
    Couche sol;
    Couche objets_bas;
    Couche objets_haut;
    Couche collision;
    ListeObjets objets;
    SDL_Texture *tileset;
} Carte;

Carte   charger_carte(const char *chemin_tmx);
void    detruire_carte(Carte *carte);
void    afficher_couche(Couche *couche, Carte *carte, Camera *camera, Rendu *rendu);
int     case_praticable(Carte *carte, int colonne, int ligne);
Objet  *chercher_objet(Carte *carte, const char *nom);

void    afficher_scene(Carte *carte, ListeEntites *entites, Camera *camera, Rendu *rendu);
```

`afficher_scene` gère l'ordre : sol → objets_bas → entités (triées) → objets_haut.

---

## Module 8 — Audio [Todo — SDL2_mixer non installé]

```c
void    initialiser_audio(void);
void    fermer_audio(void);

void    jouer_musique(const char *chemin);
void    arreter_musique(void);
void    pause_musique(void);
void    reprendre_musique(void);
void    regler_volume_musique(int volume);

int     charger_son(const char *chemin);
void    jouer_son(int id_son);
void    regler_volume_sons(int volume);
void    liberer_sons(void);
```

---

## Module 9 — Texte [V]

```c
typedef struct Police {
    TTF_Font *font;
    int taille;
} Police;

Police  charger_police(const char *chemin, int taille);
void    detruire_police(Police *police);
void    afficher_texte(Rendu *rendu, Police *police, const char *texte, int x, int y, Couleur couleur);
```

---

## Module 10 — Dialogues et Flags [V]

### Flags (événements)

```c
typedef struct Flags {
    char noms[512][64];
    int  valeurs[512];
    int  nb_flags;
} Flags;

void    definir_flag(Flags *flags, const char *nom, int valeur);
int     flag_actif(Flags *flags, const char *nom);
void    sauvegarder_flags(Flags *flags, Fichier_KV *save);
void    charger_flags(Flags *flags, Fichier_KV *save);
```

### Boîte de dialogue

```c
typedef struct BoiteDialogue {
    char texte_complet[512];
    int caracteres_affiches;
    float vitesse;
    float chrono;
    int terminee;
    Police *police;
} BoiteDialogue;

BoiteDialogue creer_boite_dialogue(Police *police, float vitesse);
void    afficher_dialogue(BoiteDialogue *boite, const char *texte);
void    mettre_a_jour_dialogue(BoiteDialogue *boite, float delta_time);
void    dessiner_dialogue(BoiteDialogue *boite, Rendu *rendu);
void    passer_dialogue(BoiteDialogue *boite);
int     dialogue_termine(BoiteDialogue *boite);
```

### Dialogue Manager

```c
typedef enum {
    ACTION_DLG_AUCUNE,
    ACTION_DLG_DONNER_OBJET,
    ACTION_DLG_SET_FLAG,
    ACTION_DLG_JOUER_SON,
    ACTION_DLG_SOIGNER,
} TypeActionDialogue;

typedef struct LigneAction {
    TypeActionDialogue type;
    char params[128];
} LigneAction;

typedef struct BlocDialogue {
    char condition[128];
    char **lignes_texte;
    int nb_lignes;
    LigneAction *actions;
    int nb_actions;
} BlocDialogue;

typedef struct FichierDialogues {
    char nom_pnj[64];
    BlocDialogue *blocs;
    int nb_blocs;
} FichierDialogues;

FichierDialogues *charger_dialogues(const char *chemin);
void              detruire_dialogues(FichierDialogues *dialogues);
BlocDialogue     *trouver_dialogue(FichierDialogues *dialogues, const char *nom_pnj, Flags *flags);
void              lancer_dialogue(Moteur *moteur, BlocDialogue *bloc);
```

### Format fichier .dlg

```
[nom_pnj]
condition: expression_flag
texte: "Ligne de dialogue"
action: type_action paramètres
```

Exemple :
```
[pnj_maman]
condition: !potion_recue
texte: "Prends ça pour la route !"
action: donner_objet potion 1
action: set_flag potion_recue

[pnj_maman]
condition: potion_recue
texte: "Sois prudent dehors !"
```

---

## Module 11 — Sauvegarde et configuration [V]

```c
typedef struct Fichier_KV {
    char cles[256][64];
    char valeurs[256][128];
    int nb_entrees;
} Fichier_KV;

Fichier_KV  lire_fichier_kv(const char *chemin);
void        ecrire_fichier_kv(const char *chemin, Fichier_KV *fichier);
const char *obtenir_valeur(Fichier_KV *fichier, const char *cle);
int         obtenir_valeur_int(Fichier_KV *fichier, const char *cle, int defaut);
void        definir_valeur(Fichier_KV *fichier, const char *cle, const char *valeur);
void        definir_valeur_int(Fichier_KV *fichier, const char *cle, int valeur);
void        sauvegarder_partie(const char *chemin, Fichier_KV *donnees);
Fichier_KV  charger_partie(const char *chemin);
int         sauvegarde_existe(const char *chemin);
```

Deux fichiers séparés, même format :
- `config.cfg` : résolution, volume, touches
- `save.sav` : progression du jeu + flags

---

## Struct Moteur (point central) [Todo]

```c
typedef struct Moteur {
    Fenetre fenetre;
    Rendu rendu;
    Timer timer;
    Entree entree;
    PileEtats etats;
    Flags flags;
    int en_cours;
} Moteur;

Moteur  creer_moteur(const char *titre, int largeur, int hauteur);
void    ajouter_etat(Moteur *moteur, Etat etat);
void    lancer_moteur(Moteur *moteur);
void    detruire_moteur(Moteur *moteur);
```

### Boucle principale (dans lancer_moteur)

```c
while (moteur->en_cours) {
    float dt = calculer_delta_time(&moteur->timer);
    mettre_a_jour_entrees(&moteur->entree);

    Etat *actuel = obtenir_etat_courant(&moteur->etats);
    actuel->gerer_evenements(actuel, &moteur->entree);
    actuel->mettre_a_jour(actuel, dt);

    effacer_ecran(&moteur->rendu);
    actuel->afficher(actuel, &moteur->rendu);
    presenter_rendu(&moteur->rendu);
}
```

---

## Dépendances externes

| Lib | Version | Usage |
|-----|---------|-------|
| SDL2 | 2.30.11 | Fenêtre, renderer, events |
| SDL2_ttf | 2.24.0 | Texte |
| SDL2_image | 2.8.4 | Chargement PNG |
| SDL2_mixer | (à ajouter) | Audio |

Toutes en version i686 (32-bit), situées dans `D:/Perso/programmes perso/Libs/`.

---

## Compilation

- Le moteur se compile en `libmoteur.a` via `ar`
- Chaque jeu linke contre cette lib + les DLLs SDL2
- Toolchain : MinGW GCC 6.3, 32-bit

---

## Module 12 — Framework de tests [V]

Framework de tests minimaliste intégré, style JUnit fait main en C. Teste uniquement la logique pure (pas de contexte SDL requis).

### Structure des fichiers

```
tests/
├── test_framework.h      ← le framework (macros, runner)
├── main_tests.c          ← point d'entrée, appelle toutes les suites
├── test_timer.c           ← tests du module timer
├── test_input.c           ← tests du module input
├── test_state.c           ← tests de la state machine
├── test_entity.c          ← tests entités + collisions
├── test_flags.c           ← tests des flags
├── test_save.c            ← tests du parser KV
└── test_dialogue.c        ← tests du parser .dlg
```

Le binaire se compile dans `bin/tests.exe`. Cible Makefile : `make test`.

### API — test_framework.h

**Macros d'assertion :**

```c
ASSERT_VRAI(condition)           // vérifie que c'est non-zéro
ASSERT_FAUX(condition)           // vérifie que c'est zéro
ASSERT_EGAL(attendu, obtenu)     // compare deux int
ASSERT_EGAL_FLOAT(a, b, eps)     // compare deux float avec tolérance
ASSERT_EGAL_STR(attendu, obtenu) // compare deux chaînes (strcmp)
ASSERT_NULL(ptr)                 // vérifie que le pointeur est NULL
ASSERT_NON_NULL(ptr)             // vérifie que le pointeur n'est pas NULL
```

Chaque macro affiche fichier, ligne et valeurs attendue/obtenue en cas d'échec, puis `return 0`. Un test qui réussit retourne `1`.

**Déclaration d'un test :**

```c
int test_mon_truc(void) {
    ASSERT_EGAL(42, ma_fonction());
    return 1;
}
```

Chaque test est une fonction `int (void)`.

**Suites et exécution :**

```c
// dans test_timer.c
void suite_timer(void) {
    LANCER_TEST(test_creer_timer);
    LANCER_TEST(test_delta_time_positif);
}

// dans main_tests.c
int main(void) {
    DEBUT_TESTS();
    suite_timer();
    suite_input();
    suite_flags();
    FIN_TESTS();
    return resultat_tests();
}
```

`DEBUT_TESTS()` initialise les compteurs. `LANCER_TEST(fn)` exécute, affiche pass/fail coloré, incrémente. `FIN_TESTS()` affiche le résumé. `resultat_tests()` retourne 0 si tout passe, 1 sinon.

**Sortie terminal :**

```
=== Tests du Moteur SDL2 ===
[PASS] test_creer_timer
[FAIL] test_action_active (test_input.c:23) — attendu: 1, obtenu: 0

=== Résultat: 15/16 tests passés ===
```

Vert pour PASS, rouge pour FAIL.
