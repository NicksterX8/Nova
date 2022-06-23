#ifndef ENTITY_METHODS_INCLUDED
#define ENTITY_METHODS_INCLUDED

#include "../EntityComponents/Components.hpp"
#include "../ECS/EntityType.hpp"
#include "../Chunks.hpp"

namespace Entities {

inline bool tryOccupyTileAt(Entity entity, Vec2 position, ChunkMap* chunkmap) {
    Tile* tile = getTileAtPosition(*chunkmap, position);
    if (!(tile && tile->entity.IsAlive())) {
        tile->entity = entity;
        return true;
    }
    return false;
}

inline bool tryOccupyTile(EntityType<PositionComponent> entity, ChunkMap* chunkmap, ECS* ecs) {
    Vec2 position = ecs->Get<PositionComponent>(entity)->vec2();
    return tryOccupyTileAt(entity, position, chunkmap);
}

Entity clone(ECS* ecs, Entity entity);

}

#endif