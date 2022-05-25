#ifndef GAME_STATE_INCLUDED
#define GAME_STATE_INCLUDED

#include <unordered_map>
#include <functional>
#include <array>
#include <vector>

#include <SDL2/SDL.h>
#include "constants.hpp"
#include "Textures.hpp"
#include "NC/cpp-vectors.hpp"
#include "NC/utils.h"
#include "Tiles.hpp"
#include "Player.hpp"
#include "Entity.hpp"

typedef Tile Chunk[CHUNKSIZE][CHUNKSIZE]; // 2d array of tiles

struct ChunkData {
    IVec2 position; // chunk position aka floor(tilePosition / CHUNKSIZE), NOT tile position
    std::vector<Entity> closeEntities;
    Chunk* chunk; // pointer to chunk tiles

    ChunkData(Chunk* chunk, IVec2 position);

    int tilePositionX() const;
    int tilePositionY() const;
};

#define TILE(chunk, row, col) ((*chunk)[row][col])
#define TILE_BY_INDEX(chunk, index) (((Tile*)chunk)[index])

void generateChunk(Chunk* chunk);

class ChunkPool {
    int _size;
    int _used;
    Chunk* pool;
public:
    ChunkPool();

    ChunkPool(int size);

    void init(int size);

    void destroy();

    int used() const;

    int size() const;

    bool hasRoom() const;

    Chunk* at(int index);

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
    int size();

    /*
    * Get a chunk data object from the map.
    * Returns NULL if the chunk couldn't be found.
    */
    ChunkData* getChunkData(IVec2 chunkPosition) const;
    /*
    * Get a const chunk from the map.
    * Returns NULL if the chunk couldn't be found.
    */
    Chunk* getChunk(IVec2 chunkPosition) const;
    ChunkData* createChunk(IVec2 position);
    void iterateChunks(std::function<int(Chunk*)> callback);
    void iterateChunkdata(std::function<int(ChunkData*)> callback);
private:
    /*
    * Create and allocate for a new ungenerated chunk at the position.
    * @return The new chunk
    */
    Chunk* newChunk(IVec2 position);
    ChunkData* newEntry(IVec2 position);
};

struct GameState {
    ChunkMap chunkmap;
    Player player;
    ECS ecs;

    void free();
    void init();
};

Tile* getTileAtPosition(ChunkMap& chunkmap, float x, float y);
Tile* getTileAtPosition(ChunkMap& chunkmap, Vec2 position);
Tile* getTileAtPosition(ChunkMap& chunkmap, int x, int y);
Tile* getTileAtPosition(ChunkMap& chunkmap, IVec2 tilePos);

bool chunkIsVisible(IVec2 chunkPosition, const SDL_FRect *worldViewport);

/*
* Get a line of tile coordinates from start to end, using DDA.
* Returns a line of size lineSize that must be freed using free()
* @param lineSize a pointer to an int to be filled in the with size of the line.
*/
IVec2* worldLine(Vec2 start, Vec2 end, int* lineSize);

void worldLineAlgorithm(Vec2 start, Vec2 end, std::function<int(IVec2)> callback);

bool pointIsOnTileEntity(ECS* ecs, Entity tileEntity, IVec2 tilePosition, Vec2 point);

bool findTileEntityAtPosition(GameState* state, Vec2 position, Entity* foundEntity);

#endif