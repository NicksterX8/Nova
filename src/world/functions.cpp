#include "world/functions.hpp"
#include "Chunks.hpp"
#include "GameState.hpp"

namespace World {

Box getEntityViewBoxBounds(const EntityWorld* ecs, Entity entity) {
    assert(ecs->entityExists(entity) && "Entity does not exist!");
    EC::Position* positionEc = ecs->Get<EC::Position>(entity);
    assert(positionEc && "Entity must have a position!");
    Vec2 pos = positionEc->vec2();
    auto* viewbox = ecs->Get<EC::ViewBox>(entity);
    assert(viewbox && "Entity must have viewbox!");
    return Box{
        pos + viewbox->box.min,
        viewbox->box.size
    };
}

bool pointInEntity(Vec2 point, Entity entity, const EntityWorld& ecs) {
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

void setEventCallbacks(EntityWorld& ecs, ChunkMap& chunkmap) {
    ecs.setComponentDestructor(EC::Position::ID, [&](Entity entity){
        auto* viewbox = ecs.Get<EC::ViewBox>(entity);
        if (!viewbox) {
            LogError("entity viewbox not found!");
            return;
        }
        auto* position = ecs.Get<EC::Position>(entity);
        if (!position) {
            LogError("Entity position not found");
            return;
        }
        forEachChunkContainingBounds(&chunkmap, {{position->vec2() + viewbox->box.min, position->vec2() + viewbox->box.max()}}, [entity](ChunkData* chunkdata){
            chunkdata->removeCloseEntity(entity);
        });
    });

    ecs.setComponentDestructor(EC::Inventory::ID, [&ecs](Entity entity){
        ecs.Get<EC::Inventory>(entity)->inventory.destroy();
    });
}

void forEachEntityInRange(const EntityWorld& ecs, const ChunkMap* chunkmap, Vec2 pos, float radius, const std::function<int(Entity)>& callback) {
    radius = abs(radius);
    float radiusSqrd = radius * radius;

    // form rectangle of chunks to search for entities in
    IVec2 highChunkBound = {(int)floor((pos.x + radius) / CHUNKSIZE), (int)floor((pos.y + radius) / CHUNKSIZE)};
    IVec2 lowChunkBound =  {(int)floor((pos.x - radius) / CHUNKSIZE), (int)floor((pos.y - radius) / CHUNKSIZE)};

    // go through each chunk in chunk search rectangle
    for (int y = lowChunkBound.y; y <= highChunkBound.y; y++) {
        for (int x = lowChunkBound.x; x <= highChunkBound.x; x++) {
            const ChunkData* chunkdata = chunkmap->get({x, y});
            if (chunkdata == NULL) {
                // entities can't be in non-existent chunks
                continue;
            }

            for (int e = 0; e < chunkdata->closeEntities.size; e++) {
                Entity closeEntity = chunkdata->closeEntities[e];
                if (!ecs.EntityExists(closeEntity)) {
                    LogError("Entity in closeEntities is dead!");
                    continue;
                }
                EC::ViewBox* entityBox = ecs.Get<EC::ViewBox>(closeEntity);
                assert(entityBox);
                Vec2 entityCenter = entityBox->box.center();
                Vec2 entitySize = entityBox->box.size;

                Vec2 halfSize = entitySize * 0.5f;

                // TODO: collision is broke for non squares. need math and stuff

                radiusSqrd += (halfSize.x * halfSize.y + halfSize.y * halfSize.y) * M_SQRT2;
            
                Vec2 delta = entityCenter - pos;
                float distSqrd = delta.x * delta.x + delta.y * delta.y;
                if (distSqrd < radiusSqrd) {
                    // entity is within radius
                    if (callback(closeEntity)) {
                        return;
                    }
                }
            }
        }
    }
}

void forEachEntityNearPoint(const EntityWorld& ecs, const ChunkMap* chunkmap, Vec2 point, const std::function<int(Entity)>& callback) {
    IVec2 chunkPos = toChunkPosition(point);
    const ChunkData* chunkdata = chunkmap->get(chunkPos);
    if (!chunkdata) {
        // entities can't be in non-existent chunks
        return;
    }

    for (int e = 0; e < chunkdata->closeEntities.size; e++) {
        Entity closeEntity = chunkdata->closeEntities[e];
        if (callback(closeEntity)) {
            return;
        }
    }
}

void forEachEntityInBounds(const EntityWorld& ecs, const ChunkMap* chunkmap, Boxf bounds, const std::function<void(Entity)>& callback) {
    forEachChunkContainingBounds(chunkmap, bounds, [&](ChunkData* chunkdata){
        Vec2 min = bounds[0];
        Vec2 max = bounds[1];

        for (int i = chunkdata->closeEntities.size-1 ; i >= 0; i--) {
            auto closeEntity = chunkdata->closeEntities[i];
            /* TEMPORARY!  TODO: replace this */
            if (!ecs.EntityExists(closeEntity)) {
                chunkdata->removeCloseEntity(closeEntity);
                continue;
            }

            auto* entityViewbox = ecs.Get<const EC::ViewBox>(closeEntity);
            auto* entityPos = ecs.Get<const EC::Position>(closeEntity);
            assert(entityViewbox && entityPos);

            Vec2 eMin = entityPos->vec2() + entityViewbox->box.min;
            Vec2 eMax = eMin + entityViewbox->box.size;

            if (eMin.x < max.x && eMax.x > min.x &&
                eMin.y < max.y && eMax.y > min.y)
            {
                callback(closeEntity);
            }
        }
    });
}

SmallVectorA<Entity, VirtualAllocator, 4> getAllEntitiesInBounds(const EntityWorld& ecs, const ChunkMap* chunkmap, Boxf bounds, VirtualAllocator allocator) {
    SmallVectorA<Entity, VirtualAllocator, 4> result{allocator};
    
    forEachChunkContainingBounds(chunkmap, bounds, [&](ChunkData* chunkdata){
        Vec2 min = bounds[0];
        Vec2 max = bounds[1];

        result.reserve(result.size() + chunkdata->closeEntities.size);
        for (int i = chunkdata->closeEntities.size-1 ; i >= 0; i--) {
            auto closeEntity = chunkdata->closeEntities[i];
            /* TEMPORARY!  TODO: replace this */
            if (!ecs.EntityExists(closeEntity)) {
                chunkdata->removeCloseEntity(closeEntity);
                continue;
            }

            auto* entityViewbox = ecs.Get<const EC::ViewBox>(closeEntity);
            auto* entityPos = ecs.Get<const EC::Position>(closeEntity);
            assert(entityViewbox && entityPos);

            Vec2 eMin = entityPos->vec2() + entityViewbox->box.min;
            Vec2 eMax = eMin + entityViewbox->box.size;

            if (eMin.x < max.x && eMax.x > min.x &&
                eMin.y < max.y && eMax.y > min.y)
            {
                result.push_back(closeEntity);
            }
        }
    });

    return result;
}

SmallVectorA<Entity, VirtualAllocator, 0> getAllEntitiesNearBounds(const EntityWorld& ecs, const ChunkMap* chunkmap, Boxf bounds, VirtualAllocator allocator) {
    SmallVectorA<Entity, VirtualAllocator, 0> result{allocator};
    
    forEachChunkContainingBounds(chunkmap, bounds, [&](ChunkData* chunkdata){
        result.append(chunkdata->closeEntities.begin(), chunkdata->closeEntities.end());
    });

    return result;
}

Entity findFirstEntityAtPosition(const EntityWorld& ecs, const ChunkMap* chunkmap, Vec2 position) {
    Entity foundEntity;
    World::forEachEntityNearPoint(ecs, chunkmap, position, [&](Entity entity){
        if (pointInEntity(position, entity, ecs)) {
            // player is targeting this entity
            foundEntity = entity;
            return 1;
        }
        return 0;
    });

    return foundEntity;
}

Entity findClosestEntityToPosition(const EntityWorld& ecs, const ChunkMap* chunkmap, Vec2 position) {
    Entity closestEntity = NullEntity;
    float closestDistSqrd = INFINITY;
    forEachEntityNearPoint(ecs, chunkmap, position, [&](Entity entity){
        // TODO: only works correctly for squares
        Vec2 entityCenter = ecs.Get<EC::ViewBox>(entity)->box.center();
        Vec2 delta = entityCenter - position;
        float entityDistSqrd = delta.x*delta.x + delta.y*delta.y;
        if (entityDistSqrd < closestDistSqrd) {
            closestEntity = entity;
            closestDistSqrd = entityDistSqrd;
        }
        return 0;
    });

    return closestEntity;
}

Entity findPlayerFocusedEntity(const EntityWorld& ecs, const ChunkMap& chunkmap, Vec2 playerMousePos) {
    Vec2 target = playerMousePos;
    Entity focusedEntity = NullEntity;
    int focusedEntityLayer = RenderLayers::Lowest;
    forEachEntityNearPoint(ecs, &chunkmap, target,
    [&](Entity entity){
        if (!ecs.EntityHas<EC::Position, EC::ViewBox, EC::Render>(entity)) {
            return 0;
        }

        auto render = ecs.Get<const EC::Render>(entity);
        if (pointInEntity(target, entity, ecs)) {
            for (int i = 0; i < render->numTextures; i++) {
                int entityLayer = render->textures[i].layer;
                if (entityLayer > focusedEntityLayer 
                    || (entityLayer == focusedEntityLayer
                    && entity.id > focusedEntity.id)) {
                    focusedEntity = entity;
                    focusedEntityLayer = entityLayer;
                }
            }
        }

        return 0;
    });
    return focusedEntity;
}

void rotateEntity(const EntityWorld& ecs, Entity entity, bool clockwise) {
    float* rotation = &ecs.Get<EC::Rotation>(entity)->degrees;
    auto rotatable = ecs.Get<EC::Rotatable>(entity);
    // left shift switches direction
    if (clockwise) {
        *rotation += rotatable->increment;
    } else {
        *rotation -= rotatable->increment;
    }
}

void entityViewChanged(ChunkMap* chunkmap, Entity entity, Vec2 newPos, Vec2 oldPos, Box newViewbox, Box oldViewbox, bool justMade) {
    /* put entity in to the chunks it's visible in */
    if (UNLIKELY(!isValidEntityPosition(newPos))) {
        LogCritical("Entity has invalid position! Position: %f,%f", newPos.x, newPos.y);
    }

    IVec2 oldMinChunkPosition = toChunkPosition(oldPos + oldViewbox.min);
    IVec2 oldMaxChunkPosition = toChunkPosition(oldPos + oldViewbox.max());
    IVec2 newMinChunkPosition = toChunkPosition(newPos + newViewbox.min);
    IVec2 newMaxChunkPosition = toChunkPosition(newPos + newViewbox.max());

    // early exit for simple and common case where entity never changed what chunks its in
    if ((newMinChunkPosition == oldMinChunkPosition) &&
        (newMaxChunkPosition == oldMaxChunkPosition) &&
        !justMade) return;

    IVec2 minChunkPosition = {
        (oldMinChunkPosition.x < newMinChunkPosition.x) ? oldMinChunkPosition.x : newMinChunkPosition.x,
        (oldMinChunkPosition.y < newMinChunkPosition.y) ? oldMinChunkPosition.y : newMinChunkPosition.y
    };
    IVec2 maxChunkPosition = {
        (oldMaxChunkPosition.x > newMaxChunkPosition.x) ? oldMaxChunkPosition.x : newMaxChunkPosition.x,
        (oldMaxChunkPosition.y > newMaxChunkPosition.y) ? oldMaxChunkPosition.y : newMaxChunkPosition.y
    };

    if (!justMade) {
        for (int col = oldMinChunkPosition.x; col <= oldMaxChunkPosition.x; col++) {
            for (int row = oldMinChunkPosition.y; row <= oldMaxChunkPosition.y; row++) {
                IVec2 chunkPosition = {col, row};

                bool inNewArea = (chunkPosition.x >= newMinChunkPosition.x && chunkPosition.y >= newMinChunkPosition.y &&
                    chunkPosition.x <= newMaxChunkPosition.x && chunkPosition.y <= newMaxChunkPosition.y);

                if (!inNewArea) {
                    // remove entity from old chunk
                    ChunkData* oldChunkdata = chunkmap->get(chunkPosition);
                    if (oldChunkdata) {
                        oldChunkdata->removeCloseEntity(entity);
                    }
                }
            }
        }
    }

    for (int col = newMinChunkPosition.x; col <= newMaxChunkPosition.x; col++) {
        for (int row = newMinChunkPosition.y; row <= newMaxChunkPosition.y; row++) {
            IVec2 chunkPosition = {col, row};
            bool inOldArea = (chunkPosition.x >= oldMinChunkPosition.x && chunkPosition.y >= oldMinChunkPosition.y &&
                chunkPosition.x <= oldMaxChunkPosition.x && chunkPosition.y <= oldMaxChunkPosition.y && !justMade);

            if (!inOldArea) {
                // add entity to new chunk
                ChunkData* newChunkdata = chunkmap->get(chunkPosition);
                if (newChunkdata) {
                    newChunkdata->closeEntities.push(entity);
                }
            }
        }
    }
}

void entityPositionChanged(GameState* state, Entity entity, Vec2 oldPos) {
    auto* position = state->ecs->Get<EC::Position>(entity);
    auto* viewbox = state->ecs->Get<EC::ViewBox>(entity);
    if (!position || !viewbox) {
        LogError("why here? no veiwbox or pos");
    }
    entityViewChanged(&state->chunkmap, entity, position->vec2(), oldPos, viewbox->box, viewbox->box, false);
}

void entityViewboxChanged(GameState* state, Entity entity, Box oldViewbox) {
    auto* position = state->ecs->Get<EC::Position>(entity);
    auto* viewbox = state->ecs->Get<EC::ViewBox>(entity);
    if (!position || !viewbox) {
        LogError("why here? no veiwbox or pos");
    }
    entityViewChanged(&state->chunkmap, entity, position->vec2(), position->vec2(), viewbox->box, oldViewbox, false);
}

void entityViewAndPosChanged(GameState* state, Entity entity, Vec2 oldPos, Box oldViewbox) {
    auto* position = state->ecs->Get<EC::Position>(entity);
    auto* viewbox = state->ecs->Get<EC::ViewBox>(entity);
    if (!position || !viewbox) {
        LogError("why here? no veiwbox or pos");
    }
    entityViewChanged(&state->chunkmap, entity, position->vec2(), oldPos, viewbox->box, oldViewbox, false);
}

void entityCreated(GameState* state, Entity entity) {
    auto* position = state->ecs->Get<EC::Position>(entity);
    auto* viewbox = state->ecs->Get<EC::ViewBox>(entity);
    if (!position || !viewbox) {
        LogError("why here? no veiwbox or pos");
    }
    entityViewChanged(&state->chunkmap, entity, position->vec2(), position->vec2(), viewbox->box, viewbox->box, true);
}

}