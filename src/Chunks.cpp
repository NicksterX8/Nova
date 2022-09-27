#include "Tiles.hpp"
#include "Chunks.hpp"

#include "debugAllocStart.hpp"

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
    // each pool holds a maximum of 10 chunks
    chunks = ChunkBucketArray::WithBuckets(1);
    chunkDataList = {0,0,0};
}

void ChunkMap::destroy() {
    // free the new chunk data created in newEntry
    for (ChunkData* chunkData : chunkDataList) {
        delete chunkData;
    }
    chunkDataList.destroy();

    chunks.destroy();
}

size_t ChunkMap::size() const {
    return map.size();
}

/*
* Get a chunk data object from the map.
* Returns NULL if the chunk couldn't be found.
*/
ChunkData* ChunkMap::get(IVec2 chunkPosition) const {
    auto it = map.find(chunkPosition);
    if (it == map.end()) {
        // chunk not found
        return nullptr;
    }
    return it->second;
}

int ChunkMap::iterateChunks(std::function<int(Chunk*)> callback) const {
    // go through pools instead of going through the map to have better data locality
    for (auto& chunk : chunks) {
        int ret = callback(&chunk);
        if (ret) {
            return ret;
        }
    }

    return 0;
}

int ChunkMap::iterateChunkdata(std::function<int(ChunkData*)> callback) const {
    for (int i = 0; i < chunkDataList.size; i++) {
        int ret = callback(chunkDataList[i]);
        if (ret) {
            return ret;
        }
    }
    return 0;
}

ChunkData* ChunkMap::newChunkAt(IVec2 position) {
    // Safety check to make sure chunk data is not overwritten / duplicated and stuff.
    // If the log warning never goes off, it might be okay to remove.
    {
        auto it = map.find(position);
        if (it != map.end()) {
            // this method was wrongly called, the entry already exists at the position,
            // abort making a new one to not cause memory leaks and other weird bugs.
            // also log it
            LogWarn("There already exists an entry at the position (%d, %d), returning old entry...", position.x, position.y);
            // just return the old entry
            return it->second;
        }
    }

    Chunk* chunk = chunks.reserveBack();
    if (chunk) {
        // TODO: This is pretty bad, it will spread all the memory everywhere,
        // this should be changed. 
        ChunkData* chunkdata = new ChunkData(chunk, position);
        if (chunkdata) {
            if (chunkDataList.push(chunkdata)) {
                map[position] = chunkdata;
                return chunkdata;
            };
        }
    }
    
    LogWarn("Failed to reserve a new chunk for chunk position (%d, %d).", position.x, position.y);
    return nullptr;
}

ChunkData* ChunkMap::getOrMakeNew(IVec2 position) {
    // newChunkAt already does this but it produces a warning so we just do it here to not produce a warning
    auto it = map.find(position);
    if (it != map.end()) {
        return it->second;
    }

    return newChunkAt(position);
}

Tile* getTileAtPosition(const ChunkMap& chunkmap, Vec2 position) {
    int chunkX = (int)floor(position.x / CHUNKSIZE);
    int chunkY = (int)floor(position.y / CHUNKSIZE);
    ChunkData* chunkdata = chunkmap.get({chunkX, chunkY});
    if (chunkdata) {
        int tileX = (int)floor(position.x - (chunkX * CHUNKSIZE));
        int tileY = (int)floor(position.y - (chunkY * CHUNKSIZE));
        return &((*chunkdata->chunk))[tileY][tileX]; // when chunkdata is not null, chunkdata->chunk should never be null
    }
    return nullptr;
}

Tile* getTileAtPosition(const ChunkMap& chunkmap, IVec2 position) {
    int chunkX,remainderX, chunkY,remainderY;

    chunkX = position.x / CHUNKSIZE; 
    remainderX = position.x % CHUNKSIZE;
    chunkX -= (remainderX < 0);

    chunkY = position.y / CHUNKSIZE; 
    remainderY = position.y % CHUNKSIZE;
    chunkY -= (remainderY < 0);

    ChunkData* chunkdata = chunkmap.get({chunkX, chunkY});
    if (chunkdata) {
        return &((*chunkdata->chunk))[remainderY][remainderX]; // when chunkdata is not null, chunkdata->chunk should never be null
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