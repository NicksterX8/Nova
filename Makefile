CXX := g++
EXT := cpp

CXXFLAGS := -g -Wall -Wextra -std=c++14 -MMD -MP -O3 \
	-Wno-missing-field-initializers -Wno-unused-variable -Wno-unused-parameter -Wno-unused-private-field

LIBS = SDL2_gfx/SDL2_gfx.a SDL_FontCache/SDL_FontCache.o NC/utils.o NC/vectors.o \
	NC/SDLContext.o
EMSC_LIBS = NC/utils.emsc.o NC/vectors.emsc.o NC/SDLContext.emsc.o SDL2_gfx/SDL2_gfx.emsc.a SDL_FontCache/SDL_FontCache.emsc.o

INCLUDES = -I /usr/local/Cellar/sdl2/2.0.16/include \
 -I /usr/local/Cellar/sdl2_image/2.0.5/include

LINKS := -L /usr/local/Cellar/sdl2_image/2.0.5/lib
LINK_FLAGS := -lSDL2 -lSDL2_image -lSDL2_ttf
# -L /usr/local/Cellar/sdl2/2.0.16/lib makes a warning for some reason (not found)
ARGS =

MAINFILE := main.cpp

FILES = main update GameSave/main Entities/Methods Chunks PlayerControls \
	GUI Items ECS/EntitySystemInterface ECS/EntityManager ECS/EntitySystem ECS/ECS \
	Entities/Entities ECS/Entity EntityComponents/Components Textures Tiles GameViewport \
	Log Debug Metadata GameState Rendering/Drawing SECS/ECS

SRC_FILES = $(FILES:=.$(EXT))
OBJ_FILES = $(FILES:=.o)
DEPEND_FILES = $(FILES:=.d)
	
EMSC_OBJ_FILES = $(FILES:=.emsc.o)

APP := exe

#.c.o: $(SRC_FILES)
#	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $^

#all: $(OBJ_FILES)
#	$(CXX) $(CXXFLAGS) $(INCLUDES) $(LIBS) main.cpp -o $(APP) $(LINKS) $(OBJ_FILES) $(LINK_FLAGS)

all: exe

exe: $(OBJ_FILES)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(LIBS) $^ -o $@ $(LINKS) $(LINK_FLAGS)

#do:
#	rm -f Entities/EntityManager.o
#	rm -f Entities/ECS.o
#	rm -f Entities/ComponentPool.o

clean:
	rm -rf $(OBJ_FILES) $(APP)
	rm -f build/game.html
	rm -f build/game.js
	rm -f build/game.wasm

-include ($(DEPEND_FILES))

%.o: %.cpp Makefile
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

%.emsc.o: %.cpp
	 em++  $(CXXFLAGS) $(INCLUDES) -c $< -o $@
# cd "/Users/melaniewilson/Faketorio/ComponentMetadata/" && g++ -std=c++17 parse.cpp -o parse && "/Users/melaniewilson/Faketorio/ComponentMetadata/"parse

compile:
	rm -f main.o
	make

compileWeb:


deploy:
	make compile
	./exe

buildNC:
	(cd NC && make all)

sourceEMCC:
	cd ~/emsdk && ./emsdk activate

web: emscripten

emscripten: $(EMSC_OBJ_FILES)
	em++ -DBUILD_EMSCRIPTEN $^ $(EMSC_LIBS) \
	-o build/game.html \
	$(INCLUDES) \
 	-I /usr/local/Cellar/emscripten/include \
	-s LLD_REPORT_UNDEFINED $(LINK_FLAGS) \
	--preload-file assets \
	-Wall -g -lm -s \
	USE_SDL=2 -s USE_SDL_IMAGE=2 -s USE_SDL_TTF=2 -s SDL2_IMAGE_FORMATS='["jpg","png"]' \
	-s ALLOW_MEMORY_GROWTH=1 \
	--use-preload-plugins