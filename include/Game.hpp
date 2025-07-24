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
#include "ECS/System.hpp"
#include "world/systems/movement.hpp"
#include "world/systems/buildings.hpp"
#include "global.hpp"
#include "memory/allocators.hpp"

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
    ECS::Systems::SystemManager ecsRenderSystems;
    ECS::Systems::SystemManager ecsStateSystems;

    World::RenderSystems::RenderEntitySystem* renderEntitySys;
    World::Systems::DynamicEntitySystem* dynamicEntitySys;
    World::Systems::GunSystem* gunSys;

    void init(GameState* state, RenderContext* renderContext, Camera& camera);
};

struct Game {
    SDLContext sdlCtx;
    GameState* state = nullptr;
    Camera camera;
    CameraFocus cameraFocus;
    Gui gui;
    PlayerControls* playerControls = nullptr;
    MouseState lastUpdateMouseState;
    Vec2 lastUpdatePlayerTargetPos;
    DebugClass debug;
    MetadataTracker metadata;
    RenderContext* renderContext = nullptr;
    GameEntitySystems systems;
    Mode mode;
private:
    bool m_paused = false;
    using EssentialAllocator = ScratchAllocator<>;
public:
    EssentialAllocator essentialAllocator;
    BlockAllocator<2048, 4> blockAllocator;

    Game() {

    }

    int init(SDLContext sdlContext);

    int start();

    void quit();

    void destroy();

    int update();

    // @return 1 for quit game, 0 otherwise
    int handleEvent(const SDL_Event*);

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

    void setMode(Mode mode) {
        this->mode = mode;
    }

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

};

#endif