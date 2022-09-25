#ifndef CHUNKS_INCLUDED
#define CHUNKS_INCLUDED

#include <vector>
#include "My/Vector.hpp"
#include <unordered_map>
#include "Tiles.hpp"
#include "constants.hpp"
#include "utils/Vectors.hpp"
#include "ECS/Entity.hpp"

using ECS::Entity;

typedef Tile Chunk[CHUNKSIZE][CHUNKSIZE]; // 2d array of tiles

struct ChunkData {
    Chunk* chunk; // pointer to chunk tiles
    IVec2 position; // chunk position aka floor(tilePosition / CHUNKSIZE), NOT tile position
    My::Vector<Entity> closeEntities; // entities that are at least partially inside the chunk
    unsigned int vbo;

    ChunkData(Chunk* chunk, IVec2 position);

    int tileX() const {return position.x * CHUNKSIZE;} // Get the X position of this chunk relative to tiles' positions
    int tileY() const {return position.y * CHUNKSIZE;} // Get the Y position of this chunk relative to tiles' positions
    Vec2 tilePosition() const {
        return position * CHUNKSIZE;
    }

    inline bool removeCloseEntity(Entity entity) {
        for (unsigned int e = 0; e < closeEntities.size; e++) {
            // TODO: try implementing binary search with sorting for faster removal
            if (closeEntities[e].id == entity.id) {
                closeEntities.remove(e);
                return true;
            }
        }
        return false;
    }
};

// Get the tile from the chunk at the specified row and column.
#define CHUNK_TILE(chunk_ptr, row, col) ((*chunk_ptr)[row][col])
// Get the tile from the chunk at the index, row ordered
#define CHUNK_TILE_INDEX(chunk_ptr, index) ((&(*chunk_ptr)[0][0])[index])

void generateChunk(Chunk* chunk);

/*
class ChunkPool {
    size_t _size;
    size_t _used;
    Chunk* pool;
public:
    ChunkPool();

    ChunkPool(int size);

    void init(int size);

    void destroy();

    size_t used() const;

    size_t size() const;

    bool hasRoom() const;

    Chunk* at(size_t index) const;

    Chunk* getNew();
}; 
*/

#define CHUNKPOOL_SIZE 256

struct IVec2Hash {
    size_t operator()(const IVec2& point) const {
        size_t hash = std::hash<int>()(((point.x - 32768) << 16) + (point.y - 32768));
        return hash;
    }
};

using ChunkPool = My::Vector<Chunk>;

typedef std::unordered_map<IVec2, ChunkData*, IVec2Hash> ChunkUnorderedMap;
class ChunkMap {
    ChunkUnorderedMap map;
    My::Vector< My::Vector<Chunk> > chunkPools;
    My::Vector<ChunkData*> chunkDataList;
public:
    void init();
    void destroy();
    size_t size() const;

    /*
    * Get a chunk data object from the map.
    * Returns NULL if the chunk couldn't be found.
    */
    ChunkData* getChunkData(IVec2 chunkPosition) const;

    /*
    * Like getChunkData except will create a new chunk if the chunk couldn't be found.
    * The chunk will not be generated, so this shouldn't be used most in most cases.
    */
    ChunkData* getOrCreateAt(IVec2 position);

    /*
    * Get a const chunk from the map.
    * Returns NULL if the chunk couldn't be found.
    */
    Chunk* getChunk(IVec2 chunkPosition) const;
    ChunkData* createChunk(IVec2 position);
    /*
    * @return The value returned from the final callback
    */
    int iterateChunks(std::function<int(Chunk*)> callback) const;

    /*
    * @return The value returned from the final callback
    */
    int iterateChunkdata(std::function<int(ChunkData*)> callback) const;

    
private:
    /*
    * Create and allocate for a new ungenerated chunk at the position.
    * @return A new allocated chunk, except in cases of memory allocation failure
    */
    Chunk* newChunk(IVec2 position);
    ChunkData* newEntry(IVec2 position);
};

inline IVec2 toChunkPosition(Vec2 position) {
    return vecFloori(position / (float)CHUNKSIZE);
}

Tile* getTileAtPosition(const ChunkMap& chunkmap, Vec2 position);

inline Tile* getTileAtPosition(const ChunkMap& chunkmap, IVec2 tilePos) {
    return getTileAtPosition(chunkmap, Vec2(tilePos.x, tilePos.y));
}

bool chunkIsVisible(IVec2 chunkPosition, const SDL_FRect *worldViewport);

#endif