#include "GameState.hpp"

#include <unordered_map>
#include <functional>
#include <array>
#include <vector>

#include <SDL2/SDL.h>
#include "constants.hpp"
#include "Textures.hpp"
#include "NC/cpp-vectors.hpp"
#include "NC/utils.h"
#include "SDL_FontCache/SDL_FontCache.h"
#include "SDL2_gfx/SDL2_gfx.h"
#include "Tiles.hpp"
#include "Player.hpp"
#include "Entities/Entities.hpp"
#include "EntitySystems/Rendering.hpp"
#include "EntitySystems/Systems.hpp"

//void foreachEntityInRange(ECS* ecs, Vec2 point, float distance, std::function<void(Entity)> callback) {   
//}

GameState::~GameState() {
    chunkmap.destroy();
    ecs.destroy();
}

void GameState::init(SDL_Renderer* renderer, GameViewport* gameViewport) {

    chunkmap.init();
    int chunkRadius = 10;
    for (int chunkX = -chunkRadius; chunkX < chunkRadius; chunkX++) {
        for (int chunkY = -chunkRadius; chunkY < chunkRadius; chunkY++) {
            ChunkData* chunkdata = chunkmap.createChunk({chunkX, chunkY});
            generateChunk(chunkdata->chunk);
        }
    }

    ecs.init<COMPONENTS>();

    auto renderSystem = new SimpleRectRenderSystem(&ecs, renderer, gameViewport);
    ecs.NewSystem<SimpleRectRenderSystem>(renderSystem);
    ecs.NewSystems<SYSTEMS>();
    ecs.System<InserterSystem>()->chunkmap = &chunkmap;
    ecs.testInitialization();

    SpecialEntityStatic::Init(&ecs);
    setGlobalECS(&ecs);

    player = Player(&ecs);

    ItemStack startInventory[] = {
        {Items::SpaceFloor, INFINITE_ITEM_QUANTITY},
        {Items::Grass, 64},
        {Items::Wall, 64},
        {Items::Grenade, INFINITE_ITEM_QUANTITY},
        {Items::SandGun, 1}
    };

    Inventory* playerInventory = player.inventory();
    for (int i = 0; i < (int)(sizeof(startInventory) / sizeof(ItemStack)); i++) {
        playerInventory->addItemStack(startInventory[i]);
    }
}

/*
* Get a line of tile coordinates from start to end, using DDA.
* Returns a line of size lineSize that must be freed using free()
* @param lineSize a pointer to an int to be filled in the with size of the line.
*/
IVec2* worldLine(Vec2 start, Vec2 end, int* lineSize) {
    IVec2 startTile = {(int)floor(start.x), (int)floor(start.y)};
    IVec2 endTile = {(int)floor(end.x), (int)floor(end.y)};
    int lineTileLength = (abs(endTile.x - startTile.x) + abs(endTile.y - startTile.y)) + 1;
    IVec2* line = (IVec2*)malloc(lineTileLength * sizeof(IVec2));
    if (lineSize) *lineSize = lineTileLength;

    Vec2 dir = ((Vec2)(end - start)).norm();

    IVec2 mapCheck = startTile;
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

void worldLineAlgorithm(Vec2 start, Vec2 end, std::function<int(IVec2)> callback) {
    Vec2 delta = end - start;
    Vec2 dir = delta.norm();

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

bool pointIsOnTileEntity(const ECS* ecs, const Entity tileEntity, IVec2 tilePosition, Vec2 point) {
    // check if the click was actually on the entity.
    bool clickedOnEntity = true;
    // if the entity doesnt have size data, assume the click was on the entity I guess,
    // since you cant do checks otherwise.
    // Kind of undecided whether the player should be able to click on an entity with no size
    auto size = ecs->Get<EC::Size>(tileEntity);
    if (size) {
        // if there isn't a position component, assume the position is the top left corner of the tile
        Vec2 position = Vec2(tilePosition.x, tilePosition.y);
        auto positionComponent = ecs->Get<EC::Position>(tileEntity);
        if (positionComponent) {
            position.x = positionComponent->x;
            position.y = positionComponent->y;
        }
        
        // now that we have position and size, do actual geometry check
        clickedOnEntity = (
            point.x > position.x && point.x < position.x + size->width &&
            point.y > position.y && point.y < position.y + size->height);        
    }

    return clickedOnEntity;
}

OptionalEntity<> findTileEntityAtPosition(const GameState* state, Vec2 position) {
    IVec2 tilePosition = position.floorToIVec();
    Tile* selectedTile = getTileAtPosition(state->chunkmap, tilePosition);
    if (selectedTile) {
        auto tileEntity = selectedTile->entity;
        if (tileEntity.Exists()) {
            if (pointIsOnTileEntity(&state->ecs, tileEntity, tilePosition, position)) {
                return tileEntity;
            }
        }
    }

    // no entity could be found at that position
    return NullEntity;
}

bool removeEntityOnTile(ECS* ecs, Tile* tile) {
    if (tile->entity.Exists()) {
        ecs->Destroy(tile->entity);
        return true;
    }
    tile->entity = NullEntity;
    return false;
}

bool placeEntityOnTile(ECS* ecs, Tile* tile, Entity entity) {
    if (!tile->entity.Exists()) {
        tile->entity = entity;
        return true;
    }
    return false;
}

void forEachEntityInRange(const ECS* ecs, const ChunkMap* chunkmap, Vec2 pos, float radius, std::function<int(EntityType<EC::Position>)> callback) {
    int nCheckedEntities = 0;
    radius = abs(radius);
    float radiusSqrd = radius * radius;

    // form rectangle of chunks to search for entities in
    IVec2 highChunkBound = {(int)floor((pos.x + radius) / CHUNKSIZE), (int)floor((pos.y + radius) / CHUNKSIZE)};
    IVec2 lowChunkBound =  {(int)floor((pos.x - radius) / CHUNKSIZE), (int)floor((pos.y - radius) / CHUNKSIZE)};

    // go through each chunk in chunk search rectangle
    for (int y = lowChunkBound.y; y <= highChunkBound.y; y++) {
        for (int x = lowChunkBound.x; x <= highChunkBound.x; x++) {
            const ChunkData* chunkdata = chunkmap->getChunkData({x, y});
            if (chunkdata == NULL) {
                // entities can't be in non-existent chunks
                continue;
            }

            for (size_t e = 0; e < chunkdata->closeEntities.size(); e++) {
                EntityType<EC::Position> closeEntity = chunkdata->closeEntities[e].cast<EC::Position>();
                if (!ecs->EntityExists(closeEntity)) {
                    Log.Error("Entity in closeEntities is dead!");
                    continue;
                }
                Vec2 entityCenter = *closeEntity.Get<EC::Position>(); // this should be getting position component
                if (closeEntity.Has<EC::Size>()) {
                    Vec2 halfSize = closeEntity.Get<EC::Size>()->toVec2().scaled(0.5f);
                    entityCenter += halfSize;
                    radiusSqrd += (halfSize.x * halfSize.x + halfSize.y * halfSize.y) * M_SQRT2;
                }
                Vec2 delta = entityCenter - pos;
                float distSqrd = delta.x * delta.x + delta.y * delta.y;
                if (distSqrd < radiusSqrd) {
                    // entity is within radius
                    nCheckedEntities++;
                    if (callback(closeEntity)) {
                        return;
                    }
                }
            }
        }
    }
}

void forEachEntityNearPoint(const ECS* ecs, const ChunkMap* chunkmap, Vec2 point, std::function<int(EntityType<EC::Position>)> callback) {
    IVec2 chunkPos = tileToChunkPosition(point);
    const ChunkData* chunkdata = chunkmap->getChunkData(chunkPos);
    if (!chunkdata) {
        // entities can't be in non-existent chunks
        return;
    }

    for (size_t e = 0; e < chunkdata->closeEntities.size(); e++) {
        EntityType<EC::Position> closeEntity = chunkdata->closeEntities[e].cast<EC::Position>();
        if (callback(closeEntity)) {
            return;
        }
    }
}

void forEachChunkContainingBounds(const ChunkMap* chunkmap, Vec2 position, Vec2 size, std::function<void(ChunkData*)> callback) {
    IVec2 minChunkPosition = tileToChunkPosition(position - size.scaled(0.5f));
    IVec2 maxChunkPosition = tileToChunkPosition(position + size.scaled(0.5f));
    for (int col = minChunkPosition.x; col <= maxChunkPosition.x; col++) {
        for (int row = minChunkPosition.y; row <= maxChunkPosition.y; row++) {
            IVec2 chunkPosition = {col, row};
            ChunkData* chunkdata = chunkmap->getChunkData(chunkPosition);
            if (chunkdata) {
                callback(chunkdata);
            }
        }
    }
}

OptionalEntity<EC::Position, EC::Size>
findFirstEntityAtPosition(const ECS* ecs, const ChunkMap* chunkmap, Vec2 position) {
    OptionalEntity<EC::Position, EC::Size> foundEntity;
    forEachEntityNearPoint(ecs, chunkmap, position, [&](EntityType<EC::Position> entity){
        if (entity.Has<EC::Size>()) {
            Vec2 entityPos = *ecs->Get<EC::Position>(entity);
            Vec2 size = ecs->Get<EC::Size>(entity)->toVec2();
            if (
              position.x > entityPos.x && position.x < entityPos.x + size.x &&
              position.y > entityPos.y && position.y < entityPos.y + size.y) {
                // player is targeting this entity
                foundEntity = entity.cast<EC::Position, EC::Size>();
                return 1;
            }
        }
        return 0;
    });

    return foundEntity;
}



OptionalEntity<EC::Position>
findClosestEntityToPosition(const ECS* ecs, const ChunkMap* chunkmap, Vec2 position) {
    EntityType<EC::Position> closestEntity = NullEntity;
    float closestDistSqrd = INFINITY;
    forEachEntityNearPoint(ecs, chunkmap, position, [&](EntityType<EC::Position> entity){
        Vec2 entityPos = *ecs->Get<EC::Position>(entity);
        Vec2 delta = entityPos - position;
        float entityDistSqrd = delta.distSqrd();
        if (entityDistSqrd < closestDistSqrd) {
            closestEntity = entity;
            closestDistSqrd = entityDistSqrd;
        }
        return 0;
    });

    return closestEntity;
}

bool pointInsideEntityBounds(Vec2 point, const EntityType<EC::Position, EC::Size> entity) {
    entity.debugAssertType();

    Vec2 entityPosition = entity.Get<EC::Position>()->vec2();
    Vec2 entitySize = entity.Get<EC::Size>()->toVec2();

    Vec2 minEntityPos = entityPosition - entitySize.scaled(0.5f);
    Vec2 maxEntityPos = entityPosition + entitySize.scaled(0.5f);

    if (point.x > minEntityPos.x && point.x < maxEntityPos.x &&
        point.y > minEntityPos.y && point.y < maxEntityPos.y) {
        
        return true;
    }
    return false;
}

void entityCreated(GameState* state, Entity entity) {
    Vec2 entityPos = state->ecs.Get<EC::Position>(entity)->vec2();
    Vec2 entitySize = state->ecs.Get<EC::Size>(entity)->toVec2();
    forEachChunkContainingBounds(&state->chunkmap, entityPos, entitySize, [&](ChunkData* chunkdata){
        chunkdata->closeEntities.push_back(entity);
    });
}

void entitySizeChanged(ChunkMap* chunkmap, const ECS* ecs, EntityType<EC::Position, EC::Size> entity, Vec2 oldSize) {
    Vec2 newSize = ecs->Get<EC::Size>(entity)->toVec2();
    if (oldSize.x == newSize.x && oldSize.y == newSize.y) {
        return;
    }
    Vec2 pos = ecs->Get<EC::Position>(entity)->vec2();
    IVec2 oldMinChunkPosition = tileToChunkPosition(pos - oldSize.scaled(0.5f));
    IVec2 oldMaxChunkPosition = tileToChunkPosition(pos + oldSize.scaled(0.5f));
    IVec2 newMinChunkPosition = tileToChunkPosition(pos - newSize.scaled(0.5f));
    IVec2 newMaxChunkPosition = tileToChunkPosition(pos + newSize.scaled(0.5f));
    IVec2 minChunkPosition = {
        (oldMinChunkPosition.x < newMinChunkPosition.x) ? oldMaxChunkPosition.x : newMaxChunkPosition.x,
        (oldMinChunkPosition.y < newMinChunkPosition.y) ? oldMaxChunkPosition.y : newMaxChunkPosition.y
    };
    IVec2 maxChunkPosition = {
        (oldMaxChunkPosition.x < newMaxChunkPosition.x) ? oldMaxChunkPosition.x : newMaxChunkPosition.x,
        (oldMaxChunkPosition.y < newMaxChunkPosition.y) ? oldMaxChunkPosition.y : newMaxChunkPosition.y
    };
    for (int col = minChunkPosition.x; col <= maxChunkPosition.x; col++) {
        for (int row = minChunkPosition.y; row <= maxChunkPosition.y; row++) {
            IVec2 chunkPosition = {col, row};

            bool inNewArea = (chunkPosition.x >= newMinChunkPosition.x && chunkPosition.y >= newMinChunkPosition.y &&
                chunkPosition.x <= newMaxChunkPosition.x && chunkPosition.y <= newMaxChunkPosition.y);
            
            bool inOldArea = (chunkPosition.x >= oldMinChunkPosition.x && chunkPosition.y >= oldMinChunkPosition.y &&
                chunkPosition.x <= oldMaxChunkPosition.x && chunkPosition.y <= oldMaxChunkPosition.y);

            if (inNewArea && !inOldArea) {
                // add entity to new chunk
                ChunkData* newChunkdata = chunkmap->getChunkData(chunkPosition);
                if (newChunkdata) {
                    newChunkdata->closeEntities.push_back(entity);
                }
            }

            if (inOldArea && !inNewArea) {
                // remove entity from old chunk
                ChunkData* oldChunkdata = chunkmap->getChunkData(chunkPosition);
                if (oldChunkdata) {
                    for (unsigned int e = 0; e < oldChunkdata->closeEntities.size(); e++) {
                        // TODO: try implementing binary search with sorting for faster removal
                        if (oldChunkdata->closeEntities[e].id == entity.id)
                            oldChunkdata->closeEntities.erase(oldChunkdata->closeEntities.begin() + e);
                    }
                }
            }
        }
    }
}

void entityPositionChanged(ChunkMap* chunkmap, const ECS* ecs, EntityType<EC::Position> entity, Vec2 oldPos) {
    Vec2 newPos = ecs->Get<EC::Position>(entity)->vec2();
    if (oldPos.x == newPos.x && oldPos.y == newPos.y) {
        return;
    }

    if (ecs->EntityHas<EC::Size>(entity)) {
        Vec2 size = ecs->Get<EC::Size>(entity)->toVec2();
        IVec2 oldMinChunkPosition = tileToChunkPosition(oldPos - size.scaled(0.5f));
        IVec2 oldMaxChunkPosition = tileToChunkPosition(oldPos + size.scaled(0.5f));
        IVec2 newMinChunkPosition = tileToChunkPosition(newPos - size.scaled(0.5f));
        IVec2 newMaxChunkPosition = tileToChunkPosition(newPos + size.scaled(0.5f));
        IVec2 minChunkPosition = {
            (oldMinChunkPosition.x < newMinChunkPosition.x) ? oldMaxChunkPosition.x : newMaxChunkPosition.x,
            (oldMinChunkPosition.y < newMinChunkPosition.y) ? oldMaxChunkPosition.y : newMaxChunkPosition.y
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

                if (inNewArea && !inOldArea) {
                    // add entity to new chunk
                    ChunkData* newChunkdata = chunkmap->getChunkData(chunkPosition);
                    if (newChunkdata) {
                        newChunkdata->closeEntities.push_back(entity);
                    }
                }

                if (inOldArea && !inNewArea) {
                    // remove entity from old chunk
                    ChunkData* oldChunkdata = chunkmap->getChunkData(chunkPosition);
                    if (oldChunkdata) {
                        for (unsigned int e = 0; e < oldChunkdata->closeEntities.size(); e++) {
                            // TODO: try implementing binary search with sorting for faster removal
                            if (oldChunkdata->closeEntities[e].id == entity.id)
                                oldChunkdata->closeEntities.erase(oldChunkdata->closeEntities.begin() + e);
                        }
                    }
                }
            }
        }
    } else {
        IVec2 oldChunkPosition = tileToChunkPosition(oldPos);
        IVec2 newChunkPosition = tileToChunkPosition(newPos);
        if (oldChunkPosition != newChunkPosition) {
            // add entity to new chunk
            ChunkData* newChunkdata = chunkmap->getChunkData(newChunkPosition);
            if (newChunkdata) {
                newChunkdata->closeEntities.push_back(entity);
            }
            // remove entity from old chunk
            ChunkData* oldChunkdata = chunkmap->getChunkData(oldChunkPosition);
            if (oldChunkdata) {
                for (unsigned int e = 0; e < oldChunkdata->closeEntities.size(); e++) {
                    // TODO: try implementing binary search with sorting for faster removal
                    if (oldChunkdata->closeEntities[e].id == entity.id)
                        oldChunkdata->closeEntities.erase(oldChunkdata->closeEntities.begin() + e);
                }
            }
        }
    }
}

void entityPositionAndSizeChanged(ChunkMap* chunkmap, const ECS* ecs, EntityType<EC::Position, EC::Size> entity, Vec2 oldPos, Vec2 oldSize) {
    Vec2 newPos = ecs->Get<EC::Position>(entity)->vec2();
    Vec2 newSize = ecs->Get<EC::Size>(entity)->toVec2();
    if (oldPos.x == newPos.x && oldPos.y == newPos.y) {
        return;
    }
    if (oldSize.x == newSize.x && oldSize.y == newSize.y) {
        return;
    }

    IVec2 oldMinChunkPosition = tileToChunkPosition(oldPos - oldSize.scaled(0.5f));
    IVec2 oldMaxChunkPosition = tileToChunkPosition(oldPos + oldSize.scaled(0.5f));
    IVec2 newMinChunkPosition = tileToChunkPosition(newPos - newSize.scaled(0.5f));
    IVec2 newMaxChunkPosition = tileToChunkPosition(newPos + newSize.scaled(0.5f));
    IVec2 minChunkPosition = {
        (oldMinChunkPosition.x < newMinChunkPosition.x) ? oldMaxChunkPosition.x : newMaxChunkPosition.x,
        (oldMinChunkPosition.y < newMinChunkPosition.y) ? oldMaxChunkPosition.y : newMaxChunkPosition.y
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

            if (inNewArea && !inOldArea) {
                // add entity to new chunk
                ChunkData* newChunkdata = chunkmap->getChunkData(chunkPosition);
                if (newChunkdata) {
                    newChunkdata->closeEntities.push_back(entity);
                }
            }

            if (inOldArea && !inNewArea) {
                // remove entity from old chunk
                ChunkData* oldChunkdata = chunkmap->getChunkData(chunkPosition);
                if (oldChunkdata) {
                    for (unsigned int e = 0; e < oldChunkdata->closeEntities.size(); e++) {
                        // TODO: try implementing binary search with sorting for faster removal
                        if (oldChunkdata->closeEntities[e].id == entity.id)
                            oldChunkdata->closeEntities.erase(oldChunkdata->closeEntities.begin() + e);
                    }
                }
            }
        }
    }
}