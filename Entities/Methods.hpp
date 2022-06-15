#ifndef ENTITY_METHODS_INCLUDED
#define ENTITY_METHODS_INCLUDED

#include "../ECS/EntityType.hpp"

namespace Entities {

inline bool tryOccupyTileAt(Entity entity, Vec2 position, ChunkMap* chunkmap) {
    Tile* tile = getTileAtPosition(*chunkmap, position);
    Entity currentEntity = tile->entity;
    if (currentEntity.IsAlive()) {
        return false;
    }
    tile->entity = entity;
    return true;
}

inline bool tryOccupyTile(EntityType<PositionComponent> entity, ChunkMap* chunkmap, ECS* ecs) {
    Vec2 position = *ecs->Get<PositionComponent>(entity);
    return tryOccupyTileAt(entity, position, chunkmap);
}

}

#endif