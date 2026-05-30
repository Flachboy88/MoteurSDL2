CC       = gcc
AR       = ar
LIBS_DIR = D:/Perso/programmes perso/Libs
ARCH     = i686-w64-mingw32

SDL2_DIR   = $(LIBS_DIR)/SDL2-2.30.11/$(ARCH)
SDL2_TTF   = $(LIBS_DIR)/SDL2_ttf-2.24.0/$(ARCH)
SDL2_IMAGE = $(LIBS_DIR)/SDL2_image-2.8.4/$(ARCH)
SDL2_MIXER = $(LIBS_DIR)/SDL2_mixer-2.8.2/$(ARCH)

INCLUDES = -Isrc -Iinclude \
           -I"$(SDL2_DIR)/include" \
           -I"$(SDL2_TTF)/include" \
           -I"$(SDL2_IMAGE)/include"\
		   -I"$(SDL2_MIXER)/include"

LDFLAGS  = -L"$(SDL2_DIR)/lib" \
           -L"$(SDL2_TTF)/lib" \
           -L"$(SDL2_IMAGE)/lib"\
		   -L"$(SDL2_MIXER)/lib"

LIBS     = -lmingw32 -lSDL2main -lSDL2 -lSDL2_ttf -lSDL2_image -lSDL2_mixer

CFLAGS   = -Wall -Wextra -std=c11 $(INCLUDES)

CORE_SRC    = src/core/core.c src/core/window.c src/core/timer.c src/core/input.c
ENGINE_SRC  = src/engine/state.c src/engine/entity.c src/engine/camera.c \
              src/engine/renderer.c src/engine/sprite.c src/engine/tilemap.c \
              src/engine/text.c src/engine/flags.c src/engine/save.c \
              src/engine/dialogue.c src/engine/xml.c src/engine/moteur.c

MOTEUR_SRC  = $(CORE_SRC) $(ENGINE_SRC)
MOTEUR_OBJ  = $(MOTEUR_SRC:.c=.o)

LIB_TARGET  = lib/libmoteur.a
EXE_TARGET  = bin/moteur.exe
TEST_TARGET = bin/tests.exe

.PHONY: all lib test run clean dlls

all: lib $(EXE_TARGET)

lib: $(LIB_TARGET)

$(LIB_TARGET): $(MOTEUR_OBJ)
	@mkdir -p lib
	$(AR) rcs $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(EXE_TARGET): main.c $(LIB_TARGET)
	$(CC) $(CFLAGS) $< -o $@ -Llib $(LDFLAGS) -lmoteur $(LIBS) -mwindows

$(TEST_TARGET): tests/main_tests.c $(LIB_TARGET)
	$(CC) $(CFLAGS) $< -o $@ -Llib $(LDFLAGS) -lmoteur $(LIBS)

test: $(TEST_TARGET)
	./$(TEST_TARGET)

run: all
	./$(EXE_TARGET)

dlls:
	cp "$(SDL2_DIR)/bin/SDL2.dll"       bin/
	cp "$(SDL2_TTF)/bin/SDL2_ttf.dll"   bin/
	cp "$(SDL2_IMAGE)/bin/SDL2_image.dll" bin/
	cp "$(SDL2_MIXER)/bin/SDL2_mixer.dll" bin/


clean:
	rm -f $(MOTEUR_OBJ) $(LIB_TARGET) $(EXE_TARGET) $(TEST_TARGET)
