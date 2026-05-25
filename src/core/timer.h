#ifndef TIMER_H
#define TIMER_H

#include <SDL2/SDL.h>

typedef struct Timer {
    Uint32 dernier_tick;
    float  delta_time;
} Timer;

Timer creer_timer(void);
float calculer_delta_time(Timer *timer);

#endif
