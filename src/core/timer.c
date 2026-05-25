#include "core/timer.h"

Timer creer_timer(void) {
    Timer t;
    t.dernier_tick = SDL_GetTicks();
    t.delta_time = 0.0f;
    return t;
}

float calculer_delta_time(Timer *timer) {
    Uint32 maintenant = SDL_GetTicks();
    timer->delta_time = (maintenant - timer->dernier_tick) / 1000.0f;
    timer->dernier_tick = maintenant;
    return timer->delta_time;
}
