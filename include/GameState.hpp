#ifndef GAME_STATE_INCLUDED
#define GAME_STATE_INCLUDED

#include <functional>
#include "llvm/BitVector.h"

#include "constants.hpp"
#include "utils/vectors.hpp"

#include "Tiles.hpp"
#include "Chunks.hpp"
#include "ECS/EntityWorld.hpp"
#include "Player.hpp"

struct GameState {
    ChunkMap chunkmap;
    EntityWorld ecs;
    Player player;
    ItemManager itemManager;

    void init(const TextureManager* textureManager);
    void destroy();
};

/*
* Get a line of the whole number x,y pairs on the path from start to end, using DDA.
*/
llvm::SmallVector<IVec2> raytraceDDA(Vec2 start, Vec2 end);

void worldLineAlgorithm(Vec2 start, Vec2 end, const std::function<int(IVec2)>& callback);

inline bool pointInEntity(Vec2 point, Entity entity, ComponentManager<const EC::ViewBox, const EC::Position> ecs) {
    bool clickedOnEntity = false;

    const auto* viewbox = ecs.Get<const EC::ViewBox>(entity);
    const auto* position = ecs.Get<const EC::Position>(entity);
    if (viewbox && position) {
        FRect entityRect = viewbox->box.rect();
        entityRect.x += position->x;
        entityRect.y += position->y;
        clickedOnEntity = pointInRect(point, entityRect); 
    }
    
    return clickedOnEntity;
}

inline void updateEntityViewBox(const GameState* state, Entity entity, EC::ViewBox viewbox) {
    assert(state->ecs.EntityExists(entity));

    llvm::BitVector entityViewBoxModifiedFlags(MAX_ENTITIES);

    auto& flags = entityViewBoxModifiedFlags;

    flags[entity.id] = 1;
}

/*
void instantiateViewBoxModifications() {
    struct ModCommand {
        Entity entity;
        EC::ViewBox newViewbox;
    };
    My::Vec<ModCommand> mods;

    for (int i = 0; i < mods.size; i++) {
        auto& mod = mods[i];
        Vec2 minPos = mod.newViewbox.min;
        Vec2 maxPos = mod.newViewbox.max();
        IVec2 minChunk = toChunkPosition(minPos);
        IVec2 maxChunk = toChunkPosition(maxPos);

        IVec2 chunkDelta = maxChunk - minChunk;
        if (chunkDelta.x > 0 || chunkDelta.y > 0) {
            
        }
    }
}
*/

OptionalEntity<> findTileEntityAtPosition(const GameState* state, Vec2 position);

/*
* Remove (or destroy) the entity on the tile, if it exists.
* Probably shouldn't use this tbh, not a good function.
* @return A boolean representing whether the entity was destroyed.
*/
bool removeEntityOnTile(EntityWorld* ecs, Tile* tile);

bool placeEntityOnTile(EntityWorld* ecs, Tile* tile, Entity entity);

void forEachEntityInRange(const EntityWorld* ecs, const ChunkMap* chunkmap, Vec2 pos, float radius, const std::function<int(Entity)>& callback);

void forEachEntityNearPoint(ComponentManager<EC::ViewBox> ecs, const ChunkMap* chunkmap, Vec2 point, const std::function<int(Entity)>& callback);

void forEachChunkContainingBounds(const ChunkMap* chunkmap, Boxf bounds, const std::function<void(ChunkData*)>& callback);

void forEachEntityInBounds(ComponentManager<const EC::ViewBox> ecs, const ChunkMap* chunkmap, Boxf bounds, const std::function<void(Entity)>& callback);

OptionalEntity<EC::ViewBox>
findFirstEntityAtPosition(const EntityWorld* ecs, const ChunkMap* chunkmap, Vec2 position);

OptionalEntity<EC::Position>
findClosestEntityToPosition(const EntityWorld* ecs, const ChunkMap* chunkmap, Vec2 position);

OptionalEntity<EC::ViewBox, EC::Render>
findPlayerFocusedEntity(ComponentManager<EC::ViewBox, const EC::Render> ecs, const ChunkMap& chunkmap, Vec2 playerMousePos);

constexpr Vec2 EntityMaxPos = {(float)INT_MAX, (float)INT_MAX};
constexpr Vec2 EntityMinPos = {(float)INT_MIN, (float)INT_MIN};

constexpr inline bool isValidEntityPosition(Vec2 p) {
    return p.x > EntityMinPos.x && p.y > EntityMinPos.y
        && p.x < EntityMaxPos.x && p.y < EntityMaxPos.y;
}

inline void setEntityStaticPosition(GameState* state, Entity entity, Vec2 position) {
    auto* positionEc = state->ecs.Get<EC::Position>(entity);
    Vec2 oldPos = positionEc->vec2();
    positionEc->x = position.x;
    positionEc->y = position.y;
    entityPositionChanged(state, entity, oldPos);
}

#endif