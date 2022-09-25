#include "Methods.hpp"

namespace Entities {

    void throwEntity(EntityWorld* ecs, Entity entity, Vec2 target, float speed) {
        ecs->Add(entity, EC::Motion(target, speed));
        if (entity.Has<EC::Rotation>(ecs)) {
            float rotation = entity.Get<EC::Rotation>(ecs)->degrees;
            float timeToTarget = target.length() / speed;
            ecs->Add<EC::AngleMotion>(entity, EC::AngleMotion(rotation + timeToTarget * 30.0f, 30.0f));
        }
    }

}

void entitySizeChanged(ChunkMap* chunkmap, const EntityWorld* ecs, EntityT<EC::Position, EC::Size> entity, Vec2 oldSize) {
    Vec2 newSize = ecs->Get<EC::Size>(entity)->vec2();
    if (oldSize.x == newSize.x && oldSize.y == newSize.y) {
        return;
    }
    bool oldSizeIsNan = false;
    if (isnan(oldSize.x) || isnan(oldSize.y)) {
        oldSize = newSize;
        oldSizeIsNan = true;
    }
    Vec2 pos = ecs->Get<EC::Position>(entity)->vec2();
    IVec2 oldMinChunkPosition = toChunkPosition(pos - oldSize * 0.5f);
    IVec2 oldMaxChunkPosition = toChunkPosition(pos + oldSize * 0.5f);
    IVec2 newMinChunkPosition = toChunkPosition(pos - newSize * 0.5f);
    IVec2 newMaxChunkPosition = toChunkPosition(pos + newSize * 0.5f);
    IVec2 minChunkPosition = {
        (oldMinChunkPosition.x < newMinChunkPosition.x) ? oldMaxChunkPosition.x : newMaxChunkPosition.x,
        (oldMinChunkPosition.y < newMinChunkPosition.y) ? oldMaxChunkPosition.y : newMaxChunkPosition.y
    };
    IVec2 maxChunkPosition = {
        (oldMaxChunkPosition.x < newMaxChunkPosition.x) ? oldMaxChunkPosition.x : newMaxChunkPosition.x,
        (oldMaxChunkPosition.y < newMaxChunkPosition.y) ? oldMaxChunkPosition.y : newMaxChunkPosition.y
    };
    for (int col = minChunkPosition.x; col <= maxChunkPosition.x; col++) {
        for (int row = minChunkPosition.y; row <= maxChunkPosition.y; row++) {
            IVec2 chunkPosition = {col, row};

            bool inNewArea = (chunkPosition.x >= newMinChunkPosition.x && chunkPosition.y >= newMinChunkPosition.y &&
                chunkPosition.x <= newMaxChunkPosition.x && chunkPosition.y <= newMaxChunkPosition.y);
            
            bool inOldArea = (chunkPosition.x >= oldMinChunkPosition.x && chunkPosition.y >= oldMinChunkPosition.y &&
                chunkPosition.x <= oldMaxChunkPosition.x && chunkPosition.y <= oldMaxChunkPosition.y);

            if ((inNewArea && !inOldArea) || oldSizeIsNan) {
                // add entity to new chunk
                ChunkData* newChunkdata = chunkmap->getChunkData(chunkPosition);
                if (newChunkdata) {
                    newChunkdata->closeEntities.push_back(entity);
                }
            }

            if (inOldArea && !inNewArea && !oldSizeIsNan) {
                // remove entity from old chunk
                ChunkData* oldChunkdata = chunkmap->getChunkData(chunkPosition);
                if (oldChunkdata) {
                    for (unsigned int e = 0; e < oldChunkdata->closeEntities.size; e++) {
                        // TODO: try implementing binary search with sorting for faster removal
                        if (oldChunkdata->closeEntities[e].id == entity.id)
                            oldChunkdata->closeEntities.remove(e);
                    }
                }
            }
        }
    }
}

void entityPositionChanged(ChunkMap* chunkmap, const EntityWorld* ecs, EntityT<EC::Position> entity, Vec2 oldPos) {
    Vec2 newPos = ecs->Get<EC::Position>(entity)->vec2();
    if (oldPos.x == newPos.x && oldPos.y == newPos.y) {
        return;
    }

    bool oldPosIsNan = false;
    if (isnan(oldPos.x) || isnan(oldPos.y)) {
        oldPos = newPos;
        oldPosIsNan = true;
    }

    if (ecs->EntityHas<EC::Size>(entity)) {
        Vec2 size = ecs->Get<EC::Size>(entity)->vec2();
        IVec2 oldMinChunkPosition = toChunkPosition(oldPos - size * 0.5f);
        IVec2 oldMaxChunkPosition = toChunkPosition(oldPos + size * 0.5f);
        IVec2 newMinChunkPosition = toChunkPosition(newPos - size * 0.5f);
        IVec2 newMaxChunkPosition = toChunkPosition(newPos + size * 0.5f);
        IVec2 minChunkPosition = {
            (oldMinChunkPosition.x < newMinChunkPosition.x) ? oldMinChunkPosition.x : newMinChunkPosition.x,
            (oldMinChunkPosition.y < newMinChunkPosition.y) ? oldMinChunkPosition.y : newMinChunkPosition.y
        };
        IVec2 maxChunkPosition = {
            (oldMaxChunkPosition.x > newMaxChunkPosition.x) ? oldMaxChunkPosition.x : newMaxChunkPosition.x,
            (oldMaxChunkPosition.y > newMaxChunkPosition.y) ? oldMaxChunkPosition.y : newMaxChunkPosition.y
        };
        for (int col = minChunkPosition.x; col <= maxChunkPosition.x; col++) {
            for (int row = minChunkPosition.y; row <= maxChunkPosition.y; row++) {
                IVec2 chunkPosition = {col, row};

                bool inNewArea = (chunkPosition.x >= newMinChunkPosition.x && chunkPosition.y >= newMinChunkPosition.y &&
                  chunkPosition.x <= newMaxChunkPosition.x && chunkPosition.y <= newMaxChunkPosition.y);
                
                bool inOldArea = (chunkPosition.x >= oldMinChunkPosition.x && chunkPosition.y >= oldMinChunkPosition.y &&
                  chunkPosition.x <= oldMaxChunkPosition.x && chunkPosition.y <= oldMaxChunkPosition.y);

                if ((inNewArea && !inOldArea) || oldPosIsNan) {
                    // add entity to new chunk
                    ChunkData* newChunkdata = chunkmap->getChunkData(chunkPosition);
                    if (newChunkdata) {
                        newChunkdata->closeEntities.push_back(entity);
                    }
                }

                if (inOldArea && !inNewArea && !oldPosIsNan) {
                    // remove entity from old chunk
                    ChunkData* oldChunkdata = chunkmap->getChunkData(chunkPosition);
                    if (oldChunkdata) {
                        oldChunkdata->removeCloseEntity(entity);
                    }
                }
            }
        }
    } else {
        IVec2 oldChunkPosition = toChunkPosition(oldPos);
        IVec2 newChunkPosition = toChunkPosition(newPos);
        if (oldChunkPosition != newChunkPosition) {
            // add entity to new chunk
            ChunkData* newChunkdata = chunkmap->getChunkData(newChunkPosition);
            if (newChunkdata) {
                newChunkdata->closeEntities.push_back(entity);
            }
            // remove entity from old chunk
            ChunkData* oldChunkdata = chunkmap->getChunkData(oldChunkPosition);
            if (oldChunkdata) {
                for (unsigned int e = 0; e < oldChunkdata->closeEntities.size; e++) {
                    // TODO: try implementing binary search with sorting for faster removal
                    if (oldChunkdata->closeEntities[e].id == entity.id)
                        oldChunkdata->closeEntities.remove(e);
                }
            }
        }
    }
}

void entityPositionAndSizeChanged(ChunkMap* chunkmap, const EntityWorld* ecs, EntityT<EC::Position, EC::Size> entity, Vec2 oldPos, Vec2 oldSize) {
    Vec2 newPos = ecs->Get<EC::Position>(entity)->vec2();
    Vec2 newSize = ecs->Get<EC::Size>(entity)->vec2();
    if (oldPos.x == newPos.x && oldPos.y == newPos.y) {
        return;
    }
    if (oldSize.x == newSize.x && oldSize.y == newSize.y) {
        return;
    }

    IVec2 oldMinChunkPosition = toChunkPosition(oldPos - oldSize * 0.5f);
    IVec2 oldMaxChunkPosition = toChunkPosition(oldPos + oldSize * 0.5f);
    IVec2 newMinChunkPosition = toChunkPosition(newPos - newSize * 0.5f);
    IVec2 newMaxChunkPosition = toChunkPosition(newPos + newSize * 0.5f);
    IVec2 minChunkPosition = {
        (oldMinChunkPosition.x < newMinChunkPosition.x) ? oldMinChunkPosition.x : newMinChunkPosition.x,
        (oldMinChunkPosition.y < newMinChunkPosition.y) ? oldMinChunkPosition.y : newMinChunkPosition.y
    };
    IVec2 maxChunkPosition = {
        (oldMaxChunkPosition.x > newMaxChunkPosition.x) ? oldMaxChunkPosition.x : newMaxChunkPosition.x,
        (oldMaxChunkPosition.y > newMaxChunkPosition.y) ? oldMaxChunkPosition.y : newMaxChunkPosition.y
    };
    for (int col = minChunkPosition.x; col <= maxChunkPosition.x; col++) {
        for (int row = minChunkPosition.y; row <= maxChunkPosition.y; row++) {
            IVec2 chunkPosition = {col, row};

            bool inNewArea = (chunkPosition.x >= newMinChunkPosition.x && chunkPosition.y >= newMinChunkPosition.y &&
                chunkPosition.x <= newMaxChunkPosition.x && chunkPosition.y <= newMaxChunkPosition.y);
            
            bool inOldArea = (chunkPosition.x >= oldMinChunkPosition.x && chunkPosition.y >= oldMinChunkPosition.y &&
                chunkPosition.x <= oldMaxChunkPosition.x && chunkPosition.y <= oldMaxChunkPosition.y);

            if (inNewArea && !inOldArea) {
                // add entity to new chunk
                ChunkData* newChunkdata = chunkmap->getChunkData(chunkPosition);
                if (newChunkdata) {
                    newChunkdata->closeEntities.push_back(entity);
                }
            }

            if (inOldArea && !inNewArea) {
                // remove entity from old chunk
                ChunkData* oldChunkdata = chunkmap->getChunkData(chunkPosition);
                if (oldChunkdata) {
                    for (int e = 0; e < oldChunkdata->closeEntities.size; e++) {
                        // TODO: try implementing binary search with sorting for faster removal
                        if (oldChunkdata->closeEntities[e].id == entity.id)
                            oldChunkdata->closeEntities.remove(e);
                    }
                }
            }
        }
    }
}