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
#include "ECS/systems/systems.hpp"
#include "ECS/entities/entities.hpp"
#include "ECS/components/components.hpp"

#include "llvm/SmallVector.h"
#include "llvm/ArrayRef.h"

void placeInserter(ChunkMap& chunkmap, EntityWorld* ecs, Vec2 mouseWorldPos) {
    Tile* tile = getTileAtPosition(chunkmap, mouseWorldPos);
    // TODO: entity collision stuff
    if (tile) {
        Vec2 inputPos = {mouseWorldPos.x + 1, mouseWorldPos.y};
        Vec2 outputPos = {mouseWorldPos.x - 1, mouseWorldPos.y};
        Entity inserter = Entities::Inserter(ecs, Vec2(floor(mouseWorldPos.x), floor(mouseWorldPos.y)) + Vec2(0.5f, 0.5f), 1, vecFloori(inputPos), vecFloor(outputPos));
        placeEntityOnTile(ecs, tile, inserter);
    }
}

void rotateEntity(const ComponentManager<EC::Rotation, EC::Rotatable>& ecs, EntityT<EC::Rotation, EC::Rotatable> entity, bool clockwise) {
    float* rotation = &entity.Get<EC::Rotation>(&ecs)->degrees;
    auto rotatable = entity.Get<EC::Rotatable>(&ecs);
    // left shift switches direction
    if (clockwise) {
        *rotation += rotatable->increment;
    } else {
        *rotation -= rotatable->increment;
    }
    rotatable->rotated = true;
}

void setDefaultKeyBindings(Game& ctx, PlayerControls* controls) {
    GameState& state = *ctx.state;
    Player& player = state.player;
    EntityWorld& ecs = state.ecs;
    ChunkMap& chunkmap = state.chunkmap;
    Camera& camera = ctx.camera;
    PlayerControls& playerControls = *ctx.playerControls;
    Vec2& mouseWorldPos = playerControls.mouseWorldPos;
    DebugClass& debug = *ctx.debug;
    ItemManager& itemManager = state.itemManager;

    controls->addKeyBinding(new FunctionKeyBinding('y',
    [&ecs, &camera, &state, &playerControls](){
        auto& mouse = playerControls.mouse;
        Entity zombie = Entities::Enemy(
            &ecs,
            camera.pixelToWorld(mouse.x, mouse.y),
            state.player.entity
        );
        (void)zombie;
    }));

    KeyBinding* keyBindings[] = {
        new FunctionKeyBinding('u', [&](){
            debug.settings["drawEntityViewBoxes"] = !debug.settings["drawEntityViewBoxes"];
        }),
        new FunctionKeyBinding('p', [&](){
            debug.settings["drawEntityCollisionBoxes"] = !debug.settings["drawEntityCollisionBoxes"];
        }),

        new FunctionKeyBinding('t', [&player, &playerControls, &chunkmap](){
            // teleport
            player.setPosition(chunkmap, playerControls.mouseWorldPos);
        }),
        
        new FunctionKeyBinding('c', [&itemManager, &ecs, &playerControls](){
            int width = 2;
            int height = 1;
            Vec2 position = vecFloor(playerControls.mouseWorldPos) + Vec2(width/2.0f, height/2.0f);
            auto chest = Entities::Chest(&ecs, position, 3, width, height, itemManager);
            (void)chest;
        }),
        new FunctionKeyBinding('l', [&ecs](){
            // do airstrikes row by row
            for (int y = -100; y < 100; y += 5) {
                for (int x = -100; x < 100; x += 5) {
                    Entities::Airstrike(&ecs, Vec2(x, y * 2), {3.0f, 3.0f}, Vec2(x, y));
                }
            }
        }),
        new FunctionKeyBinding('i', [&ecs, &mouseWorldPos, &chunkmap](){
            placeInserter(chunkmap, &ecs, mouseWorldPos);
        }),
        new FunctionKeyBinding('r', [&playerControls, &ecs, &chunkmap](){
            auto focusedEntity = findPlayerFocusedEntity(ecs, chunkmap, playerControls.mouseWorldPos);
            if (focusedEntity.Has<EC::Rotation, EC::Rotatable>(&ecs))
                rotateEntity(ComponentManager<EC::Rotation, EC::Rotatable, EC::Position>(&ecs), focusedEntity.cast<EC::Rotation, EC::Rotatable>(), playerControls.keyboard.keyState[SDL_SCANCODE_LSHIFT]);
        }),
        // testing stuff
        new FunctionKeyBinding('5', [&](){
            ecs.IterateEntities([&](Entity entity){
                // dont kill player plz
                if (entity.id != player.entity.id) {
                    ecs.Destroy(entity);
                }
            });
        }),
        
    };

    for (size_t i = 0; i < sizeof(keyBindings) / sizeof(KeyBinding*); i++) {
        controls->addKeyBinding(keyBindings[i]);
    }
}

/*
bool canRunConcurrently(
    ECS::ComponentAccessType* accessArrayA, ECS::ComponentAccessType* accessArrayB,
    bool AContainsStructuralChanges, bool AMustExecuteAfterStructuralChanges,
    bool BContainsStructuralChanges, bool BMustExecuteAfterStructuralChanges
) {
    using namespace ECS::ComponentAccess;
    for (int i = 0; i < NUM_COMPONENTS; i++) {
        const ECS::ComponentAccessType accessA = accessArrayA[i];
        const ECS::ComponentAccessType accessB = accessArrayB[i];
        if (accessA & (Write | Remove)) {
            if (accessB & (Write | Read | Remove | Add | Flag)) {
                // fail
                return false;
            }
        }
        else if (accessB & (Write | Remove)) {
            if (accessA & (Write | Read | Remove | Add | Flag)) {
                // fail
                return false;
            }
        }

        if ((accessA & Add) && (accessB & (Add | Remove | Write | Read | Flag))) {
            return false;
        }

        if ((accessB & Add) && (accessA & (Add | Remove | Write | Read | Flag))) {
            return false;
        }

        if (AContainsStructuralChanges) {
            if (BMustExecuteAfterStructuralChanges) {
                // fail
                return false;
            }
        }

        if (BContainsStructuralChanges) {
            if (AMustExecuteAfterStructuralChanges) {
                // fail
                return false;
            }
        } 
    }

    return true;
}

bool canRunConcurrently(ECS::EntitySystem* sysA, ECS::EntitySystem* sysB) {
    return canRunConcurrently(sysA->sys.componentAccess, sysB->sys.componentAccess,
        sysA->containsStructuralChanges, sysA->mustExecuteAfterStructuralChanges,
        sysB->containsStructuralChanges, sysB->mustExecuteAfterStructuralChanges);
}

void systemJob(ECS::EntitySystem* system) {
    // Log("starting job for system %p", system);
    system->Update();
    // Log("finishing job for system %p", system);
}

void runConcurrently(EntityWorld* ecs, ECS::EntitySystem* sysA, ECS::EntitySystem* sysB) {
    if (!canRunConcurrently(sysA, sysB)) {
        LogCritical("Attempted to run two incompatible systems together concurrently! Running in sequence.");
        systemJob(sysA);
        systemJob(sysB);
        return;
    }

    std::thread thread2(systemJob, sysA);
    std::thread thread3(systemJob, sysB);

    thread2.join();
    thread3.join();

    return;
}

void playBackCommandBuffer(EntityWorld* ecs, ECS::EntityCommandBuffer* buffer) {
    //for (auto command : buffer->commands) {
    //    command(ecs);
    //}
    //buffer->commands.clear();
}
*/
/*
template<class S>
void runSystem(EntityWorld* ecs) {
    ecs->System<S>()->Update();
    playBackCommandBuffer(ecs, ecs->System<S>()->commandBuffer);
}
*/

void updateDynamicEntityChunkPositions(EntityWorld& ecs, GameState* state) {
    ecs.ForEach<
    EntityQuery< ECS::RequireComponents<EC::Dynamic, EC::ViewBox> >
    >([&](auto entity){
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

        entityPositionChanged(state, entity, oldPos);
        /*
        

        if (UNLIKELY(!isValidEntityPosition(newPos))) {
            LogCritical("Entity has invalid position! Entity: %s, Position: %f,%f", entity.DebugStr(), newPos.x, newPos.y);
            // don't move at all
            ecs.Set<EC::Dynamic>(entity, {oldPos});
            return;
        }

        / put entity in to the chunks it's visible in /

        IVec2 oldMinChunkPosition = toChunkPosition(oldPos);
        IVec2 oldMaxChunkPosition = toChunkPosition(oldPos + size);
        IVec2 newMinChunkPosition = toChunkPosition(newPos);
        IVec2 newMaxChunkPosition = toChunkPosition(newPos + size);
        IVec2 minChunkPosition = {
            (oldMinChunkPosition.x < newMinChunkPosition.x) ? oldMinChunkPosition.x : newMinChunkPosition.x,
            (oldMinChunkPosition.y < newMinChunkPosition.y) ? oldMinChunkPosition.y : newMinChunkPosition.y
        };
        IVec2 maxChunkPosition = {
            (oldMaxChunkPosition.x > newMaxChunkPosition.x) ? oldMaxChunkPosition.x : newMaxChunkPosition.x,
            (oldMaxChunkPosition.y > newMaxChunkPosition.y) ? oldMaxChunkPosition.y : newMaxChunkPosition.y
        };
        for (int col = minChunkPosition.x; col <= maxChunkPosition.x; col++) {
            for (int row = minChunkPosition.y; row <= maxChunkPosition.y; row++) {
                IVec2 chunkPosition = {col, row};

                bool inNewArea = (chunkPosition.x >= newMinChunkPosition.x && chunkPosition.y >= newMinChunkPosition.y &&
                    chunkPosition.x <= newMaxChunkPosition.x && chunkPosition.y <= newMaxChunkPosition.y);
                
                bool inOldArea = (chunkPosition.x >= oldMinChunkPosition.x && chunkPosition.y >= oldMinChunkPosition.y &&
                    chunkPosition.x <= oldMaxChunkPosition.x && chunkPosition.y <= oldMaxChunkPosition.y);

                if ((inNewArea && !inOldArea)) {
                    // add entity to new chunk
                    ChunkData* newChunkdata = state->chunkmap.get(chunkPosition);
                    if (newChunkdata) {
                        newChunkdata->closeEntities.push(entity);
                    }
                }

                / Remove entity form chunks it's no longer in /

                if (inOldArea && !inNewArea) {
                    // remove entity from old chunk
                    ChunkData* oldChunkdata = state->chunkmap.get(chunkPosition);
                    if (oldChunkdata) {
                        oldChunkdata->removeCloseEntity(entity);
                    }
                }
            }
        }
        
        positionEc->x = dynamicEc->pos.x;
        positionEc->y = dynamicEc->pos.y;
        */

    });
}

static void updateSystems(GameState* state) {
    EntityWorld& ecs = state->ecs;
    auto& chunkmap = state->chunkmap;

    ecs.ForEach<
    EntityQuery< ECS::RequireComponents<EC::Dynamic, EC::Follow> >
    >([&](EntityT<EC::Dynamic, EC::Follow> entity){
        auto followComponent = ecs.Get<EC::Follow>(entity);
        assert(followComponent);
        if (!ecs.EntityExists(followComponent->entity)) {
            return;
        }
        EntityT<EC::Position> following = followComponent->entity;
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
            if (following.Has<EC::Health>(&ecs)) {
                following.Get<EC::Health>(&ecs)->damage(10);
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

    
    //MotionSystem motionSystem;
    //motionSystem.Update(ecs, &state->chunkmap);

    /*
    state->chunkmap.iterateChunkdata([](ChunkData* chunkdata){
        chunkdata->closeEntities.clear();
        return 0;
    });
    */

    /*
        Vec2 size = sys.GetReadOnly<EC::Size>(entity)->vec2();

        IVec2 minChunkPosition = toChunkPosition(position - size * 0.5f);
        IVec2 maxChunkPosition = toChunkPosition(position + size * 0.5f);
        for (int col = minChunkPosition.x; col <= maxChunkPosition.x; col++) {
            for (int row = minChunkPosition.y; row <= maxChunkPosition.y; row++) {
                IVec2 chunkPosition = {col, row};
                ChunkData* chunkdata = chunkmap->get(chunkPosition);
                if (chunkdata) {
                    chunkdata->closeEntities.push(entity);
                }
            }
        }
        */

   /*
    ecs.System<MotionSystem>()->m_ecs = &ecs;
    ecs.System<MotionSystem>()->m_chunkmap = &state->chunkmap;

    std::array<EntitySystem*, 3> group = {
        ecs.System<GrowthSystem>(),
        ecs.System<MotionSystem>(),
        ecs.System<RotationSystem>()
    };
    runConcurrently(&ecs, group);

    runSystem<FollowSystem>(&ecs);
    // ecs.System<ExplosivesSystem>()->Update();
    runSystem<ExplosivesSystem>(&ecs);
    */

    /*
    state->chunkmap.iterateChunkdata([](ChunkData* chunkdata){
        chunkdata->closeEntities.clear();
        return 0;
    });
    runSystem<PositionSystem>(&ecs);
    */

   /*
    ecs.System<ExplosionSystem>()->Update(&ecs, state->chunkmap);
    playBackCommandBuffer(&ecs, ecs.System<ExplosionSystem>()->commandBuffer);
    runSystem<InserterSystem>(&ecs);
    runSystem<InventorySystem>(&ecs);
    runSystem<HealthSystem>(&ecs);
    runSystem<RotatableSystem>(&ecs);
    runSystem<DyingSystem>(&ecs);
    */
    
    ecs.ForEach<
    EntityQuery< ECS::RequireComponents<EC::Health> >
    >([&](Entity entity){
        auto* health = ecs.Get<EC::Health>(entity); assert(health);
        // Must do check like this instead of (*health <= 0.0f) to account for NaN values,
        // which can occur when infinite damage is done to an entity with infinite health
        // so in that situation the infinite damage wins out, rather than the infinte health
        if (!(health->health > 0.0f)) {
            if (!ecs.EntitySignature(entity.id).getComponent<EC::Immortal>()) {
                ecs.Destroy(entity);
            }
        }

        if (health->iFrames > 0) {
            health->iFrames--;
        }
    });

    ecs.ForEach< EntityQuery< ECS::RequireComponents<EC::Motion, EC::Dynamic> > >(
    [&](auto entity){
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

    ecs.ForEach< EntityQuery< ECS::RequireComponents<EC::Position, EC::Fresh> > >(
    [&](Entity entity){
        auto fresh = ecs.Get<EC::Fresh>(entity); assert(fresh);
        if (fresh->flags.getComponent<EC::Position>()) {
            // ec::position just added
            fresh->flags.setComponent<EC::Position>(0);
        }
    });

    ecs.ForEach< EntityQuery< ECS::RequireComponents<EC::Fresh> > >(
    [&](Entity entity){
        ecs.Remove<EC::Fresh>(entity);
    });
}

int tick(GameState* state, PlayerControls* playerControls) {
    state->player.grenadeThrowCooldown--;
    updateSystems(state);
    if (Metadata->getTick() % 1 == 0) {
        int i = 0;
        /*
        for (auto& chunk: state->chunkmap.chunkList) {
            for (int y = 0; y < CHUNKSIZE; y++) {
                for (int x = 0; x < CHUNKSIZE; x++) {
                    //chunk[y][x].type = TileTypes::Grass;
                    TileType& type = chunk[y][x].type;
                    if (type == TileTypes::Grass) {
                        type = TileTypes::Sand;
                    }
                    else if (type == TileTypes::Sand) {
                        if (randomInt(0, 3) == 0) {
                            type = TileTypes::Grass;
                        }
                    } else {
                        chunk[y][x].type = randomInt(TileTypes::Grass, TileTypes::Wall);
                    }
                }
            }
        }
        */
        /*
        for (int x = -200; x < 200; x++) {
            for (int y = -200; y < 200; y++) {
                #define GETT(x, y) getTileAtPosition(state->chunkmap, IVec2{x, y})
                #define GETTT(x, y) GETT(x, y)->type
                Tile* tile = getTileAtPosition(state->chunkmap, IVec2{x, y});
                if (tile) {
                    TileType& type = tile->type;
                    if (GETT(x, y+1) && GETT(x, y-1) && GETTT(x, y+1) == GETTT(x, y-1)) {
                        type = GETTT(x, y+1);
                    }
                    if (GETT(x+1, y) && GETT(x-1, y) && GETTT(x+1, y) == GETTT(x-1, y)) {
                        GETTT(x+1, y) = type;
                        GETTT(x-1, y) = type;
                    }
                }
            }
        }
        */
    }

    playerControls->doPlayerMovementTick();

    updateDynamicEntityChunkPositions(state->ecs, state);

    return 0;
}

void logComponentPoolSizes(const EntityWorld& ecs) {
    LogInfo("Total number of entities: %u", ecs.EntityCount());
    for (ECS::ComponentID id = 0; id < ecs.NumComponentPools(); id++) {
        const ECS::ComponentPool* pool = ecs.GetPool(id);
        LogInfo("%s || Size: %u", pool->name, pool->size());
        if (pool->size() > ecs.EntityCount()) {
            LogError("Size is too large!");
        }
    }
}

void displaySizeChanged(SDL_Window* window, const RenderContext& renderContext, Camera* camera) {
    int drawableWidth,drawableHeight;
    SDL_GL_GetDrawableSize(window, &drawableWidth, &drawableHeight);
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
        case SDL_QUIT:
            code = 1; 
            break;
        case SDL_WINDOWEVENT:
            if (event->window.event == SDL_WINDOWEVENT_RESIZED) {
                displaySizeChanged(sdlCtx.win, *renderContext, &camera);
            }
            if (event->window.event == SDL_WINDOWEVENT_DISPLAY_CHANGED) {
                float oldPixelScale = SDL::pixelScale;
                SDL::pixelScale = SDL::getPixelScale(sdlCtx.win);
                camera.baseScale = BASE_UNIT_SCALE * SDL::pixelScale;
                float pixelScaleChange = SDL::pixelScale / oldPixelScale;
                scaleAllFonts(renderContext->fonts, pixelScaleChange);
                displaySizeChanged(sdlCtx.win, *renderContext, &camera);
            }
            break;
        case SDL_MOUSEWHEEL: {
            float wheelMovement = event->wheel.preciseY;
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
        case SDL_KEYDOWN: {
            switch (event->key.keysym.sym) {
                case ']':
                    logComponentPoolSizes(state->ecs);
                    break;
            }
        break;}
        case SDL_CONTROLLERDEVICEADDED: {
            int joystickIndex = event->jdevice.which;
            LogInfo("Controller device added. Joystick index: %d", joystickIndex);
            playerControls->connectController();
        break;}
        case SDL_CONTROLLERDEVICEREMOVED: {
            int instanceID = event->jdevice.which;
            LogInfo("Controller device removed. Instance id: %d", instanceID);
            playerControls->controller.disconnect();
        break;}
        case SDL_CONTROLLERBUTTONDOWN: {
            Uint8 buttonPressed = event->cbutton.button;
            if (buttonPressed == SDL_CONTROLLER_BUTTON_A) {
                playerControls->movePlayer(playerControls->mouseWorldPos - state->player.get<EC::Position>()->vec2());
            }
        break;}
        default:
        break;
    }

    playerControls->handleEvent(event);
    return code;
}

void focusCameraOnEntity(Camera* camera, CameraEntityFocus entityFocus, const EntityWorld* ecs) {
    Entity entity = entityFocus.entity;
    auto* viewbox = ecs->Get<EC::ViewBox>(entity);
    auto* pos = ecs->Get<EC::Position>(entity);
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

    auto* rotation = ecs->Get<EC::Rotation>(entity);
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
    
    double targetTPS = (double)Metadata->tick.targetUpdatesPerSecond;
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
    focusCamera(&camera, &cameraFocus, &state->ecs);

    float scale = SDL::pixelScale;
    RenderOptions options = {
        {camera.pixelWidth, camera.pixelHeight},
        scale
    };

    render(*renderContext, options, gui, state, camera, *playerControls, mode, true); 

    lastUpdateMouseState = mouse;
    lastUpdatePlayerTargetPos = playerControls->mouseWorldPos;

    if (quit) {
        LogInfo("Returning from main update loop.");
        return 1;
    }
    return 0; 
}

int Game::init(int screenWidth, int screenHeight) {
    LogInfo("Starting game init");

    this->gui = new Gui();
    this->debug->console = &this->gui->console;

    this->renderContext = new RenderContext(sdlCtx.win, sdlCtx.gl);
    LogInfo("starting render init");  
    renderInit(*renderContext, screenWidth, screenHeight);

    this->state = new GameState();

    this->state->init(&renderContext->textures);
    for (int e = 0; e < 200; e++) {
        Vec2 pos = {(float)randomInt(-400, 400), (float)randomInt(-400, 400)};
        // do placing collision checks
        auto tree = Entities::Tree(&state->ecs, pos, {4, 4});
        (void)tree;
    }

    Entities::Tree(&state->ecs, {10.5, 10.5}, {40, 40});

    for (int i = 0; i < 10; i++) {
        auto tree = Entities::Grenade(&state->ecs, Vec2(i*2));
        state->ecs.Get<EC::Render>(tree)->textures[1].layer = i;
        state->ecs.Get<EC::Render>(tree)->textures[0].layer = i;
    }

    this->playerControls = new PlayerControls(this);
    SDL_Point mousePos = SDL::getMousePixelPosition();
    this->lastUpdateMouseState = {
        .x = mousePos.x,
        .y = mousePos.y,
        .buttons = SDL::getMouseButtons()
    };

    setDefaultKeyBindings(*this, playerControls);

    glViewport(0, 0, screenWidth, screenHeight);
    this->camera = Camera(BASE_UNIT_SCALE, glm::vec3(0.0f), screenWidth, screenHeight);
    this->cameraFocus = {CameraEntityFocus{this->state->player.entity, true}};

    setCommands(this);

    //resizeRenderBuffer(renderContext->framebuffer, {screenWidth+1, screenHeight});

    return 0;
}

void Game::quit() {
    LogInfo("Quitting!");
    mode = Quit;

    renderQuit(*renderContext);

    double secondsElapsed = metadata.end();
    LogInfo("Time elapsed: %.1f", secondsElapsed);
    // GameSave::save()
}

void Game::destroy() {
    LogInfo("destroying game");
    Debug = nullptr;
    delete this->debug;
    delete this->playerControls;
    delete this->renderContext;
}

int Game::start() {
    // GameSave::load()

    LogInfo("Starting!");
    mode = Playing;

    if (metadata.vsyncEnabled) {
        metadata.frame.targetUpdatesPerSecond = 60;
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