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
