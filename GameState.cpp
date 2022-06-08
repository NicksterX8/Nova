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
#include "Entities/Systems/Rendering.hpp"
#include "Entities/Systems/Systems.hpp"

/*
std::vector<Entity>& entitiesSpatialSearch(const ChunkMap* chunkmap, Vec2 pos, float radius) {
    radius = abs(radius);
    float radiusSqrd = radius * radius;

    // form rectangle of chunks to search for entities in
    IVec2 highChunkBound = {(int)floor((pos.x + radius) / CHUNKSIZE), (int)floor((pos.y + radius) / CHUNKSIZE)};
    IVec2 lowChunkBound =  {(int)floor((pos.x - radius) / CHUNKSIZE), (int)floor((pos.y - radius) / CHUNKSIZE)};

    // int chunksWidth =  (highChunkBound.x - lowChunkBound.x);
    // int chunksHeight = (highChunkBound.y - lowChunkBound.y);

    // int nNearbyChunks = chunksWidth * chunksHeight;
    
    std::vector<Entity> entities; // entities found in spatial search

    // go through each chunk in chunk search rectangle
    for (int y = lowChunkBound.y; y < highChunkBound.y; y++) {
        for (int x = lowChunkBound.x; x < highChunkBound.x; x++) {
            const ChunkData* chunkdata = chunkmap->getChunkData({x, y});
            if (chunkdata == NULL) {
                // entities can't be in non-existent chunks
                continue;
            }

            for (int e = 0; e < chunkdata->closeEntities.size(); e++) {
                Entity closeEntity = chunkdata->closeEntities[e];
                Vec2 entityPosition = {0, 0}; // this should be getting position component
                float distSqrd = entityPosition.x * entityPosition.x + entityPosition.y * entityPosition.y;
                if (distSqrd < radiusSqrd) {
                    // entity is within radius
                    entities.push_back(closeEntity);
                }
            }
        }
    }

    return entities;
}
*/

//void foreachEntityInRange(ECS* ecs, Vec2 point, float distance, std::function<void(Entity)> callback) {   
//}

void GameState::free() {
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

    auto renderSystem = new RenderSystem(renderer, gameViewport);
    ecs.NewSystem<RenderSystem>(renderSystem);
    ecs.NewSystems<SYSTEMS>();
    ecs.testInitialization();
    SpecialEntityStatic::Init(&ecs);



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

bool pointIsOnTileEntity(ECS* ecs, Entity tileEntity, IVec2 tilePosition, Vec2 point) {
    // check if the click was actually on the entity.
    bool clickedOnEntity = true;
    // if the entity doesnt have size data, assume the click was on the entity I guess,
    // since you cant do checks otherwise.
    // Kind of undecided whether the player should be able to click on an entity with no size
    auto size = ecs->Get<SizeComponent>(tileEntity);
    if (size) {
        // if there isn't a position component, assume the position is the top left corner of the tile
        Vec2 position = Vec2(tilePosition.x, tilePosition.y);
        auto positionComponent = ecs->Get<PositionComponent>(tileEntity);
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

bool findTileEntityAtPosition(GameState* state, Vec2 position, Entity* foundEntity) {
    IVec2 tilePosition = position.floorToIVec();
    Tile* selectedTile = getTileAtPosition(state->chunkmap, tilePosition);
    if (selectedTile) {
        Entity tileEntity = selectedTile->entity;
        if (state->ecs.EntityLives(tileEntity)) {
            if (pointIsOnTileEntity(&state->ecs, tileEntity, tilePosition, position)) {

                if (foundEntity)
                    *foundEntity = tileEntity;
                return true;
            }
        }
        
    }

    // no entity could be found at that position
    return false;
}

void removeEntityOnTile(ECS* ecs, Tile* tile) {
    ecs->Remove<COMPONENTS>(tile->entity);
    tile->entity.id = NULL_ENTITY;
}

void placeEntityOnTile(ECS* ecs, Tile* tile, Entity entity) {
    if (!ecs->EntityLives(tile->entity)) {
        tile->entity = entity;
    }
}

void useGrenade(GameState* state, Vec2 playerAim) {
    
}