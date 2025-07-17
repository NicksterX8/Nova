#include "Game.hpp"

#include <thread>

#include "utils/Log.hpp"
#include "utils/Debug.hpp"
#include "utils/random.hpp"
#include "GUI/Gui.hpp"
#include "PlayerControls.hpp"
#include "rendering/rendering.hpp"
#include "GameSave/main.hpp"
#include "commands.hpp"

#include "ECS/ECS.hpp"
#include "world/entities/entities.hpp"
#include "world/components/components.hpp"
#include "ECS/ArchetypePool.hpp"
#include "rendering/systems/new.hpp"

#include "ADT/SmallVector.hpp"
#include "ADT/ArrayRef.hpp"

void setDefaultKeyBindings(Game& ctx, PlayerControls* controls) {
    GameState& state = *ctx.state;
    Player& player = state.player;
    EntityWorld& ecs = *state.ecs;
    ChunkMap& chunkmap = state.chunkmap;
    Camera& camera = ctx.camera;
    PlayerControls& playerControls = *ctx.playerControls;
    Vec2& mouseWorldPos = playerControls.mouseWorldPos;
    DebugClass& debug = ctx.debug;
    ItemManager& itemManager = state.itemManager;

    KeyBinding* keyBindings[] = {
        new FunctionKeyBinding('u', [&](){
            debug.settings["drawEntityViewBoxes"] = !debug.settings["drawEntityViewBoxes"];
        }),
        new FunctionKeyBinding('p', [&](){
            debug.settings["drawEntityCollisionBoxes"] = !debug.settings["drawEntityCollisionBoxes"];
        })
    };

    for (size_t i = 0; i < sizeof(keyBindings) / sizeof(KeyBinding*); i++) {
        controls->addKeyBinding(keyBindings[i]);
    }
}

void GameEntitySystems::init(GameState* state, RenderContext* renderContext, Camera& camera) {
    auto& allocator = GlobalAllocators.gameScratchAllocator;
    this->renderEntitySys = NEW(World::RenderSystems::RenderEntitySystem(ecsRenderSystems, *renderContext, camera, *state->ecs, state->chunkmap), allocator);
    this->dynamicEntitySys = NEW(World::Systems::DynamicEntitySystem(ecsStateSystems, &state->chunkmap), allocator);

    ECS::System::setupSystems(ecsRenderSystems);
    ECS::System::setupSystems(ecsStateSystems);
}

void updateDynamicEntityChunkPositions(EntityWorld& ecs, GameState* state) {
    namespace EC = World::EC;
    
    ecs.ForEach([](ECS::Signature components){
        return components.hasAll(ECS::getSignature<EC::Position, EC::Dynamic, EC::ViewBox>());
    }, [&](auto entity){
        //auto* viewbox  = ecs.Get<EC::ViewBox>(entity);
        auto* positionEc = ecs.Get<EC::Position>(entity);
        auto* dynamicEc = ecs.Get<EC::Dynamic>(entity);

        Vec2 oldPos = positionEc->vec2();
        Vec2 newPos = dynamicEc->pos;
        if (oldPos.x == newPos.x && oldPos.y == newPos.y) {
            return;
        }

        positionEc->x = newPos.x;
        positionEc->y = newPos.y;

        World::entityPositionChanged(state, entity, oldPos);
    });
}

static void updateSystems(GameState* state) {
    EntityWorld& ecs = *state->ecs;
    auto& chunkmap = state->chunkmap;

    namespace EC = World::EC;
    ECS::System::executeSystems(*state->ecsSystems);

    ecs.ForEach([](ECS::Signature components){
        return components[EC::Follow::ID] && components[EC::CollisionBox::ID] && components[EC::Dynamic::ID] && components[EC::Position::ID]; 
    }, [&](Entity entity){
        auto followComponent = ecs.Get<EC::Follow>(entity);
        assert(followComponent);
        if (!ecs.EntityExists(followComponent->entity)) {
            return;
        }
        Entity following = followComponent->entity;
        Vec2 followingPos = ecs.Get<EC::Position>(following)->vec2();
        Box followingCollision = ecs.Get<EC::CollisionBox>(following)->box;
        Vec2 target = followingPos + followingCollision.center();

        auto* position = ecs.Get<EC::Dynamic>(entity);
        Box collision = ecs.Get<EC::CollisionBox>(entity)->box;
        Vec2 center = position->pos + collision.center();;
        Vec2 delta = {target.x - center.x, target.y - center.y};

        if (delta.x*delta.x + delta.y*delta.y < followComponent->speed * followComponent->speed) {
            //Vec2 oldPos = position->vec2();
            position->pos += delta;
            //entityPositionChanged(&state->chunkmap, &ecs, entity, oldPos);

            // do something
            // hurt them if they have health
            if (ecs.EntityHas<EC::Health>(following)) {
                ecs.Get<EC::Health>(following)->damage(10);
            }

        } else {
            Vec2 unit;
            // normalized vector with x = 0.0 is NaN
            if (delta.x == 0.0f) {
                unit = Vec2{0.0f, ((delta.y > 0.0f) ? 1 : -1) * followComponent->speed};
            } else {
                unit = glm::normalize(delta) * followComponent->speed;
            }
            
            /*
            Vec2 oldPos = position->vec2();
            position->x += unit.x;
            position->y += unit.y;
            */
            position->pos += unit;
            //entityPositionChanged(&state->chunkmap, &ecs, entity, oldPos);

            float rotationRadians = atan2f(delta.y, delta.x);
            ecs.Set<EC::Rotation>(entity, {glm::degrees(rotationRadians) - 90.0f});
        }
    });

    ECS::EntityCommandBuffer destroyEntities;
    ecs.useCommandBuffer(&destroyEntities);
    ecs.ForEach([](ECS::Signature components){
       return components[EC::Health::ID]; 
    }, [&](Entity entity){
        auto* health = ecs.Get<EC::Health>(entity);
        // Must do check like this instead of (*health <= 0.0f) to account for NaN values,
        // which can occur when infinite damage is done to an entity with infinite health
        // so in that situation the infinite damage wins out, rather than the infinte health
        if (!(health->health > 0.0f)) {
            if (!ecs.EntityHas<EC::Immortal>(entity)) {
                ecs.Destroy(entity);
            }
        }

        if (health->iFrames > 0) {
            health->iFrames--;
        }
    });
    ecs.flushCurrentCommandBuffer();

    ecs.ForEach([](ECS::Signature components){
        return components[EC::Dynamic::ID] && components[EC::Motion::ID];
    }, [&](auto entity){
        auto* pos = ecs.Get<EC::Dynamic>(entity);
        Vec2 oldPos = pos->pos;
        auto* motion = ecs.Get<EC::Motion>(entity);
        Vec2 target = motion->target;
        Vec2 delta = target - oldPos;
        float speed = motion->speed;
        Vec2 unit = glm::normalize(delta) * speed;

        float dist = delta.x*delta.x + delta.y*delta.y;

        if (dist < speed*speed) {
            pos->pos.x = target.x;
            pos->pos.y = target.y;
        } else {
            pos->pos.x += unit.x;
            pos->pos.y += unit.y;
        }

        //entityPositionChanged(state, entity, oldPos);
    });

    ecs.ForEach([](ECS::Signature components){
        return components.hasComponents<EC::Fresh, EC::Position>();
    }, [&](Entity entity){
        auto fresh = ecs.Get<EC::Fresh>(entity); assert(fresh);
        if (fresh->components.getComponent<EC::Position>()) {
            // ec::position just added
            fresh->components.setComponent<EC::Position>(0);
        }
    });

    ECS::EntityCommandBuffer removeFreshes;
    ecs.useCommandBuffer(&removeFreshes);
    ecs.ForEach([](ECS::Signature components){
        return components.hasComponents<EC::Fresh>();
    }, [&](Entity entity){
        ecs.Remove<EC::Fresh>(entity);
    });
    ecs.flushCurrentCommandBuffer();
}

int tick(GameState* state, PlayerControls* playerControls) {
    state->player.grenadeThrowCooldown--;

    updateSystems(state);

    playerControls->doPlayerMovementTick();

    //updateDynamicEntityChunkPositions(*state->ecs, state);

    return 0;
}

void displaySizeChanged(SDL_Window* window, const RenderContext& renderContext, Camera* camera) {
    int drawableWidth,drawableHeight;
    SDL_GetWindowSizeInPixels(window, &drawableWidth, &drawableHeight);
    glViewport(0, 0, drawableWidth, drawableHeight);
    camera->pixelWidth = (float)drawableWidth;
    camera->pixelHeight = (float)drawableHeight;
    camera->displayViewport.w = drawableWidth;
    camera->displayViewport.h = drawableHeight;

    resizeRenderBuffer(renderContext.framebuffer, {drawableWidth, drawableHeight});
}

int Game::handleEvent(const SDL_Event* event) {
    int code = 0;
    switch(event->type) {
        case SDL_EVENT_QUIT:
            code = 1; 
            break;
        case SDL_EVENT_WINDOW_DISPLAY_CHANGED: {
            if (event->window.windowID == SDL_GetWindowID(sdlCtx.primary.window)) {
                float oldPixelScale = SDL::pixelScale;
                SDL::pixelScale = SDL::getPixelScale(sdlCtx.primary.window);
                camera.baseScale = BASE_UNIT_SCALE * SDL::pixelScale;
                float pixelScaleChange = SDL::pixelScale / oldPixelScale;
                //scaleAllFonts(renderContext->fonts, pixelScaleChange);
                displaySizeChanged(sdlCtx.primary.window, *renderContext, &camera);
            }
        break;}
        case SDL_EVENT_WINDOW_RESIZED:
            if (event->window.windowID == SDL_GetWindowID(sdlCtx.primary.window)) {
                displaySizeChanged(sdlCtx.primary.window, *renderContext, &camera);
            }
        break;
        case SDL_EVENT_MOUSE_WHEEL: {
            float wheelMovement = event->wheel.y;
            if (wheelMovement != 0.0f) {
                const float min = 1.0f / 16.0f;
                const float max = 32.0f;
                const float logMin = log2(min);
                const float logMax = log2(max);

                const float wheelMax = 32.0f;

                static float wheelPos = 16.0f;
                wheelPos += wheelMovement;

                if (wheelPos < 1.0f) {
                    wheelPos = 1.0f;
                } else if (wheelPos > wheelMax) {
                    wheelPos = wheelMax;
                }

                float logZoom = ((logMax - logMin) / (wheelMax)) * wheelPos + logMin;
                float zoom = pow(2, logZoom);
                
                camera.zoom = zoom;
            }
        break;}    
        case SDL_EVENT_KEY_DOWN: {
            switch (event->key.key) {
                
            }
        break;}
        case SDL_EVENT_GAMEPAD_ADDED: {
            int joystickIndex = event->jdevice.which;
            LogInfo("Controller device added. Joystick index: %d", joystickIndex);
            playerControls->connectController();
        break;}
        case SDL_EVENT_GAMEPAD_REMOVED: {
            int instanceID = event->jdevice.which;
            LogInfo("Controller device removed. Instance id: %d", instanceID);
            playerControls->controller.disconnect();
        break;}
        case SDL_EVENT_GAMEPAD_BUTTON_DOWN: {
            Uint8 buttonPressed = event->gbutton.button;
            if (buttonPressed == SDL_GAMEPAD_BUTTON_LABEL_A) {
                playerControls->movePlayer(playerControls->mouseWorldPos - state->player.get<World::EC::Position>()->vec2());
            }
        break;}
        default:
        break;
    }

    auto actions = playerControls->handleEvent(event);
    for (auto& action : actions) {
        action.function(this);
    }
    return code;
}

void focusCameraOnEntity(Camera* camera, CameraEntityFocus entityFocus, const EntityWorld* ecs) {
    Entity entity = entityFocus.entity;
    auto* viewbox = ecs->Get<World::EC::ViewBox>(entity);
    auto* pos = ecs->Get<World::EC::Position>(entity);
    Vec2 focus = {0, 0};
    if (pos) {
        focus = pos->vec2();
        // if viewbox, focus on the center of the viewbox. otherwise just use position only
        if (viewbox) {
            focus += viewbox->box.center(); // focus on player center
        }
    }

    camera->position.x = focus.x;
    camera->position.y = focus.y;
    assert(isValidEntityPosition(camera->position));

    auto* rotation = ecs->Get<World::EC::Rotation>(entity);
    if (entityFocus.rotateWithEntity && rotation) {
        camera->setAngle(rotation->degrees);
    } else {
        camera->setAngle(0.0f);
    }
    // in non debug maybe just set the position to 0,0
}

void focusCamera(Camera* camera, const CameraFocus* focus, const EntityWorld* ecs) {
    if (std::holds_alternative<CameraEntityFocus>(focus->focus)) {
        auto entityFocus = std::get<CameraEntityFocus>(focus->focus);
        focusCameraOnEntity(camera, entityFocus, ecs);
    } else if (std::holds_alternative<Vec2>(focus->focus)) {
        Vec2 point = std::get<Vec2>(focus->focus);
        camera->position.x = point.x;
        camera->position.y = point.y;
        camera->setAngle(0.0f);
    }
}

int Game::update() {

    DO_ONCE(LogInfo("Whats up?"));
    
    double targetTPS = (double)TICKS_PER_SECOND;
    double fixedFrametime = 1000.0 / targetTPS;
    constexpr int maxTicks = 3; // per frame

    bool quit = false;

    // get user input state for this update
    SDL_PumpEvents();
    MouseState mouse = getMouseState();
    // handle events //
    playerControls->updateState();

    SDL_Event event;
    while (SDL_PollEvent(&event) != 0) {
        int code = handleEvent(&event);
        if (code == 1) quit = true;
    }

    playerControls->update();
    if (playerControls->mouseWorldPos != lastUpdatePlayerTargetPos) {
        playerControls->playerMouseTargetMoved(mouse, lastUpdateMouseState);
    }

    auto frame = metadata.newFrame();
    double deltaTime = metadata.frame.deltaTime;

    static double remainingTime = 0.0;

    if (!isPaused()) {
        remainingTime += deltaTime;
        
        int ticksThisFrame = 0;
        while (remainingTime > fixedFrametime) {
            remainingTime -= fixedFrametime;

            if (ticksThisFrame++ < maxTicks) {
                Tick currentTick = metadata.newTick();
                tick(state, playerControls);
            }
        }

        // update to new state from tick
        focusCamera(&camera, &cameraFocus, state->ecs);
    }

    float scale = SDL::pixelScale;
    RenderOptions options = {
        {camera.pixelWidth, camera.pixelHeight},
        scale
    };

    std::vector<GameAction> guiActions = gui.updateGuiState(state, *playerControls);
    for (auto& action : guiActions) {
        action.function(this);
    }

    render(*renderContext, options, &gui, state, camera, *playerControls, mode, true); 

    lastUpdateMouseState = mouse;
    lastUpdatePlayerTargetPos = playerControls->mouseWorldPos;

    if (quit) {
        LogInfo("Returning from main update loop.");
        return 1;
    }
    return 0; 
}

inline void runJob(const ECS::EntityManager& components, Job job, Group* group, void* args) {
    std::vector<ECS::ArchetypePool*> eligiblePools;
    int groupSize = findEligiblePools(group->group, components, &eligiblePools);

    // for each group, make sure array is big enough to fit one element per entity
    for (auto& array : group->arrays) {
        void*& data = *array.ptrToData;
        data = realloc(data, array.typeSize * groupSize);
    }

    JobDataNew jobData;
    jobData.dependencies = job.dependencies;
    jobData.indexBegin = 0;
    jobData.groupVars = args;
    

    
    if (eligiblePools.empty()) return;
    int index = 0;
    for (auto* pool : eligiblePools) {
        jobData.pool = pool;
        job.executeFunc(&jobData, index, index + pool->size);
        index += pool->size;
    }
}

inline void testJobs(Game* game) {
    using namespace GUI;
    SystemManager& manager = game->systems.ecsRenderSystems;
    World::NewEntityViewBoxSystem newsystem{manager, nullptr};
    for (auto& job : newsystem.jobs) {
        runJob(game->state->ecs->em, job.job, job.group, job.args);
    }
}

int Game::init(SDLContext sdlContext) {
    this->sdlCtx = sdlContext;

    LogInfo("Game init start");
    this->mode = Unstarted;

    this->debug.debugging = true;
    this->debug.console = &this->gui.console;
    Debug = &this->debug;
    
    this->renderContext = NEW(RenderContext(sdlCtx.primary.window, sdlCtx.primary.glContext), essentialAllocator);
    LogInfo("starting render init");
    renderInit(*renderContext);

    this->gui.init(renderContext->guiRenderer, this);

    this->state = NEW(GameState(), essentialAllocator);
    this->state->init(&renderContext->textures);

    this->systems.ecsRenderSystems.entityManager = &this->state->ecs->em;
    this->systems.ecsStateSystems.entityManager  = &this->state->ecs->em;
    renderContext->ecsRenderSystems = &this->systems.ecsRenderSystems;
    state->ecsSystems = &this->systems.ecsStateSystems;

    // init systems
    systems.init(state, renderContext, camera);

    for (int e = 0; e < 200; e++) {
        Vec2 pos = {(float)randomInt(-400, 400), (float)randomInt(-400, 400)};
        // do placing collision checks
        auto tree = World::Entities::Tree::make(state->ecs, pos, {4, 4}).make();
        World::entityCreated(state, tree);
        (void)tree;
    }

    auto tree = World::Entities::Tree::make(state->ecs, {10.5, 10.5}, {40, 40})();
    state->ecs->Add<World::EC::Fresh, World::EC::Immortal>(tree, {}, {});

    for (int i = 0; i < 10; i++) {
        auto& tree = World::Entities::Grenade::make(state->ecs, Vec2(i*2));
        state->ecs->Get<World::EC::Render>(tree())->textures[1].layer = i;
        state->ecs->Get<World::EC::Render>(tree())->textures[0].layer = i;
    }

    testJobs(this);

    state->ecs->Destroy(tree);

    World::Entities::TextBox::make(state->ecs, {10, -5}, World::EC::Text{
        .message = "This is an example of a text box!",
        .rendering = TextRenderingSettings{
            .color = {0, 255, 55, 255},
            .font = Fonts->get("Debug")
        },
        .formatting = TextFormattingSettings{
            .align = TextAlignment::TopCenter
        },
    });

    this->playerControls = NEW(PlayerControls(this), essentialAllocator);
    SDL_FPoint mousePos = SDL::getMousePixelPosition();
    this->lastUpdateMouseState = {
        .position = {mousePos.x, mousePos.y},
        .buttons = SDL::getMouseButtons()
    };

    setDefaultKeyBindings(*this, playerControls);

    int screenWidth,screenHeight;
    SDL_GetWindowSizeInPixels(sdlCtx.primary.window, &screenWidth, &screenHeight);

    glViewport(0, 0, screenWidth, screenHeight);
    this->camera = Camera(BASE_UNIT_SCALE, glm::vec3(0.0f), screenWidth, screenHeight);
    this->cameraFocus = {CameraEntityFocus{this->state->player.entity, true}};

    setCommands(this);

    LogInfo("Game init end");

    constexpr SDL_Color red = {255,0,0,255};
    constexpr SDL_Color green = {0,255,0,255};
    constexpr SDL_Color blue = {0,0,255,255};
    SDL_Color colors[] = {
        red, green, blue, red, blue, blue, green, green, green, red, blue, green, blue
    };
    gui.console.newMessage("This is crazy", GUI::Console::MessageType::Error, colors);

    return 0;
}

void Game::quit() {
    LogInfo("Quitting!");
    mode = Quit;

    renderQuit(*renderContext);

    double secondsElapsed = metadata.end();
    LogInfo("Quit. Time elapsed: %.1f seconds", secondsElapsed);
}

void Game::destroy() {
    LogInfo("destroying game");
    Debug = nullptr;
}

int Game::start() {
    // GameSave::load()

    LogInfo("Starting!");
    mode = Playing;

    if (metadata.vsyncEnabled) {
        metadata.frame.targetUpdatesPerSecond = 60.0f;
    } else {
        metadata.frame.targetUpdatesPerSecond = TARGET_FPS;
    }
    metadata.start();
#ifdef EMSCRIPTEN
    emscripten_set_main_loop_arg(Game::emscriptenUpdateWrapper, this, 0, 1);
#else
    int code;
    do {
        code = update();
    } while (code == 0);
#endif
    return 0;
}