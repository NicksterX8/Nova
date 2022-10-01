#include "Tiles.hpp"
#include "Chunks.hpp"

ChunkData::ChunkData(Chunk* chunk, IVec2 position) {
    this->chunk = chunk;
    this->position = position;
    this->closeEntities = My::Vec<Entity>(0);
    this->vbo = 0;
}

void generateChunk(Chunk* chunk) {
    for (int row = 0; row < CHUNKSIZE; row++) {
        for (int col = 0; col < CHUNKSIZE; col++) {
            TileType tileType = TileTypes::Grass;
            switch(rand() % 4) {
            case 0:
                tileType = TileTypes::Sand;
                break;
            case 1:
                tileType = TileTypes::Grass;
                break;
            case 2:
                tileType = TileTypes::Wall;
                break;
            default:
                tileType = TileTypes::Empty;
                break;
            }
            CHUNK_TILE(chunk, row, col) = Tile(tileType);
        }
    }
}

void ChunkMap::init() {
    map = InternalChunkMap::WithBuckets(32);
    chunkList = ChunkBucketArray::WithBuckets(1);
    chunkdataList = ChunkDataBucketArray::WithBuckets(1);
}

void ChunkMap::destroy() {
    chunkdataList.destroy();
    chunkList.destroy();
    map.destroy();
}

/*
* Get a chunk data object from the map.
* Returns NULL if the chunk couldn't be found.
*/
ChunkData* ChunkMap::get(IVec2 chunkPosition) const {
    /*
    auto it = map.find(chunkPosition);
    if (it == map.end()) {
        // chunk not found
        return nullptr;
    }
    return it->second;
    */

    ChunkData** maybeChunkdata = map.lookup(chunkPosition);
    if (maybeChunkdata) return *maybeChunkdata;
    return nullptr;
}

ChunkData* ChunkMap::newChunkAt(IVec2 position) {
    // Safety check to make sure chunk data is not overwritten / duplicated and stuff.
    // If the log warning never goes off, it might be okay to remove.
    {
        ChunkData** chunkdata = map.lookup(position);
        if (chunkdata) {
            // this method was wrongly called, the entry already exists at the position,
            // abort making a new one to not cause memory leaks and other weird bugs.
            // also log it
            LogWarn("There already exists an entry at the position (%d, %d), returning old entry...", position.x, position.y);
            return *chunkdata; // return the old entry
        }
    }

    Chunk* chunk = chunkList.reserveBack();
    if (chunk) { // check for possible memory errors
        ChunkData* chunkdata = chunkdataList.reserveBack();
        if (chunkdata) {
            *chunkdata = ChunkData(chunk, position);
            map.insert(position, chunkdata);
            return chunkdata;
        } else {
            // undo the reserve back
            chunkList.pop();
        }
    }
    
    LogWarn("Failed to reserve a new chunk for chunk position (%d, %d).", position.x, position.y);
    return nullptr;
}

ChunkData* ChunkMap::getOrMakeNew(IVec2 position) {
    // newChunkAt already does this but it produces a warning so we just do it here to not produce a warning.
    /*
    auto it = map.find(position);
    if (it != map.end()) {
        return it->second;
    }
    */
    ChunkData** chunkdata = map.lookup(position);
    if (chunkdata) return *chunkdata;

    return newChunkAt(position);
}

Tile* getTileAtPosition(const ChunkMap& chunkmap, Vec2 position) {
    const float chunkSize = (float)CHUNKSIZE;
    int chunkX = (int)floor(position.x / chunkSize);
    int chunkY = (int)floor(position.y / chunkSize);
    ChunkData** chunkdata = chunkmap.map.lookup({chunkX, chunkY});
    if (chunkdata) {
        int tileX = (int)floor(position.x - chunkX * chunkSize);
        int tileY = (int)floor(position.y - chunkY * chunkSize);
        return &((*(*chunkdata)->chunk))[tileY][tileX]; // when chunkdata is not null, chunkdata->chunk should never be null
    }
    return nullptr;
}

Tile* getTileAtPosition(const ChunkMap& chunkmap, IVec2 position) {
    int chunkX,remainderX;
    int chunkY,remainderY;

    remainderX = position.x % CHUNKSIZE;
    chunkX = position.x / CHUNKSIZE - (remainderX < 0); 

    remainderY = position.y % CHUNKSIZE;
    chunkY = position.y / CHUNKSIZE - (remainderY < 0); 

    ChunkData** chunkdata = chunkmap.map.lookup({chunkX, chunkY});
    if (chunkdata) {
        //return &((*(*chunkdata)->chunk))[remainderY][remainderX]; // when chunkdata is not null, chunkdata->chunk should never be null
        return &CHUNK_TILE((*chunkdata)->chunk, position.y - chunkY * CHUNKSIZE, position.x - chunkX * CHUNKSIZE);
    }
    return nullptr;
}

bool chunkIsVisible(IVec2 chunkPosition, const SDL_FRect *worldViewport) {
    float chunkRelativeMinX = chunkPosition.x * CHUNKSIZE - worldViewport->x;
    float chunkRelativeMinY = chunkPosition.y * CHUNKSIZE - worldViewport->y;

    float chunkRelativeMaxX = chunkRelativeMinX + CHUNKSIZE;
    float chunkRelativeMaxY = chunkRelativeMinY + CHUNKSIZE;

    return ((chunkRelativeMinX <= worldViewport->w * 1.5f && chunkRelativeMaxX >= 0) &&
            (chunkRelativeMinY <= worldViewport->h * 1.5f && chunkRelativeMaxY >= 0));
}