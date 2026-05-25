#include "test_framework.h"
#include "core/timer.h"

static int test_creer_timer(void) {
    SDL_Delay(1); /* garantit que GetTicks() > 0 */
    Timer t = creer_timer();
    ASSERT_EGAL_FLOAT(0.0f, t.delta_time, 0.001f);
    ASSERT_VRAI(t.dernier_tick > 0);
    return 1;
}

static int test_delta_time_positif(void) {
    Timer t = creer_timer();
    SDL_Delay(16);
    float dt = calculer_delta_time(&t);
    ASSERT_VRAI(dt > 0.0f);
    ASSERT_VRAI(dt < 1.0f);
    return 1;
}

static void suite_timer(void) {
    printf("--- Timer ---\n");
    LANCER_TEST(test_creer_timer);
    LANCER_TEST(test_delta_time_positif);
}
