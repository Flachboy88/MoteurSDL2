#ifndef TEST_FRAMEWORK_H
#define TEST_FRAMEWORK_H

#include <stdio.h>
#include <string.h>
#include <math.h>

#define COULEUR_VERT  "\033[32m"
#define COULEUR_ROUGE "\033[31m"
#define COULEUR_RESET "\033[0m"

static int tests_total = 0;
static int tests_passes = 0;
static int tests_echoues = 0;

#define DEBUT_TESTS() do { \
    tests_total = 0; tests_passes = 0; tests_echoues = 0; \
    printf("\n=== Tests du Moteur SDL2 ===\n\n"); \
} while(0)

#define FIN_TESTS() do { \
    printf("\n=== Resultat: %d/%d tests passes ===\n", \
           tests_passes, tests_total); \
    if (tests_echoues > 0) \
        printf("    %d test(s) echoue(s)\n", tests_echoues); \
} while(0)

#define LANCER_TEST(fn) do { \
    tests_total++; \
    if (fn()) { \
        tests_passes++; \
        printf("  " COULEUR_VERT "[PASS]" COULEUR_RESET " %s\n", #fn); \
    } else { \
        tests_echoues++; \
        printf("  " COULEUR_ROUGE "[FAIL]" COULEUR_RESET " %s\n", #fn); \
    } \
} while(0)

#define ASSERT_VRAI(cond) do { \
    if (!(cond)) { \
        printf("    ECHEC: %s:%d \xe2\x80\x94 %s est faux\n", \
               __FILE__, __LINE__, #cond); \
        return 0; \
    } \
} while(0)

#define ASSERT_FAUX(cond) do { \
    if ((cond)) { \
        printf("    ECHEC: %s:%d \xe2\x80\x94 %s est vrai\n", \
               __FILE__, __LINE__, #cond); \
        return 0; \
    } \
} while(0)

#define ASSERT_EGAL(attendu, obtenu) do { \
    int _a = (attendu), _o = (obtenu); \
    if (_a != _o) { \
        printf("    ECHEC: %s:%d \xe2\x80\x94 attendu: %d, obtenu: %d\n", \
               __FILE__, __LINE__, _a, _o); \
        return 0; \
    } \
} while(0)

#define ASSERT_EGAL_FLOAT(attendu, obtenu, epsilon) do { \
    float _a = (attendu), _o = (obtenu); \
    if (fabsf(_a - _o) > (epsilon)) { \
        printf("    ECHEC: %s:%d \xe2\x80\x94 attendu: %.4f, obtenu: %.4f\n", \
               __FILE__, __LINE__, _a, _o); \
        return 0; \
    } \
} while(0)

#define ASSERT_EGAL_STR(attendu, obtenu) do { \
    const char *_a = (attendu), *_o = (obtenu); \
    if (strcmp(_a, _o) != 0) { \
        printf("    ECHEC: %s:%d \xe2\x80\x94 attendu: \"%s\", obtenu: \"%s\"\n", \
               __FILE__, __LINE__, _a, _o); \
        return 0; \
    } \
} while(0)

#define ASSERT_NULL(ptr) do { \
    if ((ptr) != NULL) { \
        printf("    ECHEC: %s:%d \xe2\x80\x94 attendu NULL\n", \
               __FILE__, __LINE__); \
        return 0; \
    } \
} while(0)

#define ASSERT_NON_NULL(ptr) do { \
    if ((ptr) == NULL) { \
        printf("    ECHEC: %s:%d \xe2\x80\x94 attendu non-NULL\n", \
               __FILE__, __LINE__); \
        return 0; \
    } \
} while(0)

static int resultat_tests(void) {
    return tests_echoues > 0 ? 1 : 0;
}

#endif
