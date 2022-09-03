
#include "Tiles.hpp"
#include "Chunks.hpp"

ChunkData::ChunkData(Chunk* chunk, IVec2 position) {
    this->chunk = chunk;
    this->position = position;
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

size_t ChunkPool::used() const {
    return _used;
}

size_t ChunkPool::size() const {
    return _size;
}

bool ChunkPool::hasRoom() const {
    return _used < _size;
}

Chunk* ChunkPool::at(size_t index) const {
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

size_t ChunkMap::size() const {
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
* Get a chunk from the map.
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

int ChunkMap::iterateChunks(std::function<int(Chunk*)> callback) const {
    // go through pools instead of going through the map to have better data locality
    for (auto& pool : chunkPools) {
        for (size_t c = 0; c < pool.used(); c++) {
            Chunk* chunk = pool.at(c);
            int ret = callback(chunk);
            if (ret) {
                return ret;
            }
        }
    }
    return 0;
}

int ChunkMap::iterateChunkdata(std::function<int(ChunkData*)> callback) const {
    for (ChunkData* chunkdata : chunkDataList) {
        int ret = callback(chunkdata);
        if (ret) {
            return ret;
        }
    }
    return 0;
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

ChunkData* ChunkMap::getOrCreateAt(IVec2 position) {
    auto it = map.find(position);
    if (it != map.end()) {
        return it->second;
    }

    Chunk* chunk = newChunk(position);
    // This is pretty bad, it will spread all the memory everywhere,
    // this should be changed.
    ChunkData* chunkData = new ChunkData(chunk, position);
    chunkDataList.push_back(chunkData);
    map[position] = chunkData;
    return chunkData;
}

Tile* getTileAtPosition(const ChunkMap& chunkmap, Vec2 position) {
    int chunkX = (int)floor(position.x / CHUNKSIZE);
    int chunkY = (int)floor(position.y / CHUNKSIZE);
    Chunk* chunk = chunkmap.getChunk({chunkX, chunkY});
    if (!chunk) {
        return NULL;
    }
    int tileX = (int)floor(position.x - (chunkX * CHUNKSIZE));
    int tileY = (int)floor(position.y - (chunkY * CHUNKSIZE));
    return &(*chunk)[tileY][tileX];
}

bool chunkIsVisible(IVec2 chunkPosition, const SDL_FRect *worldViewport) {
    float chunkRelativeMinX = chunkPosition.x * CHUNKSIZE - worldViewport->x;
    float chunkRelativeMinY = chunkPosition.y * CHUNKSIZE - worldViewport->y;

    float chunkRelativeMaxX = chunkRelativeMinX + CHUNKSIZE;
    float chunkRelativeMaxY = chunkRelativeMinY + CHUNKSIZE;

    return ((chunkRelativeMinX <= worldViewport->w * 1.5f && chunkRelativeMaxX >= 0) &&
            (chunkRelativeMinY <= worldViewport->h * 1.5f && chunkRelativeMaxY >= 0));
}