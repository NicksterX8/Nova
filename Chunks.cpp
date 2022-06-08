
#include "Tiles.hpp"
#include "Chunks.hpp"

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