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
#include "Entity.hpp"

typedef Tile Chunk[CHUNKSIZE][CHUNKSIZE]; // 2d array of tiles

ChunkData::ChunkData(Chunk* chunk, IVec2 position) {
        if (chunk) {
            this->chunk = chunk;
        }
        this->position = position;
    }

int ChunkData::tilePositionX() const {
    return position.x * CHUNKSIZE;
}
int ChunkData::tilePositionY() const {
    return position.y * CHUNKSIZE;
}

void generateChunk(Chunk* chunk) {
    for (int row = 0; row < CHUNKSIZE; row++) {
        for (int col = 0; col < CHUNKSIZE; col++) {
            TileType tileType = TileTypes::Grass;
            //TILE(chunk, row, col) = Tile(randomInt(0, NUM_TILE_TYPES-1));
            TILE(chunk, row, col) = Tile(tileType);
        }
    }
}

ChunkPool::ChunkPool() {
    _size = 0;
    _used = 0;  
}

ChunkPool::ChunkPool(int size) {
    init(size);
}

void ChunkPool::init(int size) {
    this->_size = size;
    pool = (Chunk*)malloc(_size * sizeof(Chunk));
    _used = 0;
}

void ChunkPool::destroy() {
    free(pool);
}

int ChunkPool::used() const {
    return _used;
}

int ChunkPool::size() const {
    return _size;
}

bool ChunkPool::hasRoom() const {
    return _used < _size;
}

Chunk* ChunkPool::at(int index) {
    if (index >= _size) {
        index = _size - 1;
    }
    if (index < 0) {
        index = 0;
    }
    return &pool[index];
}

Chunk* ChunkPool::getNew() {
    if (_used < _size) {
        Chunk* newChunk = &pool[_used];
        _used++;
        return newChunk;
    }
    return NULL;
}

void ChunkMap::init() {
    // each pool holds a maximum of 10 chunks
    ChunkPool pool(CHUNKPOOL_SIZE); // starting pool
    chunkPools.push_back(pool);
}

void ChunkMap::destroy() {
    // free the new chunk data created in newEntry
    for (auto chunkData : chunkDataList) {
        delete chunkData;
    }

    // free all chunks (which are being held in the pools)
    for (auto pool : chunkPools) {
        pool.destroy();
    }
}

int ChunkMap::size() {
    return map.size();
}

/*
* Get a chunk data object from the map.
* Returns NULL if the chunk couldn't be found.
*/
ChunkData* ChunkMap::getChunkData(IVec2 chunkPosition) const {
    auto it = map.find(chunkPosition);
    if (it == map.end()) {
        // chunk not found
        return NULL;
    }
    return it->second;
}

/*
* Get a const chunk from the map.
* Returns NULL if the chunk couldn't be found.
*/
Chunk* ChunkMap::getChunk(IVec2 chunkPosition) const {
    auto it = map.find(chunkPosition);
    if (it == map.end()) {
        // chunk not found
        return NULL;
    }
    return it->second->chunk;
}

ChunkData* ChunkMap::createChunk(IVec2 position) {
    return newEntry(position);
}

void ChunkMap::iterateChunks(std::function<int(Chunk*)> callback) {
    // go through pools instead of going through the map to have better data locality
    for (auto& pool : chunkPools) {
        for (int c = 0; c < pool.used(); c++) {
            Chunk* chunk = pool.at(c);
            if (callback(chunk)) {
                return;
            }
        }
    }
}

void ChunkMap::iterateChunkdata(std::function<int(ChunkData*)> callback) {
    for (ChunkData* chunkdata : chunkDataList) {
        if (callback(chunkdata)) {
            return;
        }
    }
}

/*
* Create and allocate for a new ungenerated chunk at the position.
* @return The new chunk
*/
Chunk* ChunkMap::newChunk(IVec2 position) {
    ChunkPool& pool = chunkPools.back();
    if (!pool.hasRoom()) {
        // make new pool
        ChunkPool newPool(CHUNKPOOL_SIZE);
        chunkPools.push_back(newPool);
        return chunkPools.back().getNew();
    }

    return pool.getNew();
}

ChunkData* ChunkMap::newEntry(IVec2 position) {
    // Safety check to make sure chunk data is not overwritten / duplicated and stuff.
    // If the log warning never goes off, it might be okay to remove.
    {
        auto it = map.find(position);
        if (it != map.end()) {
            // this method was wrongly called, the entry already exists at the position,
            // abort making a new one to not cause memory leaks and other weird bugs.
            // also log it
            Log("Warning: ChunkMap::newEntry was called at an already existing position key. Position: (%d, %d) Aborting.", position.x, position.y);
            // just return the old entry
            return it->second;
        }
    }

    Chunk* chunk = newChunk(position);
    // This is pretty bad, it will spread all the memory everywhere,
    // this should be changed.
    ChunkData* chunkData = new ChunkData(chunk, position);
    chunkDataList.push_back(chunkData);
    map[position] = chunkData;

    return chunkData;
}

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

void GameState::init() {
    player = Player(0.0, 0.0);
    for (int s = 0; s < player.inventory.size; s++) {
        player.inventory.items[s] = ItemStack();
    }

    ItemStack startInventory[] = {
        {Items::SpaceFloor, 9999},
        {Items::Grass, 9999},
        {Items::Wall, 9999}
    };
    
    for (int i = 0; (i < (int)(sizeof(startInventory) / sizeof(ItemStack))) && (i < player.inventory.size); i++) {
        player.inventory.items[i] = startInventory[i];
    }

    player.heldItemStack = &player.inventory.items[0];
    player.selectedHotbarStack = 0;
    player.facingDirection = DirectionUp;

    chunkmap.init();
    int chunkRadius = 10;
    for (int chunkX = -chunkRadius; chunkX < chunkRadius; chunkX++) {
        for (int chunkY = -chunkRadius; chunkY < chunkRadius; chunkY++) {
            ChunkData* chunkdata = chunkmap.createChunk({chunkX, chunkY});
            generateChunk(chunkdata->chunk);
        }
    }

    for (int e = 0; e < 5000; e++) {
        Vec2 pos = {(float)randomInt(-200, 200), (float)randomInt(-200, 200)};
        Entity entity = Entities::Tree(&ecs, pos, {randomInt(1, 5) + rand0to1(), randomInt(1, 5) + rand0to1()});
    }
}

Tile* getTileAtPosition(ChunkMap& chunkmap, float x, float y) {
    int chunkX = (int)floor(x / CHUNKSIZE);
    int chunkY = (int)floor(y / CHUNKSIZE);
    Chunk* chunk = chunkmap.getChunk({chunkX, chunkY});
    if (!chunk) {
        return NULL;
    }
    int tileX = (int)floor(x - (chunkX * CHUNKSIZE));
    int tileY = (int)floor(y - (chunkY * CHUNKSIZE));
    return &(*chunk)[tileY][tileX];
}

Tile* getTileAtPosition(ChunkMap& chunkmap, Vec2 position) {
    return getTileAtPosition(chunkmap, position.x, position.y);
}

Tile* getTileAtPosition(ChunkMap& chunkmap, int x, int y) {
    return getTileAtPosition(chunkmap, (float)x, (float)y);
}

Tile* getTileAtPosition(ChunkMap& chunkmap, IVec2 tilePos) {
    return getTileAtPosition(chunkmap, (float)tilePos.x, (float)tilePos.y);
}

bool chunkIsVisible(IVec2 chunkPosition, const SDL_FRect *worldViewport) {
    float chunkRelativeMinX = chunkPosition.x * CHUNKSIZE - worldViewport->x;
    float chunkRelativeMinY = chunkPosition.y * CHUNKSIZE - worldViewport->y;

    float chunkRelativeMaxX = chunkRelativeMinX + CHUNKSIZE;
    float chunkRelativeMaxY = chunkRelativeMinY + CHUNKSIZE;

    return ((chunkRelativeMinX <= worldViewport->w * 1.5f && chunkRelativeMaxX >= 0) &&
            (chunkRelativeMinY <= worldViewport->h * 1.5f && chunkRelativeMaxY >= 0));
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