#ifndef CHUNKS_INCLUDED
#define CHUNKS_INCLUDED

#include "My/Vec.hpp"
#include "My/BucketArray.hpp"
#include "My/HashMap.hpp"
#include <unordered_map>
#include "Tiles.hpp"
#include "constants.hpp"
#include "utils/vectors.hpp"
#include "ECS/Entity.hpp"
#include "utils/Metadata.hpp"

using ECS::Entity;

typedef Tile Chunk[CHUNKSIZE][CHUNKSIZE]; // 2d array of tiles
typedef IVec2 TileCoord;
typedef IVec2 ChunkCoord;

struct ChunkData {
    Chunk* chunk; // pointer to chunk tiles. // when this is not null, chunkdata->chunk should never be null
    ChunkCoord position; // chunk position aka floor(tilePosition / CHUNKSIZE), NOT tile position
    My::Vec<Entity> closeEntities; // entities that are at least partially inside the chunk

    ChunkData(Chunk* chunk, IVec2 position);

    int tileX() const {return position.x * CHUNKSIZE;} // Get the X position of this chunk relative to tiles' positions
    int tileY() const {return position.y * CHUNKSIZE;} // Get the Y position of this chunk relative to tiles' positions
    Vec2 tilePosition() const {
        return position * CHUNKSIZE;
    }

    bool removeCloseEntity(Entity entity) {
        for (unsigned int e = 0; e < closeEntities.size; e++) {
            // TODO: try implementing binary search with sorting for faster removal
            if (closeEntities[e].id == entity.id) {
                closeEntities.remove((int)e);
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

inline IVec2 toChunkPosition(Vec2 position) {
    const float chunkSize = (float)CHUNKSIZE;
    return {
        (int)floor(position.x / chunkSize),
        (int)floor(position.y / chunkSize)
    };
}

inline IVec2 toChunkPosition(IVec2 position) {
    return {
        position.x / CHUNKSIZE - (position.x % CHUNKSIZE < 0), 
        position.y / CHUNKSIZE - (position.y % CHUNKSIZE < 0)
    };
}

void generateChunk(ChunkData* chunk);

#define CHUNK_BUCKET_SIZE 512
#define CHUNKDATA_BUCKET_SIZE 512

struct ChunkMap {
    struct KeyHash {
        My::Map::Hash operator()(const IVec2& point) const {
            //return std::hash<int>()(((point.x - 32768) << 16) + (point.y - 32768));
            
            constexpr size_t hashTypeHalfBits = sizeof(My::Map::Hash) * CHAR_BIT / 2;
            static_assert(sizeof(point) == sizeof(My::Map::Hash), "point fits into hash");
            /*
            // y coordinate is most significant 32 bits, x least significant 32 bits
            return ((size_t)point.y << hashTypeHalfBits) | ((size_t)point.x);
            */
            //return std::hash<size_t>()(*(size_t*)&point);
            if (Metadata && Metadata->seconds() < 12.0) {
                constexpr std::hash<int> hasher;
                //return hasher(point.y) ^ hasher(point.x);
                My::Map::Hash hash;
                hash = (hasher(point.y - (1 << 31)) << 32) | hasher(point.x - (1 << 31));
                return hash;
            } else if (Metadata && Metadata->seconds() > 12.0 && Metadata->seconds() < 22.0) {
                return ((size_t)point.y << hashTypeHalfBits) | ((size_t)point.x);
            } else {
                return ((size_t)point.y << hashTypeHalfBits) | ((size_t)point.x);
            }
        }
    };
    using InternalChunkMap = My::HashMap<IVec2, ChunkData, KeyHash>;
    using ChunkBucketArray = My::BucketArray<Chunk, CHUNK_BUCKET_SIZE>;
    // TODO: try to use a hybrid map so we dont need this bucket array
    using ChunkDataBucketArray = My::BucketArray< ChunkData, CHUNKDATA_BUCKET_SIZE, Mem::AlignmentAllocator<sizeof(ChunkData)> >;

    InternalChunkMap map;
    ChunkBucketArray chunkList;
    ChunkDataBucketArray chunkdataList;

    mutable Chunk* cachedChunk;
    mutable ChunkCoord cachedChunkCoord;

    /* Methods */

    void init();
    void destroy();
    inline size_t size() const { return map.size; }

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
};

Tile* getTileAtPosition(const ChunkMap& chunkmap, Vec2 position);

Tile* getTileAtPosition(const ChunkMap& chunkmap, IVec2 position);

int getTiles(const ChunkMap& chunkmap, const IVec2* inTiles, Tile* outTiles, int count);

bool chunkIsVisible(IVec2 chunkPosition, const SDL_FRect *worldViewport);

#endif