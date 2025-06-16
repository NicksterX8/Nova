#include "world/entities/methods.hpp"
#include "GameState.hpp"

namespace World {

namespace Entities {

    void throwEntity(EntityWorld& ecs, Entity entity, Vec2 target, float speed) {
        ecs.Add(entity, EC::Motion(target, speed));
        if (ecs.EntityHas<EC::Rotation>(entity)) {
            float rotation = ecs.Get<EC::Rotation>(entity)->degrees;
            float timeToTarget = target.length() / speed;
            ecs.Add<EC::AngleMotion>(entity, EC::AngleMotion(rotation + timeToTarget * 30.0f, 30.0f));
        }
    }

    Entity findNamedEntity(const char* name, const EntityWorld* ecs) {
        Entity target = NullEntity;
        ecs->ForEach_EarlyReturn([](ECS::Signature components){
            return components[EC::Nametag::ID];
        }, [&](Entity entity){
            auto nametag = ecs->Get<const EC::Nametag>(entity);
            // super inefficient btw
            if (My::streq(name, nametag->name)) {
                target = entity;
                return true;
            }
            return false;
        });

        return target;
    }

    void scaleGuy(GameState* state, Entity guy, float scale) {
        auto view = state->ecs.Get<EC::ViewBox>(guy);
        if (view) {
            auto oldViewbox = view->box;
            view->box.size = view->box.size * scale;
            entityViewboxChanged(state, guy, oldViewbox);
        }
        auto collision = state->ecs.Get<EC::CollisionBox>(guy);
        if (collision) {
            Vec2 size = collision->box.size;
            Vec2 newSize = size * scale;
            collision->box.min *= scale;
            collision->box.size *= scale;
        }
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
                chunkPosition.x <= oldMaxChunkPosition.x && chunkPosition.y <= oldMaxChunkPosition.y);

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
    auto* position = state->ecs.Get<EC::Position>(entity);
    auto* viewbox = state->ecs.Get<EC::ViewBox>(entity);
    if (!position || !viewbox) {
        LogError("why here? no veiwbox or pos");
    }
    entityViewChanged(&state->chunkmap, entity, position->vec2(), oldPos, viewbox->box, viewbox->box, false);
}

void entityViewboxChanged(GameState* state, Entity entity, Box oldViewbox) {
    auto* position = state->ecs.Get<EC::Position>(entity);
    auto* viewbox = state->ecs.Get<EC::ViewBox>(entity);
    if (!position || !viewbox) {
        LogError("why here? no veiwbox or pos");
    }
    entityViewChanged(&state->chunkmap, entity, position->vec2(), position->vec2(), viewbox->box, oldViewbox, false);
}

void entityViewAndPosChanged(GameState* state, Entity entity, Vec2 oldPos, Box oldViewbox) {
    auto* position = state->ecs.Get<EC::Position>(entity);
    auto* viewbox = state->ecs.Get<EC::ViewBox>(entity);
    if (!position || !viewbox) {
        LogError("why here? no veiwbox or pos");
    }
    entityViewChanged(&state->chunkmap, entity, position->vec2(), oldPos, viewbox->box, oldViewbox, false);
}

void entityCreated(GameState* state, Entity entity) {

}

}