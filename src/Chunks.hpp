#ifndef CHUNKS_INCLUDED
#define CHUNKS_INCLUDED

#include <vector>
#include "My/Vec.hpp"
#include "My/BucketArray.hpp"
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
    My::Vec<Entity> closeEntities; // entities that are at least partially inside the chunk
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

#define CHUNK_BUCKET_SIZE 512
#define CHUNKDATA_BUCKET_SIZE 512

class ChunkMap {
    struct KeyHash {
        size_t operator()(const IVec2& point) const {
            size_t hash = std::hash<int>()(((point.x - 32768) << 16) + (point.y - 32768));
            return hash;
        }
    };
    using ChunkUnorderedMap = std::unordered_map<IVec2, ChunkData*, KeyHash>;
    using ChunkBucketArray = My::BucketArray<Chunk, CHUNK_BUCKET_SIZE>;
    using ChunkDataBucketArray = My::BucketArray<ChunkData, CHUNKDATA_BUCKET_SIZE>;

    ChunkUnorderedMap map;
    ChunkBucketArray chunks;
    ChunkDataBucketArray chunkdataList;
public:
    void init();
    void destroy();
    size_t size() const;

    /*
    * Get chunk data from the map for the given chunk position key.
    * Returns NULL if the chunk couldn't be found.
    */
    ChunkData* get(IVec2 chunkPosition) const;

    ChunkData* newChunkAt(IVec2 position);

    /*
    * Like get() except will create a new chunk if the chunk couldn't be found.
    * The chunk will not be generated, so this shouldn't be used most in most cases.
    */
    ChunkData* getOrMakeNew(IVec2 position);
    /*
    * @return The value returned from the final callback
    */
    int iterateChunks(std::function<int(Chunk*)> callback) const;

    /*
    * @return The value returned from the final callback
    */
    int iterateChunkdata(std::function<int(ChunkData*)> callback) const;

};

inline IVec2 toChunkPosition(Vec2 position) {
    return vecFloori(position / (float)CHUNKSIZE);
}

Tile* getTileAtPosition(const ChunkMap& chunkmap, Vec2 position);

Tile* getTileAtPosition(const ChunkMap& chunkmap, IVec2 position);

bool chunkIsVisible(IVec2 chunkPosition, const SDL_FRect *worldViewport);

#endif