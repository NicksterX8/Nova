#include "world/functions.hpp"
#include "Chunks.hpp"
#include "GameState.hpp"

namespace World {

Box getEntityViewBoxBounds(const EntityWorld* ecs, Entity entity) {
    Vec2 pos = ecs->Get<EC::Position>(entity)->vec2();
    auto* viewbox = ecs->Get<EC::ViewBox>(entity);
    return Box{
        pos + viewbox->box.min,
        pos + viewbox->box.min + viewbox->box.size
    };
}

void setEventCallbacks(EntityWorld& ecs, ChunkMap& chunkmap) {
    ecs.SetOnAdd<EC::ViewBox>([&](EntityWorld* ecs, Entity entity) {

        auto* viewBox = ecs->Get<World::EC::ViewBox>(entity);
        if (!viewBox) {
            LogError("entity viewbox not found!");
            return;
        }
        auto* position = ecs->Get<World::EC::Position>(entity);
        if (!position) {
            LogError("Entity position not found. Make sure to add position before viewbox.");
            return;
        }

        Vec2 pos = position->vec2();

        IVec2 minChunkPosition = toChunkPosition(pos + viewBox->box.min);
        IVec2 maxChunkPosition = toChunkPosition(pos + viewBox->box.max());
        for (int col = minChunkPosition.x; col <= maxChunkPosition.x; col++) {
            for (int row = minChunkPosition.y; row <= maxChunkPosition.y; row++) {
                IVec2 chunkPosition = {col, row};
                // add entity to new chunk
                ChunkData* newChunkdata = chunkmap.get(chunkPosition);
                if (newChunkdata) {
                    newChunkdata->closeEntities.push(entity);
                }
            }
        }
    });
    ecs.SetBeforeRemove<EC::ViewBox>([&](EntityWorld* ecs, Entity entity){
        auto* viewbox = ecs->Get<EC::ViewBox>(entity);
        if (!viewbox) {
            LogError("entity viewbox not found!");
            return;
        }
        auto* position = ecs->Get<EC::Position>(entity);
        if (!position) {
            LogError("Entity position not found");
            return;
        }
        forEachChunkContainingBounds(&chunkmap, {{position->vec2() + viewbox->box.min, position->vec2() + viewbox->box.max()}}, [entity](ChunkData* chunkdata){
            if (!chunkdata->removeCloseEntity(entity)) {
                LogWarn("couldn't remove entity");
            }
        });
    });

    ecs.SetOnAdd<EC::Inventory>([](EntityWorld* ecs, Entity entity){
        ecs->Get<EC::Inventory>(entity)->inventory.addRef();
    });
    ecs.SetBeforeRemove<EC::Inventory>([](EntityWorld* ecs, Entity entity){
        ecs->Get<EC::Inventory>(entity)->inventory.removeRef();
    });

    ecs.SetOnAdd<EC::TransportLineEC>([](EntityWorld* ecs, Entity entity){
        auto* transportLineEc = ecs->Get<EC::TransportLineEC>(entity);
        
    });
    ecs.SetBeforeRemove<EC::TransportLineEC>([&](EntityWorld* ecs, Entity entity){
        auto* transportLineEc = ecs->Get<EC::TransportLineEC>(entity);
        transportLineEc->belts.destroy();
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
    Entity focusedEntity;
    int focusedEntityLayer = RenderLayers::Lowest;
    forEachEntityNearPoint(ecs, &chunkmap, target,
    [&](Entity entity){
        if (!ecs.EntityHas<EC::Position, EC::ViewBox, EC::Render>(entity)) {
            return 0;
        }

        auto render = ecs.Get<const EC::Render>(entity);
        // TODO: enhance for multiple textures
        int entityLayer = render->textures[0].layer;

        if (pointInEntity(target, entity, ecs)) {
            if (entityLayer > focusedEntityLayer
            || (entityLayer == focusedEntityLayer && entity.id > focusedEntity.id)) {
                focusedEntity = entity;
                focusedEntityLayer = entityLayer;
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
    rotatable->rotated = true;
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