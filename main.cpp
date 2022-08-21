#include "constants.hpp"
#include "Log.hpp"

#include <vector>
#include <stdio.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include "gl.h"

#include "SDL2_gfx/SDL2_gfxPrimitives.h"
#include "SDL_FontCache/SDL_FontCache.h"

#include "NC/SDLContext.h"
#include "NC/SDLBuild.h"
#include "NC/colors.h"
#include "NC/utils.h"
#include "NC/cpp-vectors.hpp"

#include "Textures.hpp"
#include "GameState.hpp"
#include "PlayerControls.hpp"
#include "GameViewport.hpp"
#include "GUI.hpp"
#include "Debug.hpp"
#include "loadData.hpp"

#include "EntityComponents/Components.hpp"
#include "Entities/Entities.hpp"
#include "EntitySystems/Rendering.hpp"
#include "EntitySystems/Systems.hpp"
#include "update.hpp"
#include "Debug.hpp"
#include "Items.hpp"
#include "GameSave/main.hpp"
#include "sdl.hpp"

#ifdef DEBUG
    //#include "Testing.hpp"
#endif

// load assets and stuff
int load(SDL_Renderer *ren, float renderScale = 1.0f) {
    FreeSans = FC_CreateFont();
    FC_LoadFont(FreeSans, ren, "assets/fonts/FreeSans.ttf", 36, SDL_BLACK, TTF_STYLE_NORMAL);

    Textures.load(ren);
    
    return 0;
}

// unload assets and stuff
void unload() {
    FC_FreeFont(FreeSans);

    Textures.unload();
}

void setDebugSettings(DebugClass& debug) {
    // shorten name for easier typing while keeping default settings
    DebugSettings& ds = debug.settings;

    ds.drawChunkBorders = false;
    ds.drawChunkCoordinates = false;
    ds.drawEntityRects = false;
    ds.drawEntityIDs = false;
}

void placeInserter(ChunkMap& chunkmap, EntityWorld* ecs, Vec2 mouseWorldPos) {
    Tile* tile = getTileAtPosition(chunkmap, mouseWorldPos);
    if (tile && !tile->entity.Exists(ecs)) {
        Vec2 inputPos = {mouseWorldPos.x + 1, mouseWorldPos.y};
        //Tile* inputTile = getTileAtPosition(chunkmap, inputPos);
        Vec2 outputPos = {mouseWorldPos.x - 1, mouseWorldPos.y};
        //Tile* outputTile = getTileAtPosition(chunkmap, outputPos);
        Entity inserter = Entities::Inserter(ecs, mouseWorldPos.vfloor() + Vec2(0.5f, 0.5f), 1, inputPos.floorToIVec(), outputPos.floorToIVec());
        placeEntityOnTile(ecs, tile, inserter);
    }
}

void rotateEntity(const ComponentManager<EC::Rotation, EC::Rotatable>& ecs, EntityT<EC::Rotation, EC::Rotatable> entity, bool counterClockwise) {
    float* rotation = &entity.Get<EC::Rotation>(&ecs)->degrees;
    auto rotatable = entity.Get<EC::Rotatable>(&ecs);
    // left shift switches direction
    if (counterClockwise) {
        *rotation -= rotatable->increment;
    } else {
        *rotation += rotatable->increment;
    }
    rotatable->rotated = true;
}

void setDefaultKeyBindings(Context& ctx, PlayerControls* controls) {
    GameState& state = *ctx.state;
    Player& player = state.player;
    EntityWorld& ecs = state.ecs;
    ChunkMap& chunkmap = state.chunkmap;
    GameViewport& gameViewport = *ctx.gameViewport;
    PlayerControls& playerControls = *ctx.playerControls;
    DebugClass& debug = ctx.debug;

    controls->addKeyBinding(new FunctionKeyBinding('y',
    [&ecs, &gameViewport, &state, &playerControls](){
        auto mouse = playerControls.getMouse();
        Entity zombie = Entities::Enemy(
            &ecs,
            gameViewport.pixelToWorldPosition(mouse.x, mouse.y),
            state.player.entity
        );
    }));

    KeyBinding* keyBindings[] = {
        new ToggleKeyBinding('b', &debug.settings.drawChunkBorders),
        new ToggleKeyBinding('u', &debug.settings.drawEntityRects),
        new ToggleKeyBinding('c', &debug.settings.drawChunkCoordinates),
        new ToggleKeyBinding('[', &debug.settings.drawChunkEntityCount),

        new FunctionKeyBinding('q', [&player](){
            player.releaseHeldItem();
        }),
        new FunctionKeyBinding('c', [&ecs, &gameViewport](){
            int width = 2;
            int height = 1;
            Vec2 position = getMouseWorldPosition(gameViewport).vfloor() + Vec2(width/2.0f, height/2.0f);
            auto chest = Entities::Chest(&ecs, position, 3, width, height);

        }),
        new FunctionKeyBinding('l', [&ecs](){
            // do airstrikes row by row
            for (int y = -100; y < 100; y += 5) {
                for (int x = -100; x < 100; x += 5) {
                    auto airstrike = Entities::Airstrike(&ecs, Vec2(x, y * 2), {3.0f, 3.0f}, Vec2(x, y));
                }
            }
        }),
        new FunctionKeyBinding('i', [&ecs, &gameViewport, &chunkmap](){
            placeInserter(chunkmap, &ecs, getMouseWorldPosition(gameViewport));
        }),
        new FunctionKeyBinding('r', [&gameViewport, &playerControls, &ecs, &chunkmap](){
            //player.findFocusedEntity(getMouseWorldPosition(gameViewport));
            auto focusedEntity = findPlayerFocusedEntity(ecs, chunkmap, getMouseWorldPosition(gameViewport));
            if (focusedEntity.Has<EC::Rotation, EC::Rotatable>(&ecs))
                rotateEntity(ComponentManager<EC::Rotation, EC::Rotatable, EC::Position>(&ecs), focusedEntity.cast<EC::Rotation, EC::Rotatable>(), playerControls.keyboardState[SDL_SCANCODE_LSHIFT]);
        }),
        new FunctionKeyBinding('5', [&](){
            const Entity* entities = ecs.GetEntityList();
            for (int i = ecs.EntityCount()-1; i >= 0; i--) {
                if (entities[i].id != 0)
                    ecs.Destroy(entities[i]);
            }
        })

    };

    for (size_t i = 0; i < sizeof(keyBindings) / sizeof(KeyBinding*); i++) {
        controls->addKeyBinding(keyBindings[i]);
    }
}

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)
#define AT __FILE__ ":" TOSTRING(__LINE__)

void logEntitySystemInfo() {
    Log("Number of systems: %d", NUM_SYSTEMS);
    const char* systemsStr = TOSTRING((SYSTEMS));
    int length = strlen(systemsStr);
    char systemsStr2[length];
    strcpy(systemsStr2, systemsStr+1);
    systemsStr2[length-2] = '\0';

    char (systemsList[NUM_SYSTEMS])[256];
    int charIndex = 0;
    for (int i = 0; i < NUM_SYSTEMS; i++) {
        const char* start = &systemsStr2[charIndex];
        const char* chr = start;
        int size = 0;
        while (true) {
            if (*chr == ',') {
                break;
            }
            if (*chr == '\0') {
                // Log("failed to find comma");
                break;
            }
            chr++;
            size++;
        }
        strncpy(systemsList[i], start, size+1);
        systemsList[i][size] = '\0';
        charIndex += (size + 2);
    }

    for (int i = 0; i < NUM_SYSTEMS; i++) {
        Log("ID: %d, System: %s.", i, systemsList[i]);
    }
}

void logEntityComponentInfo() {
    const char* componentsStr = TOSTRING((COMPONENT_NAMES));
    int length = strlen(componentsStr);
    char componentsStr2[length];
    strcpy(componentsStr2, componentsStr+1);
    componentsStr2[length-2] = '\0';

    char* buffer = (char*)malloc(sizeof(char) * 256 * NUM_COMPONENTS);

    char* componentsList;
    componentsList = buffer;
    int charIndex = 0;
    for (int i = 0; i < NUM_COMPONENTS; i++) {
        const char* start = &componentsStr2[charIndex];
        const char* chr = start;
        int size = 0;
        while (true) {
            if (*chr == ',') {
                break;
            }
            if (*chr == '\0') {
                // Log("failed to find comma");
                break;
            }
            chr++;
            size++;
        }
        strlcpy(&componentsList[i * 256], start, size+1);
        componentsList[i * 256 + size] = '\0';
        charIndex += (size + 2);
    }

    for (int i = 0; i < NUM_COMPONENTS; i++) {
        Log("ID: %d, Component: %s.", i, &componentsList[i * 256]);
        componentNames[i] = &componentsList[i * 256];
    }
}

void initLogging() {
    Log.init("save/log.txt");
    SDL_LogSetOutputFunction(Logger::logOutputFunction, &Log);
    SDL_LogSetAllPriority(SDL_LOG_PRIORITY_WARN);
    SDL_LogSetPriority(LOG_CATEGORY_MAIN, SDL_LOG_PRIORITY_INFO);
}

int main(int argc, char** argv) {
    initLogging();

    DebugClass debug;
    setDebugSettings(debug);
    Debug = &debug;

    MetadataTracker metadata = MetadataTracker(TARGET_FPS, ENABLE_VSYNC);
    Metadata = &metadata;

    Log.useEscapeCodes = true;
    for (int i = 0; i < argc; i++) {
        if (strcmp(argv[i], "--no-color-codes") == 0) {
            Log.useEscapeCodes = false;
        }
    }

    SDLContext sdlCtx = initSDL();

    logEntitySystemInfo();
    logEntityComponentInfo();

    int screenWidth,screenHeight;
    SDL_GetRendererOutputSize(sdlCtx.ren, &screenWidth, &screenHeight);

    load(sdlCtx.ren, sdlCtx.scale);
    loadTileData();
    loadItemData();

    GameViewport gameViewport;

    GameState* state = new GameState();
    
    state->init(sdlCtx.ren, &gameViewport);

    for (int e = 0; e < 20000; e++) {
        Vec2 pos = {(float)randomInt(-200, 200), (float)randomInt(-200, 200)};
        auto tree = Entities::Tree(&state->ecs, pos, {1, 1});
    }

    gameViewport = newGameViewport(screenWidth, screenHeight, state->player.getPosition().x, state->player.getPosition().y);
    float worldScale = 1.0f;
    SDL_Point mousePos = SDLGetMousePixelPosition();
    MouseState lastUpdateMouseState = {
        mousePos.x,
        mousePos.y,
        SDLGetMouseButtons()
    };
    Vec2 lastUpdatePlayerTargetPos = gameViewport.pixelToWorldPosition(mousePos.x, mousePos.y);
    GUI gui;
    PlayerControls playerControls = PlayerControls(gameViewport); // player related event handler
    Context context = Context(
        sdlCtx,
        state,
        &gui,
        playerControls,
        &gameViewport,
        worldScale,
        lastUpdateMouseState,
        lastUpdatePlayerTargetPos,
        debug,
        metadata
    );

    setDefaultKeyBindings(context, &playerControls);

    if (metadata.vsyncEnabled) {
        metadata.setTargetFps(60);
    } else {
        metadata.setTargetFps(TARGET_FPS);
    }
    metadata.start();
    NC_SetMainLoop(updateWrapper, &context);
    double secondsElapsed = metadata.end();
    Log("Time elapsed: %.1f", secondsElapsed);

    // GameSave::save(state);

    unload();

    delete state;

    Log.destroy();

    quitSDL(&sdlCtx);

    return 0;
}