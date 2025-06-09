#include "GameState.hpp"

#include <functional>
#include <array>
#include <glm/geometric.hpp>

#include <SDL3/SDL.h>
#include "constants.hpp"
#include "rendering/textures.hpp"
#include "utils/vectors_and_rects.hpp"
#include "Tiles.hpp"
#include "Player.hpp"
#include "ECS/entities/entities.hpp"
#include "ECS/systems/systems.hpp"
#include "metadata/ecs/ecs.hpp"
#include "items/manager.hpp"
#include "items/prototypes/prototypes.hpp"

template<class... Components>
void addComponents(EntityWorld* ecs, Entity entity, Components... components) {
    ecs->StartDeferringEvents();
    bool success[] = {ecs->Add<Components>(entity) ...};
    // do something
    ecs->StopDeferringEvents();
}

Box getEntityViewBoxBounds(const EntityWorld* ecs, Entity entity) {
    Vec2 pos = ecs->Get<EC::Position>(entity)->vec2();
    auto* viewbox = ecs->Get<EC::ViewBox>(entity);
    return Box{
        pos + viewbox->box.min,
        pos + viewbox->box.min + viewbox->box.size
    };
}

void makeItemPrototypes(ItemManager& im) {
    using namespace items;
    auto& pm = im.prototypes;

    auto tile = Prototypes::Tile(pm);
    auto grenade = Prototypes::Grenade(pm);
    auto sandGun = Prototypes::SandGun(pm);

    ItemPrototype prototypes[] = {
        tile, grenade, sandGun
    };

    for (int i = 0; i < sizeof(prototypes) / sizeof(ItemPrototype); i++) {
        pm.add(prototypes[i]);
    }
}

void GameState::init(const TextureManager* textureManager) {
    /* Init Chunkmap */
    chunkmap.init();
    int chunkRadius = 4;
    for (int chunkX = -chunkRadius; chunkX < chunkRadius; chunkX++) {
        for (int chunkY = -chunkRadius; chunkY < chunkRadius; chunkY++) {
            ChunkData* chunkdata = chunkmap.newChunkAt({chunkX, chunkY});
            if (chunkdata) {
                generateChunk(chunkdata);
            } else {
                LogError("Failed to create chunk at tile (%d,%d) for initialization", chunkX * CHUNKSIZE, chunkY * CHUNKSIZE);
                continue;
            }
        }
    }

    /* Init ECS */
    ecs = EntityWorld::Init<ECS_COMPONENTS>();
    ecs.SetOnAdd<EC::ViewBox>([&](EntityWorld* ecs, Entity entity) {

        auto* viewBox = ecs->Get<EC::ViewBox>(entity);
        if (!viewBox) {
            LogError("entity viewbox not found!");
            return;
        }
        auto* position = ecs->Get<EC::Position>(entity);
        if (!position) {
            LogError("Entity position not found. Make sure to add position before viewbox.");
            return;
        }

        Vec2 pos = position->vec2();

        IVec2 minChunkPosition = toChunkPosition(pos + viewBox->box.min);
        IVec2 maxChunkPosition = toChunkPosition(pos + viewBox->box.max());
        for (int col = minChunkPosition.x; col <= maxChunkPosition.x; col++) {
            for (int row = minChunkPosition.y; row <= maxChunkPosition.y; row++) {
                IVec2 chunkPosition = {col, row};
                // add entity to new chunk
                ChunkData* newChunkdata = chunkmap.get(chunkPosition);
                if (newChunkdata) {
                    newChunkdata->closeEntities.push(entity);
                }
            }
        }
    });
    ecs.SetBeforeRemove<EC::ViewBox>([&](EntityWorld* ecs, Entity entity){
        auto* viewbox = ecs->Get<EC::ViewBox>(entity);
        if (!viewbox) {
            LogError("entity viewbox not found!");
            return;
        }
        auto* position = ecs->Get<EC::Position>(entity);
        if (!position) {
            LogError("Entity position not found");
            return;
        }
        forEachChunkContainingBounds(&chunkmap, {{position->vec2() + viewbox->box.min, position->vec2() + viewbox->box.max()}}, [entity](ChunkData* chunkdata){
            if (!chunkdata->removeCloseEntity(entity)) {
                LogWarn("couldn't remove entity");
            }
        });
    });

    ecs.SetOnAdd<EC::Inventory>([](EntityWorld* ecs, Entity entity){
        ecs->Get<EC::Inventory>(entity)->inventory.addRef();
    });
    ecs.SetBeforeRemove<EC::Inventory>([](EntityWorld* ecs, Entity entity){
        ecs->Get<EC::Inventory>(entity)->inventory.removeRef();
    });

    ecs.SetOnAdd<EC::TransportLineEC>([](EntityWorld* ecs, Entity entity){
        auto* transportLineEc = ecs->Get<EC::TransportLineEC>(entity);
        
    });
    ecs.SetBeforeRemove<EC::TransportLineEC>([&](EntityWorld* ecs, Entity entity){
        auto* transportLineEc = ecs->Get<EC::TransportLineEC>(entity);
        transportLineEc->belts.destroy();
    });

    /* Init Items */
    My::Vec<GECS::ComponentInfo> componentInfo; // TODO: Unimportant: This is leaked currently
    {
        using namespace ITC;
        constexpr auto componentInfoArray = GECS::getComponentInfoList<ITEM_COMPONENTS_LIST>();
        componentInfo = My::Vec<GECS::ComponentInfo>(&componentInfoArray[0], componentInfoArray.size());
    }
    itemManager = ItemManager(ArrayRef(componentInfo.data, componentInfo.size), ItemTypes::Count);

    makeItemPrototypes(itemManager);
    loadTileData(itemManager, textureManager);

    /* Init Player */
    player = Player(&ecs, Vec2(0, 0), itemManager);

    ItemStack startInventory[] = {
        ItemStack(items::Prototypes::SandGun::make(itemManager)),
        ItemStack(items::Prototypes::Grenade::make(itemManager), 67),
        ItemStack(items::Prototypes::Tile::make(itemManager, TileTypes::Grass), 64),
        ItemStack(items::Prototypes::Tile::make(itemManager, TileTypes::Sand), 24),
        ItemStack(items::Prototypes::Tile::make(itemManager, TileTypes::TransportBelt), 200),
       // ItemStack(items::Items::tile(itemManager, TileTypes::Grass), 2)
    };

    Inventory* playerInventory = player.inventory();
    for (int i = 0; i < (int)(sizeof(startInventory) / sizeof(ItemStack)); i++) {
        playerInventory->addItemStack(startInventory[i]);
    }

    //Entities::Tree(&ecs, {-2, -2}, {5, 5});
    
    //auto tree = Entities::Tree(&ecs, {-20, -20}, {5, 5});
    //Entities::giveName(tree, "Holy tree", &ecs);

    //Entities::scaleGuy(this, tree, 2.0f);

    auto enemy = Entities::Enemy(&ecs, {15, 15}, player.entity);
    ecs.Remove<EC::Follow>(enemy);

    /*
    auto view = ecs.Get<EC::ViewBox>(enemy);
    view->size = {0.2, 0.2};
    auto collision = ecs.Get<EC::CollisionBox>(enemy);
    collision->box[]
    */
    Entities::scaleGuy(this, enemy, 2.0f);
    Entities::giveName(enemy, "MyLittleGuy", &ecs);
}

void GameState::destroy() {
    chunkmap.destroy();
    ecs.destroy();
}

llvm::SmallVector<IVec2> raytraceDDA(const Vec2 start, const Vec2 end) {
    const IVec2 start_i = vecFloori(start);
    const IVec2 end_i = vecFloori(end);
    const int lineLength = (abs(end_i.x - start_i.x) + abs(end_i.y - start_i.y)) + 1;
    llvm::SmallVector<IVec2> line(lineLength, {}); // make small vector of size lineLength left uninitialized
    const Vec2 delta = end - start;
    const Vec2 dir = glm::normalize(delta);
    const Vec2 unitStep = {sqrt(1 + (dir.y / dir.x) * (dir.y / dir.x)), sqrt(1 + (dir.x / dir.y) * (dir.x / dir.y))};

    IVec2 mapCheck = start_i;
    Vec2 rayLength;
    IVec2 step;
    if (dir.x < 0) {
        step.x = -1;
        rayLength.x = (start.x - mapCheck.x) * unitStep.x;
    } else {
        step.x = 1;
        rayLength.x = ((mapCheck.x + 1) - start.x) * unitStep.x;
    }
    if (dir.y < 0) {
        step.y = -1;
        rayLength.y = (start.y - mapCheck.y) * unitStep.y;
    } else {
        step.y = 1;
        rayLength.y = ((mapCheck.y + 1) - start.y) * unitStep.y;
    }

    // have to check for horizontal and vertical lines because of divison by zero things

    // horizontal line
    if (start.y == end.y) {
        for (int i = 0; i < lineLength; i++) {
            line[i] = mapCheck;
            mapCheck.x += step.x;
        }
    }
    // vertical line
    else if (start_i.x == end_i.x) {
        for (int i = 0; i < lineLength; i++) {
            line[i] = mapCheck;
            mapCheck.y += step.y;
        }
    }
    // any other line
    else {
        for (int i = 0; i < lineLength; i++) {
            line[i] = mapCheck;
            if (rayLength.x < rayLength.y) {
                mapCheck.x += step.x;
                rayLength.x += unitStep.x;
            } else {
                mapCheck.y += step.y;
                rayLength.y += unitStep.y;
            }
        }
    }
    
    return line;
}

/*
* Get a line of chunk coordinates from start to end, using DDA.
* Returns a line of size lineSize that must be freed using free()
* @param lineSize a pointer to an int to be filled in the with size of the line.

IVec2* worldChunkLine(Vec2 startChunkCoord, Vec2 endChunkCoord, int* lineSize) {
    IVec2 startChunk = vecFloori(startChunkCoord);
    IVec2 endChunk = vecFloori(endChunkCoord);
    IVec2 delta = endChunk - startChunk;
    int lineTileLength = (abs(delta.x) + abs(delta.y)) + 1;
    IVec2* line = Alloc<IVec2>(lineTileLength);
    if (lineSize) *lineSize = lineTileLength;

    Vec2 dir = glm::normalize((Vec2)(endChunk - startChunk));

    IVec2 mapCheck = startChunk;
    Vec2 unitStep = {sqrt(1 + (dir.y / dir.x) * (dir.y / dir.x)), sqrt(1 + (dir.x / dir.y) * (dir.x / dir.y))};
    Vec2 rayLength;
    IVec2 step;
    if (dir.x < 0) {
        step.x = -1;
        rayLength.x = (start.x - mapCheck.x) * unitStep.x;
    } else {
        step.x = 1;
        rayLength.x = ((mapCheck.x + 1) - start.x) * unitStep.x;
    }
    if (dir.y < 0) {
        step.y = -1;
        rayLength.y = (start.y - mapCheck.y) * unitStep.y;
    } else {
        step.y = 1;
        rayLength.y = ((mapCheck.y + 1) - start.y) * unitStep.y;
    }

    // have to check for horizontal and vertical lines because of divison by zero things

    // horizontal line
    if (startTile.y == endTile.y) {
        for (int i = 0; i < lineTileLength; i++) {
            line[i] = mapCheck;
            mapCheck.x += step.x;
        }
    }
    // vertical line
    else if (startTile.x == endTile.x) {
        for (int i = 0; i < lineTileLength; i++) {
            line[i] = mapCheck;
            mapCheck.y += step.y;
        }
    }
    // any other line
    else {
        for (int i = 0; i < lineTileLength; i++) {
            line[i] = mapCheck;
            if (rayLength.x < rayLength.y) {
                mapCheck.x += step.x;
                rayLength.x += unitStep.x;
            } else {
                mapCheck.y += step.y;
                rayLength.y += unitStep.y;
            }
        }
    }
    
    return line;
}
*/

void worldLineAlgorithm(Vec2 start, Vec2 end, const std::function<int(IVec2)>& callback) {
    Vec2 delta = end - start;
    Vec2 dir = glm::normalize(delta);

    IVec2 mapCheck = {(int)floor(start.x), (int)floor(start.y)};
    Vec2 unitStep = {sqrt(1 + (dir.y / dir.x) * (dir.y / dir.x)), sqrt(1 + (dir.x / dir.y) * (dir.x / dir.y))};
    Vec2 rayLength;
    IVec2 step;
    if (dir.x < 0) {
        step.x = -1;
        rayLength.x = (start.x - mapCheck.x) * unitStep.x;
    } else {
        step.x = 1;
        rayLength.x = ((mapCheck.x + 1) - start.x) * unitStep.x;
    }
    if (dir.y < 0) {
        step.y = -1;
        rayLength.y = (start.y - mapCheck.y) * unitStep.y;
    } else {
        step.y = 1;
        rayLength.y = ((mapCheck.y + 1) - start.y) * unitStep.y;
    }

    // horizontal line
    if (floor(start.y) == floor(end.y)) {
        int iterations = abs(floor(end.x) - mapCheck.x) + 1;
        for (int i = 0; i < iterations; i++) {
            if (callback(mapCheck)) {
                return;
            }
            mapCheck.x += step.x;
        }
        return;
    }
    // vertical line
    if (floor(start.x) == floor(end.x)) {
        int iterations = abs(floor(end.y) - mapCheck.y) + 1;
        for (int i = 0; i < iterations; i++) {
            if (callback(mapCheck)) {
                return;
            }
            mapCheck.y += step.y;
        }
        return;
    }

    int iterations = 0; // for making sure it doesn't go infinitely
    while (iterations < 300) {
        if (rayLength.x < rayLength.y) {
            mapCheck.x += step.x;
            rayLength.x += unitStep.x;
        } else {
            mapCheck.y += step.y;
            rayLength.y += unitStep.y;
        }

        if (callback(mapCheck)) {
            return;
        }

        if (mapCheck.x == (int)floor(end.x) && mapCheck.y == (int)floor(end.y)) {
            break;
        }

        iterations++;
    }
}

OptionalEntity<> findTileEntityAtPosition(const GameState* state, Vec2 position) {
    IVec2 tilePosition = vecFloori(position);
    Tile* selectedTile = getTileAtPosition(state->chunkmap, tilePosition);
    if (selectedTile) {
        /*
        auto tileEntity = selectedTile->entity;
        if (tileEntity.Exists(&state->ecs)) {
            if (pointIsOnTileEntity(&state->ecs, tileEntity, tilePosition, position)) {
                return tileEntity;
            }
        }
        */
    }

    // no entity could be found at that position
    return NullEntity;
}

bool removeEntityOnTile(EntityWorld* ecs, Tile* tile) {
    /*
    if (tile->entity.Exists(ecs)) {
        ecs->Destroy(tile->entity);
        return true;
    }
    tile->entity = NullEntity;
    */
    return false;
}

bool placeEntityOnTile(EntityWorld* ecs, Tile* tile, Entity entity) {
    /*
    if (!tile->entity.Exists(ecs)) {
        tile->entity = entity;
        return true;
    }
    */
    return false;
}

void forEachEntityInRange(const ComponentManager<EC::ViewBox>& ecs, const ChunkMap* chunkmap, Vec2 pos, float radius, const std::function<int(EntityT<EC::ViewBox>)>& callback) {
    radius = abs(radius);
    float radiusSqrd = radius * radius;

    // form rectangle of chunks to search for entities in
    IVec2 highChunkBound = {(int)floor((pos.x + radius) / CHUNKSIZE), (int)floor((pos.y + radius) / CHUNKSIZE)};
    IVec2 lowChunkBound =  {(int)floor((pos.x - radius) / CHUNKSIZE), (int)floor((pos.y - radius) / CHUNKSIZE)};

    // go through each chunk in chunk search rectangle
    for (int y = lowChunkBound.y; y <= highChunkBound.y; y++) {
        for (int x = lowChunkBound.x; x <= highChunkBound.x; x++) {
            const ChunkData* chunkdata = chunkmap->get({x, y});
            if (chunkdata == NULL) {
                // entities can't be in non-existent chunks
                continue;
            }

            for (int e = 0; e < chunkdata->closeEntities.size; e++) {
                Entity closeEntity = chunkdata->closeEntities[e];
                if (!ecs.EntityExists(closeEntity)) {
                    LogError("Entity in closeEntities is dead!");
                    continue;
                }
                EC::ViewBox* entityBox = ecs.Get<EC::ViewBox>(closeEntity);
                assert(entityBox);
                Vec2 entityCenter = entityBox->box.center();
                Vec2 entitySize = entityBox->box.size;

                Vec2 halfSize = entitySize * 0.5f;

                // TODO: collision is broke for non squares. need math and stuff

                radiusSqrd += (halfSize.x * halfSize.y + halfSize.y * halfSize.y) * M_SQRT2;
            
                Vec2 delta = entityCenter - pos;
                float distSqrd = delta.x * delta.x + delta.y * delta.y;
                if (distSqrd < radiusSqrd) {
                    // entity is within radius
                    if (callback(closeEntity)) {
                        return;
                    }
                }
            }
        }
    }
}

void forEachEntityNearPoint(ComponentManager<EC::ViewBox> ecs, const ChunkMap* chunkmap, Vec2 point, const std::function<int(Entity)>& callback) {
    IVec2 chunkPos = toChunkPosition(point);
    const ChunkData* chunkdata = chunkmap->get(chunkPos);
    if (!chunkdata) {
        // entities can't be in non-existent chunks
        return;
    }

    for (int e = 0; e < chunkdata->closeEntities.size; e++) {
        Entity closeEntity = chunkdata->closeEntities[e];
        if (callback(closeEntity)) {
            return;
        }
    }
}

void forEachChunkContainingBounds(const ChunkMap* chunkmap, Boxf bounds, const std::function<void(ChunkData*)>& callback) {
    IVec2 minChunkPosition = toChunkPosition(bounds[0]);
    IVec2 maxChunkPosition = toChunkPosition(bounds[1]);
    for (int col = minChunkPosition.x; col <= maxChunkPosition.x; col++) {
        for (int row = minChunkPosition.y; row <= maxChunkPosition.y; row++) {
            IVec2 chunkPosition = {col, row};
            ChunkData* chunkdata = chunkmap->get(chunkPosition);
            if (chunkdata) {
                callback(chunkdata);
            }
        }
    }
}

void forEachEntityInBounds(ComponentManager<const EC::ViewBox> ecs, const ChunkMap* chunkmap, Boxf bounds, const std::function<void(Entity)>& callback) {
    forEachChunkContainingBounds(chunkmap, bounds, [&](ChunkData* chunkdata){
        Vec2 min = bounds[0];
        Vec2 max = bounds[1];

        for (int i = chunkdata->closeEntities.size-1 ; i >= 0; i--) {
            auto closeEntity = chunkdata->closeEntities[i];
            /* TEMPORARY!  TODO: replace this */
            if (!ecs.EntityExists(closeEntity)) {
                chunkdata->removeCloseEntity(closeEntity);
                continue;
            }

            auto* entityViewbox = ecs.Get<const EC::ViewBox>(closeEntity);
            auto* entityPos = ecs.Get<const EC::Position>(closeEntity);
            assert(entityViewbox && entityPos);

            Vec2 eMin = entityPos->vec2() + entityViewbox->box.min;
            Vec2 eMax = eMin + entityViewbox->box.size;

            if (eMin.x < max.x && eMax.x > min.x &&
                eMin.y < max.y && eMax.y > min.y)
            {
                callback(closeEntity);
            }
        }
    });
}

OptionalEntity<EC::ViewBox>
findFirstEntityAtPosition(const EntityWorld& ecs, const ChunkMap* chunkmap, Vec2 position) {
    OptionalEntity<EC::ViewBox> foundEntity;
    forEachEntityNearPoint(ComponentManager<EC::ViewBox>(&ecs), chunkmap, position, [&](Entity entity){
        if (pointInEntity(position, entity, ComponentManager<EC::ViewBox>(&ecs))) {
            // player is targeting this entity
            foundEntity = entity;
            return 1;
        }
        return 0;
    });

    return foundEntity;
}



OptionalEntity<EC::Position>
findClosestEntityToPosition(const EntityWorld* ecs, const ChunkMap* chunkmap, Vec2 position) {
    EntityT<EC::Position> closestEntity = NullEntity;
    float closestDistSqrd = INFINITY;
    forEachEntityNearPoint(ComponentManager<EC::ViewBox>(ecs), chunkmap, position, [&](Entity entity){
        // TODO: only works correctly for squares
        Vec2 entityCenter = ecs->Get<EC::ViewBox>(entity)->box.center();
        Vec2 delta = entityCenter - position;
        float entityDistSqrd = delta.x*delta.x + delta.y*delta.y;
        if (entityDistSqrd < closestDistSqrd) {
            closestEntity = entity;
            closestDistSqrd = entityDistSqrd;
        }
        return 0;
    });

    return closestEntity;
}

OptionalEntity<EC::ViewBox, EC::Render>
findPlayerFocusedEntity(ComponentManager<EC::ViewBox, const EC::Render> ecs, const ChunkMap& chunkmap, Vec2 playerMousePos) {
    Vec2 target = playerMousePos;
    Entity focusedEntity;
    int focusedEntityLayer = RenderLayers::Lowest;
    forEachEntityNearPoint(ecs, &chunkmap, target,
    [&](Entity entity){
        if (!entity.Has<EC::Position, EC::ViewBox, EC::Render>(&ecs)) {
            return 0;
        }

        auto render = entity.Get<const EC::Render>(&ecs);
        // TODO: enhance for multiple textures
        int entityLayer = render->textures[0].layer;

        if (pointInEntity(target, entity, ecs)) {
            if (entityLayer > focusedEntityLayer
            || (entityLayer == focusedEntityLayer && entity.id > focusedEntity.id)) {
                focusedEntity = entity;
                focusedEntityLayer = entityLayer;
            }
        }

        return 0;
    });
    return focusedEntity;
}