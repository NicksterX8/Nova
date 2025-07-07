#ifndef UPDATE_INCLUDED
#define UPDATE_INCLUDED

#include <vector>
#include <variant>
#include <random>
#include <SDL3/SDL.h>
#include <math.h>

#include "constants.hpp"
#include "rendering/textures.hpp"
#include "GameState.hpp"
#include "PlayerControls.hpp"
#include "GUI/Gui.hpp"
#include "sdl.hpp"
#include "Camera.hpp"
#include "rendering/rendering.hpp"
#include "ECS/system.hpp"
#include "world/systems/movement.hpp"
#include "global.hpp"
#include "utils/allocators.hpp"

struct Game;

void rotateEntity(const EntityWorld& ecs, Entity entity, bool counterClockwise);

void setDefaultKeyBindings(Game& ctx, PlayerControls* controls);

struct CameraEntityFocus {
    Entity entity;
    bool rotateWithEntity;
};

struct CameraFocus {
    std::variant<
        CameraEntityFocus, // follow entity
        Vec2 // static point
    > focus;
};

namespace World {
    namespace RenderSystems {
        struct RenderEntitySystem;
    }
}

struct GameEntitySystems {
    ECS::System::SystemManager ecsRenderSystems;
    ECS::System::SystemManager ecsStateSystems;

    World::RenderSystems::RenderEntitySystem* renderEntitySys;
    World::Systems::DynamicEntitySystem* dynamicEntitySys;

    void init(GameState* state, RenderContext* renderContext, Camera& camera);
};

struct Game {
    SDLContext sdlCtx;
    GameState* state;
    Camera camera;
    CameraFocus cameraFocus;
    Gui* gui;
    PlayerControls* playerControls;
    MouseState lastUpdateMouseState;
    Vec2 lastUpdatePlayerTargetPos;
    DebugClass* debug;
    MetadataTracker metadata;
    RenderContext* renderContext;
    GameEntitySystems systems;
    Mode mode;
private:
    bool m_paused = false;
    using EssentialAllocator = ScratchAllocator<Mallocator>;
public:
    EssentialAllocator essentialAllocator;
    BlockAllocator<2048, 4> blockAllocator;

    bool isPaused() const {
        return m_paused;
    }

    void pause() {
        m_paused = true;
        Global.paused = true;
    }

    void resume() {
        m_paused = false;
        Global.paused = false;
    }

    Game(SDLContext sdlContext):
    sdlCtx(sdlContext), metadata(TARGET_FPS, TICKS_PER_SECOND, ENABLE_VSYNC),
    essentialAllocator((1ULL << 15), &mallocator), blockAllocator(&mallocator) {
        Metadata = &metadata;

        trackAllocator("Essential Allocator", &essentialAllocator);
        trackAllocator("Medium allocator", &blockAllocator);

        debug = New(DebugClass(), &blockAllocator);
        Debug = debug;
        debug->debugging = 1;

        state = NULL;
        gui = NULL;
        playerControls = NULL;
        renderContext = NULL;
        mode = Unstarted;
    }

    int init();

    int start();

    void quit();

    void destroy();

    int update();

    // @return 1 for quit game, 0 otherwise
    int handleEvent(const SDL_Event*);

#ifdef EMSCRIPTEN
    // update wrapper function to unwrap the void pointer main loop parameter into its properties
    static int emscriptenUpdateWrapper(void* param) {
        Game* game = static_cast<Game*>(param);
        if (game) {
            return game->update();
        } else {
            // handle error
            LogError("Parameter passed to emscripten update wrapper was null!");
            return -1;
        }
        
    }
#endif

    void setMode(Mode mode) {
        this->mode = mode;
    }

};

#endif