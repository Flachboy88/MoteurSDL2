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
