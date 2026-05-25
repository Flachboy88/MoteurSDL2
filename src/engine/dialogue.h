#ifndef DIALOGUE_H
#define DIALOGUE_H

struct Flags;
struct Police;
struct Rendu;
struct Moteur;

typedef enum {
    ACTION_DLG_AUCUNE,
    ACTION_DLG_DONNER_OBJET,
    ACTION_DLG_SET_FLAG,
    ACTION_DLG_JOUER_SON,
    ACTION_DLG_SOIGNER
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

typedef struct BoiteDialogue {
    char texte_complet[512];
    int caracteres_affiches;
    float vitesse;
    float chrono;
    int terminee;
    struct Police *police;
} BoiteDialogue;

FichierDialogues *charger_dialogues(const char *chemin);
void              detruire_dialogues(FichierDialogues *dialogues);
BlocDialogue     *trouver_dialogue(FichierDialogues *dialogues,
                                    const char *nom_pnj,
                                    struct Flags *flags);

BoiteDialogue creer_boite_dialogue(struct Police *police, float vitesse);
void          afficher_dialogue_texte(BoiteDialogue *boite, const char *texte);
void          mettre_a_jour_dialogue(BoiteDialogue *boite, float delta_time);
void          dessiner_dialogue(BoiteDialogue *boite, struct Rendu *rendu);
void          passer_dialogue(BoiteDialogue *boite);
int           dialogue_termine(BoiteDialogue *boite);

#endif
