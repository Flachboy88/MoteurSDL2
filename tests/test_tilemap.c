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

static int test_tilemap_tilesets(void) {
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
    RectF adjacent = {30, 10, 5, 5};
    ASSERT_FAUX(carte_collision_rect(&c, adjacent));

    detruire_carte(&c);
    remove("bin/t_coll.tmx");
    return 1;
}

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

    ASSERT_EGAL(14, frame_active(&a, 0));
    ASSERT_EGAL(14, frame_active(&a, 50));
    ASSERT_EGAL(13, frame_active(&a, 150));
    ASSERT_EGAL(12, frame_active(&a, 250));
    ASSERT_EGAL(14, frame_active(&a, 300));
    ASSERT_EGAL(13, frame_active(&a, 450));
    return 1;
}

static int test_tilemap_parse_animation_inline(void) {
    const char *tmx =
        "<?xml version=\"1.0\"?>\n"
        "<map width=\"1\" height=\"1\" tilewidth=\"16\" tileheight=\"16\">\n"
        " <tileset firstgid=\"1\" name=\"T\" tilewidth=\"16\" tileheight=\"16\" tilecount=\"36\" columns=\"6\">\n"
        "  <image source=\"t.png\" width=\"96\" height=\"96\"/>\n"
        "  <tile id=\"14\">\n"
        "   <animation>\n"
        "    <frame tileid=\"14\" duration=\"350\"/>\n"
        "    <frame tileid=\"13\" duration=\"350\"/>\n"
        "    <frame tileid=\"12\" duration=\"350\"/>\n"
        "   </animation>\n"
        "  </tile>\n"
        " </tileset>\n"
        "</map>\n";
    ecrire_fichier("bin/t_anim.tmx", tmx);

    Carte c = charger_carte(NULL, "bin/t_anim.tmx");
    ASSERT_EGAL(1, c.nb_tilesets);
    ASSERT_EGAL(1, c.tilesets[0].nb_animations);
    TuileAnimee *a = &c.tilesets[0].animations[0];
    ASSERT_EGAL(14, a->tile_local);
    ASSERT_EGAL(3, a->nb_frames);
    ASSERT_EGAL(14, a->frames[0].tile_local);
    ASSERT_EGAL(350, a->frames[0].duree_ms);
    ASSERT_EGAL(13, a->frames[1].tile_local);
    ASSERT_EGAL(12, a->frames[2].tile_local);
    /* gid 15 = firstgid(1) + tile 14 -> doit mapper sur ce tileset */
    ASSERT_EGAL(1, tileset_pour_gid(&c, 15)->firstgid);

    detruire_carte(&c);
    remove("bin/t_anim.tmx");
    return 1;
}

static void suite_tilemap(void) {
    LANCER_TEST(test_tilemap_couche_csv);
    LANCER_TEST(test_tilemap_tilesets);
    LANCER_TEST(test_tilemap_objets);
    LANCER_TEST(test_tilemap_collision);
    LANCER_TEST(test_tilemap_gid_resolution);
    LANCER_TEST(test_tilemap_animation);
    LANCER_TEST(test_tilemap_parse_animation_inline);
}
