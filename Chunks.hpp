#ifndef CHUNKS_INCLUDED
#define CHUNKS_INCLUDED

#include <vector>
#include <unordered_map>
#include "Tiles.hpp"
#include "constants.hpp"
#include "NC/cpp-vectors.hpp"
#include "ECS/Entity.hpp"

using ECS::Entity;

typedef Tile Chunk[CHUNKSIZE][CHUNKSIZE]; // 2d array of tiles

struct ChunkData {
    IVec2 position; // chunk position aka floor(tilePosition / CHUNKSIZE), NOT tile position
    std::vector<Entity> closeEntities; // entities that are at least partially inside the chunk
    Chunk* chunk; // pointer to chunk tiles

    ChunkData(Chunk* chunk, IVec2 position);

    int tilePositionX() const; // Get the X position of this chunk relative to tiles' positions
    int tilePositionY() const; // Get the Y position of this chunk relative to tiles' positions

    inline bool removeCloseEntity(Entity entity) {
        for (unsigned int e = 0; e < closeEntities.size(); e++) {
            // TODO: try implementing binary search with sorting for faster removal
            if (closeEntities[e].id == entity.id) {
                closeEntities.erase(closeEntities.begin() + e);
                return true;
            }
        }
        return false;
    }
};

#define CHUNK_TILE(chunk_ptr, row, col) ((*chunk_ptr)[row][col])
#define CHUNK_TILE_INDEX(chunk_ptr, index) ((&(*chunk_ptr)[0][0])[index])

void generateChunk(Chunk* chunk);

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

#define CHUNKPOOL_SIZE 100

typedef std::unordered_map<IVec2, ChunkData*, IVec2::Hash> ChunkUnorderedMap;
class ChunkMap {
    ChunkUnorderedMap map;
    std::vector<ChunkPool> chunkPools;
    std::vector<ChunkData*> chunkDataList;
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
    * @return The new chunk
    */
    Chunk* newChunk(IVec2 position);
    ChunkData* newEntry(IVec2 position);
};

IVec2 tileToChunkPosition(Vec2 position);

Tile* getTileAtPosition(const ChunkMap& chunkmap, float x, float y);
Tile* getTileAtPosition(const ChunkMap& chunkmap, Vec2 position);
Tile* getTileAtPosition(const ChunkMap& chunkmap, int x, int y);
Tile* getTileAtPosition(const ChunkMap& chunkmap, IVec2 tilePos);

bool chunkIsVisible(IVec2 chunkPosition, const SDL_FRect *worldViewport);

#endif