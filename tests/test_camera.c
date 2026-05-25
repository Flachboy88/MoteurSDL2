#include "test_framework.h"
#include "engine/camera.h"

static int test_creer_camera(void) {
    Camera cam = creer_camera(800, 600);
    ASSERT_EGAL(800, cam.largeur);
    ASSERT_EGAL(600, cam.hauteur);
    ASSERT_EGAL_FLOAT(0.0f, cam.x, 0.001f);
    ASSERT_EGAL_FLOAT(0.0f, cam.y, 0.001f);
    return 1;
}

static int test_centrer_camera(void) {
    Camera cam = creer_camera(800, 600);
    centrer_camera(&cam, 400.0f, 300.0f);
    ASSERT_EGAL_FLOAT(0.0f, cam.x, 0.001f);
    ASSERT_EGAL_FLOAT(0.0f, cam.y, 0.001f);
    return 1;
}

static int test_centrer_camera_decale(void) {
    Camera cam = creer_camera(800, 600);
    centrer_camera(&cam, 1000.0f, 800.0f);
    ASSERT_EGAL_FLOAT(600.0f, cam.x, 0.001f);
    ASSERT_EGAL_FLOAT(500.0f, cam.y, 0.001f);
    return 1;
}

static int test_limiter_camera_min(void) {
    Camera cam = creer_camera(800, 600);
    cam.x = -50.0f;
    cam.y = -30.0f;
    limiter_camera(&cam, 1600, 1200);
    ASSERT_EGAL_FLOAT(0.0f, cam.x, 0.001f);
    ASSERT_EGAL_FLOAT(0.0f, cam.y, 0.001f);
    return 1;
}

static int test_limiter_camera_max(void) {
    Camera cam = creer_camera(800, 600);
    cam.x = 2000.0f;
    cam.y = 1500.0f;
    limiter_camera(&cam, 1600, 1200);
    ASSERT_EGAL_FLOAT(800.0f, cam.x, 0.001f);
    ASSERT_EGAL_FLOAT(600.0f, cam.y, 0.001f);
    return 1;
}

static int test_fixer_camera(void) {
    Camera cam = creer_camera(800, 600);
    fixer_camera(&cam, 123.0f, 456.0f);
    ASSERT_EGAL_FLOAT(123.0f, cam.x, 0.001f);
    ASSERT_EGAL_FLOAT(456.0f, cam.y, 0.001f);
    return 1;
}

static void suite_camera(void) {
    printf("--- Camera ---\n");
    LANCER_TEST(test_creer_camera);
    LANCER_TEST(test_centrer_camera);
    LANCER_TEST(test_centrer_camera_decale);
    LANCER_TEST(test_limiter_camera_min);
    LANCER_TEST(test_limiter_camera_max);
    LANCER_TEST(test_fixer_camera);
}
