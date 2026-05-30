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
