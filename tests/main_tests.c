#include "test_framework.h"
#include <SDL2/SDL.h>

#include "test_timer.c"
#include "test_save.c"
#include "test_flags.c"

int main(int argc, char *argv[]) {
    (void)argc; (void)argv;

    if (SDL_Init(SDL_INIT_TIMER) != 0) {
        fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
        return 1;
    }

    DEBUT_TESTS();
    suite_timer();
    suite_save();
    suite_flags();
    FIN_TESTS();

    SDL_Quit();
    return resultat_tests();
}
