#include "Tiles.hpp"
#include "Chunks.hpp"
#include "global.hpp"

ChunkData::ChunkData(Chunk* chunk, IVec2 position) {
    this->chunk = chunk;
    this->position = position;
    this->closeEntities = My::Vec<Entity>(0);
}

void generateChunk(ChunkData* chunkdata) {
    if (!chunkdata) return;
    Chunk& chunk = *chunkdata->chunk;
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
            chunk[row][col] = Tile(tileType);
        }
    }
}

void ChunkMap::init() {
    map = InternalChunkMap::WithBuckets(32);
    chunkList = ChunkBucketArray::WithBuckets(1);
    chunkdataList = ChunkDataBucketArray::WithBuckets(1);
    
    // pre-cache atleast one chunk so there's no special cases when a chunk has not yet been cached
    cachedChunkCoord = IVec2{0, 0};
    auto chunkdata = newChunkAt(cachedChunkCoord);
    if (!chunkdata) {
        LogCrash(CrashReason::MemoryFail, "Failed to reserve new chunk in chunkmap");
    }
    cachedChunk = chunkdata->chunk;
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
    ChunkData** maybeChunkdata = map.lookup(chunkPosition);
    if (maybeChunkdata) return *maybeChunkdata;
    return nullptr;
    */

    return map.lookup(chunkPosition);
}

ChunkData* ChunkMap::newChunkAt(IVec2 position) {
    // Safety check to make sure chunk data is not overwritten / duplicated and stuff.
    // If the log warning never goes off, it might be okay to remove.
    {
        // unlikely to find it since it's not supposed to exist, so don't get()
        ChunkData* chunkdata = map.lookup(position);
        if (chunkdata) {
            // this method was wrongly called, the entry already exists at the position,
            // abort making a new one to not cause memory leaks and other weird bugs.
            // also log it
            LogWarn("There already exists an entry at the position (%d, %d), returning old entry...", position.x, position.y);
            return chunkdata; // return the old entry
        }
    }

    Chunk* chunk = chunkList.reserveBack();
    if (chunk) { // check for possible memory errors
        ChunkData* chunkdata = chunkdataList.reserveBack();
        if (chunkdata) {
            *chunkdata = ChunkData(chunk, position);
            map.insert(position, *chunkdata);
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
    ChunkData* chunkdata = get(position);
    if (chunkdata) return chunkdata;

    return newChunkAt(position);
}

Tile* getTileAtPosition(const ChunkMap& chunkmap, Vec2 position) {
    IVec2 chunkPosition = toChunkPosition(position);
    if (chunkPosition != chunkmap.cachedChunkCoord) {
        ChunkData* chunkdata = chunkmap.get(position);
        if (!chunkdata) return nullptr;
        chunkmap.cachedChunk = chunkdata->chunk;
        chunkmap.cachedChunkCoord = chunkPosition;
    }
    int tileX = (int)floor(position.x - (chunkPosition.x * CHUNKSIZE));
    int tileY = (int)floor(position.y - (chunkPosition.y * CHUNKSIZE));
    return &(*chunkmap.cachedChunk)[tileY][tileX]; // when chunkdata is not null, chunkdata->chunk should never be null
}

Tile* getTileAtPosition(const ChunkMap& chunkmap, IVec2 position) {
    IVec2 chunkPosition = toChunkPosition(position);
    if (chunkPosition != chunkmap.cachedChunkCoord) {
        ChunkData* chunkdata = chunkmap.get(chunkPosition);
        if (!chunkdata) return nullptr;  
        chunkmap.cachedChunk = chunkdata->chunk;
        chunkmap.cachedChunkCoord = chunkPosition;
    }
    // when chunkdata is not null, chunkdata->chunk should never be null
    int tileX = position.x - chunkPosition.x * CHUNKSIZE;
    int tileY = position.y - chunkPosition.y * CHUNKSIZE;
    return &(*chunkmap.cachedChunk)[tileY][tileX];
}

int getTiles(const ChunkMap& chunkmap, const IVec2* inTiles, Tile* outTiles, int count) {
    int numMissedTiles = 0;
    for (int i = 0; i < count; i++) {
        TileCoord tileCoord = inTiles[i];
        ChunkCoord chunkCoord = toChunkPosition(tileCoord);
        if (chunkCoord != chunkmap.cachedChunkCoord) {
            auto chunkdata = chunkmap.get(chunkCoord);
            if (chunkdata) {
                chunkmap.cachedChunk = chunkdata->chunk;
                chunkmap.cachedChunkCoord = chunkCoord;
            } else {
                // set null tile cause chunk didn't exist
                outTiles[i] = Tile(TileTypes::Empty);
                numMissedTiles++;
                continue;
            }
        }
        outTiles[i] = (*chunkmap.cachedChunk)[tileCoord.y - chunkCoord.y * CHUNKSIZE][chunkCoord.x - chunkCoord.x * CHUNKSIZE];
    }

    return count - numMissedTiles;
}

bool chunkIsVisible(IVec2 chunkPosition, const SDL_FRect *worldViewport) {
    float chunkRelativeMinX = chunkPosition.x * CHUNKSIZE - worldViewport->x;
    float chunkRelativeMinY = chunkPosition.y * CHUNKSIZE - worldViewport->y;

    float chunkRelativeMaxX = chunkRelativeMinX + CHUNKSIZE;
    float chunkRelativeMaxY = chunkRelativeMinY + CHUNKSIZE;

    return ((chunkRelativeMinX <= worldViewport->w * 1.5f && chunkRelativeMaxX >= 0) &&
            (chunkRelativeMinY <= worldViewport->h * 1.5f && chunkRelativeMaxY >= 0));
}