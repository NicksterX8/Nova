#ifndef RENDERING_SYSTEMS_INCLUDED
#define RENDERING_SYSTEMS_INCLUDED

#include <vector>
#include <tuple>
#include "../ECS/EntitySystem.hpp"
#include "../ECS/Query.hpp"
#include "../ECS/ECS.hpp"
#include "../GameViewport.hpp"
#include "../Log.hpp"
#include "../Rendering/Drawing.hpp"
#include "../Debug.hpp"

template<class Query>
bool ta(Query query, ComponentFlags signature) {
    return query.check_t(signature);
}

template<class C1, class C2>
using TwoComponentEntityCallback = std::function<void(C1&, C2&)>;

/*
template<class C1, class C2, class Func>
void iterateComponents(ECS* ecs, Func callback) {
    ComponentPool<C1>* pool1 = ecs->manager.getPool<C1>();
    ComponentPool<C2>* pool2 = ecs->manager.getPool<C2>();
    const Uint32 size1 = pool1->getSize();
    const Uint32 size2 = pool2->getSize();
    if (size1 < size2) {
        // use component pool 1
        for (Uint32 c = 0; c < size1; c++) {
            //
            EntityID entityID = pool1->componentOwners[c];
            Entity entity = ecs->manager.entities[ecs->manager.indices[entityID]];
            if (ecs->entityComponents(entityID)[getID<C2>()]) {
                callback(entity.cast<C1, C2>(), &pool1->components[c], ecs->Get<C2>(entity));
            }
            //
            EntityID entityID = pool1->componentOwners[c];
            // Entity entity = ecs->manager.entities[ecs->manager.indices[entityID]];
            if (ecs->entityComponents(entityID)[getID<C1>()]) {
                callback(pool1->components[c], pool2->components[pool2->entityComponentSet[entityID]]);
            }
        }
        
    } else {
        // use component pool 2
        for (Uint32 c = 0; c < size2; c++) {
            EntityID entityID = pool2->componentOwners[c];
            if (ecs->entityComponents(entityID)[getID<C1>()]) {
                callback(pool1->components[pool1->entityComponentSet[entityID]], pool2->components[c]);
            }
        }
    }
}
*/

template<class C1, class Func>
void iterateComponents(ECS* ecs, Func callback) {
    ecs->iterateComponents<C1>(callback);
}

template<class T, bool Mutable>
using MutComponent = T;
/*
void tester(ECS* ecs) {
    iterateComponents<EC::Position, EC::Render>(ecs,
    [](EC::Position& position, EC::Render& render){
        render.destination.x = position.x;
        render.destination.y = position.y;
    });

    iterateComponents<EC::Render>(ecs, [](EC::Render& render){
        render.layer++;
    });
}
*/

class RenderSizeSystem : public EntitySystem {
public:
    const GameViewport* gameViewport;

    #define SYSTEM_ACCESS(component) (sys.componentAccess[getID<component>()])

    RenderSizeSystem(ECS* ecs) : EntitySystem(ecs) {
        using namespace ComponentAccess;
        sys.GiveAccess<EC::Render>(Read | Write);
        SYSTEM_ACCESS(EC::Size) = Read;
    }

    bool Query(ComponentFlags entitySignature) const {
        return ((entitySignature & componentSignature<EC::Size, EC::Render>()) == componentSignature<EC::Size, EC::Render>());
    }

    void Update() {
        ForEach([&](Entity entity){
            SDL_FRect* renderDst = &sys.GetReadWrite<EC::Render>(entity)->destination;
            renderDst->w = sys.GetReadOnly<EC::Size>(entity)->width * TileWidth;
            renderDst->h = sys.GetReadOnly<EC::Size>(entity)->height * TileHeight;
        });
    }

private:

};

class RenderPositionSystem : public EntitySystem {
public:
    const GameViewport* gameViewport;

    RenderPositionSystem(ECS* ecs) : EntitySystem(ecs) {
        using namespace ComponentAccess;
        sys.GiveAccess<EC::Position>(Read);
        sys.GiveAccess<EC::Render>(Read | Write);
    }

    bool Query(ComponentFlags entitySignature) const {
        return ((entitySignature & componentSignature<EC::Position, EC::Render>()) == componentSignature<EC::Position, EC::Render>());
    }

    void Update() {
        ForEach([&](Entity entity){
            // copy rotation to render component
            SDL_FRect* renderDst = &sys.GetReadWrite<EC::Render>(entity)->destination;
            Vec2 dstPosition = gameViewport->worldToPixelPositionF(*sys.GetReadOnly<EC::Position>(entity));
            renderDst->x = dstPosition.x;
            renderDst->y = dstPosition.y;
        });
    }

private:

};

class SimpleRectRenderSystem : public EntitySystem {
public:
    typedef EntityQuery<
        RequireComponents<EC::Render, EC::Size, EC::Position>,
        AvoidComponents<>,
        LogicalOrComponents<>
    > QueryT;

    SDL_Renderer* renderer = NULL;
    float scale = 1.0f;
    const GameViewport* gameViewport = NULL;
    ComponentFlags signature;

    struct RenderTarget {
        SDL_Texture* texture;
        const SDL_FRect* destination;
        float degreesRotation;
    };

    SimpleRectRenderSystem(ECS* ecs) : EntitySystem(ecs) {}

    SimpleRectRenderSystem(ECS* ecs, SDL_Renderer* renderer, const GameViewport* gameViewport) : EntitySystem(ecs) {
        this->renderer = renderer;
        this->gameViewport = gameViewport;

        using namespace ComponentAccess;
        sys.GiveAccess<EC::Render>(ReadWrite);
        sys.GiveAccess<EC::Size>(Read);
        sys.GiveAccess<EC::Position>(Read);
    }

    using QueriedEntity = EntityType<EC::Render, EC::Size, EC::Position>;

    bool Query(ComponentFlags components) const {
        //constexpr ComponentFlags need = componentSignature<EC::Render, EC::Size, EC::Position>();
        //return ((need & components) == need);
        return QueryT::check(components);
    }

    void Update() {
        ForEach([this](Entity entity){
            SDL_FRect* renderDst = &sys.GetReadWrite<EC::Render>(entity)->destination;
            renderDst->w = sys.GetReadOnly<EC::Size>(entity)->width * TileWidth;
            renderDst->h = sys.GetReadOnly<EC::Size>(entity)->height * TileHeight;
        });

        ForEach([this](Entity entity){
            // copy position component to render component
            SDL_FRect* renderDst = &sys.GetReadWrite<EC::Render>(entity)->destination;
            Vec2 dstPosition = gameViewport->worldToPixelPositionF(*sys.GetReadOnly<EC::Position>(entity));
            renderDst->x = dstPosition.x;
            renderDst->y = dstPosition.y;
        });

        ForEach([this](Entity entity){
            SDL_FRect* renderDst = &sys.GetReadWrite<EC::Render>(entity)->destination;
            renderDst->x -= renderDst->w * 0.5f;
            renderDst->y -= renderDst->h * 0.5f;
        });

        Uint32 index = 0;

        std::vector<RenderTarget> renderLayers[NUM_RENDER_LAYERS];
        ForEach([&](Entity entity){
            EC::Render* render = sys.GetReadWrite<EC::Render>(entity);
            if (render->texture != NULL) {
                const SDL_FRect* dest = &render->destination;
                if ((dest->x + dest->w > gameViewport->display.x && dest->x < gameViewport->displayRightEdge()) &&
                    (dest->y + dest->h > gameViewport->display.y && dest->y < gameViewport->displayBottomEdge())) {
                    //(dest->x + dest->w > gameViewport->display.x && dest->x < gameViewport->displayRightEdge() &&
                    //dest->y > gameViewport->display.y && dest->y < gameViewport->displayBottomEdge())) {
                    
                    renderLayers[render->layer].push_back({render->texture, dest, render->rotation});
                    render->renderIndex = index++;
                }
            }
        });

        int nRenderedEntities = 0;
        for (int l = 0; l < NUM_RENDER_LAYERS; l++) {
            std::vector<RenderTarget>& renderLayer = renderLayers[l];
            for (size_t r = 0; r < renderLayer.size(); r++) {
                SDL_RenderCopyExF(renderer, renderLayer[r].texture, NULL, renderLayer[r].destination, renderLayer[r].degreesRotation, NULL, SDL_FLIP_NONE);
                nRenderedEntities++;
            }
        }

        ForEach<EC::Health, EC::Render>([&](EntityType<EC::Health, EC::Render> entity){   
            auto destRect = &sys.GetReadOnly<EC::Render>(entity)->destination;
            SDL_Rect _dest = FC_GetBounds(FreeSans, destRect->x, destRect->y, FC_ALIGN_LEFT, FC_MakeScale(0.5, 0.5), "%.1f", entity.Get<EC::Health>()->healthValue);
            auto dest = &_dest;
            if ((dest->x + dest->w > gameViewport->display.x && dest->x < gameViewport->displayRightEdge()) &&
                (dest->y + dest->h > gameViewport->display.y && dest->y < gameViewport->displayBottomEdge())) {
                FC_DrawScale(FreeSans, renderer, destRect->x, destRect->y, FC_MakeScale(0.5, 0.5), "%.1f", entity.Get<EC::Health>()->healthValue);
            }
    
        });

        if (Debug->settings.drawEntityIDs) {
            int nRenderedIDs = 0;
            ForEach([&](Entity entity){
                auto destRect = &sys.GetReadOnly<EC::Render>(entity)->destination;
                SDL_Rect _dest = FC_GetBounds(FreeSans, destRect->x, destRect->y, FC_ALIGN_LEFT, FC_MakeScale(0.5, 0.5), "%u", entity.id);
                auto dest = &_dest;
                if ((dest->x + dest->w > gameViewport->display.x && dest->x < gameViewport->displayRightEdge()) &&
                    (dest->y + dest->h > gameViewport->display.y && dest->y < gameViewport->displayBottomEdge())) {
                    FC_DrawScale(FreeSans, renderer, destRect->x, destRect->y, FC_MakeScale(0.5, 0.5), "%u", entity.id);
                    nRenderedIDs++;
                }
            });
            Log("rendered %d ids", nRenderedIDs);
        }

        if (Debug->settings.drawEntityRects) {
            for (int l = 0; l < NUM_RENDER_LAYERS; l++) {
                std::vector<RenderTarget>& renderLayer = renderLayers[l];
                for (auto target : renderLayer) {
                    Draw::debugRect(renderer, target.destination, scale);
                }
            }
        }
    }
};

// draw simple render entities
class RenderSystem : public EntitySystem {
public:
    SDL_Renderer* renderer = NULL;
    float scale = 1.0f;
    const GameViewport* gameViewport = NULL;
    ComponentFlags signature;

    struct RenderTarget {
        SDL_Texture* texture;
        const SDL_FRect* destination;
        float degreesRotation;
    };

    RenderSystem(ECS* ecs) : EntitySystem(ecs) {}

    RenderSystem(ECS* ecs, SDL_Renderer* renderer, const GameViewport* gameViewport) : EntitySystem(ecs) {
        this->renderer = renderer;
        this->gameViewport = gameViewport;
        using namespace EC;
        signature = componentSignature<Render>();

        using namespace ComponentAccess;
        sys.GiveAccess<Render>(ReadWrite);
        sys.GiveAccess<Size>(Read);
        sys.GiveAccess<Position>(Read);
    }

    bool Query(ComponentFlags entitySignature) const {
        if ((entitySignature & signature) == signature) {
            return true;
        }
        return false;
    }

    void Update() {
        ForEach<EC::Render, EC::Size>([&](auto entity){
            SDL_FRect* renderDst = &sys.GetReadWrite<EC::Render>(entity)->destination;
            renderDst->x -= renderDst->w * 0.5f;
            renderDst->y -= renderDst->h * 0.5f;
        });

        std::vector<RenderTarget> renderLayers[NUM_RENDER_LAYERS];
        ForEach([&](Entity entity){
            const EC::Render* render = sys.GetReadOnly<EC::Render>(entity);
            if (render->texture != NULL) {
                const SDL_FRect* dest = &render->destination;
                if ((dest->x + dest->w > gameViewport->display.x && dest->x < gameViewport->displayRightEdge()) &&
                    (dest->y + dest->h > gameViewport->display.y && dest->y < gameViewport->displayBottomEdge())) {
                    //(dest->x + dest->w > gameViewport->display.x && dest->x < gameViewport->displayRightEdge() &&
                    //dest->y > gameViewport->display.y && dest->y < gameViewport->displayBottomEdge())) {
                    
                    renderLayers[render->layer].push_back({render->texture, dest, render->rotation});
                }
            }
        });

        int nRenderedEntities = 0;
        for (int l = 0; l < NUM_RENDER_LAYERS; l++) {
            std::vector<RenderTarget>& renderLayer = renderLayers[l];
            for (size_t r = 0; r < renderLayer.size(); r++) {
                SDL_RenderCopyExF(renderer, renderLayer[r].texture, NULL, renderLayer[r].destination, renderLayer[r].degreesRotation, NULL, SDL_FLIP_NONE);
                nRenderedEntities++;
            }
        }

        ForEach<EC::Health, EC::Render>([&](EntityType<EC::Health, EC::Render> entity){   
            auto destRect = &sys.GetReadOnly<EC::Render>(entity)->destination;
            SDL_Rect _dest = FC_GetBounds(FreeSans, destRect->x, destRect->y, FC_ALIGN_LEFT, FC_MakeScale(0.5, 0.5), "%.1f", entity.Get<EC::Health>()->healthValue);
            auto dest = &_dest;
            if ((dest->x + dest->w > gameViewport->display.x && dest->x < gameViewport->displayRightEdge()) &&
                (dest->y + dest->h > gameViewport->display.y && dest->y < gameViewport->displayBottomEdge())) {
                FC_DrawScale(FreeSans, renderer, destRect->x, destRect->y, FC_MakeScale(0.5, 0.5), "%.1f", entity.Get<EC::Health>()->healthValue);
            }
    
        });

        if (Debug->settings.drawEntityIDs) {
            int nRenderedIDs = 0;
            ForEach([&](Entity entity){
                auto destRect = &sys.GetReadOnly<EC::Render>(entity)->destination;
                SDL_Rect _dest = FC_GetBounds(FreeSans, destRect->x, destRect->y, FC_ALIGN_LEFT, FC_MakeScale(0.5, 0.5), "%u", entity.id);
                auto dest = &_dest;
                if ((dest->x + dest->w > gameViewport->display.x && dest->x < gameViewport->displayRightEdge()) &&
                    (dest->y + dest->h > gameViewport->display.y && dest->y < gameViewport->displayBottomEdge())) {
                    FC_DrawScale(FreeSans, renderer, destRect->x, destRect->y, FC_MakeScale(0.5, 0.5), "%u", entity.id);
                    nRenderedIDs++;
                }
            });
            Log("rendered %d ids", nRenderedIDs);
        }

        if (Debug->settings.drawEntityRects) {
            for (int l = 0; l < NUM_RENDER_LAYERS; l++) {
                std::vector<RenderTarget>& renderLayer = renderLayers[l];
                for (auto target : renderLayer) {
                    Draw::debugRect(renderer, target.destination, scale);
                }
            }
        }
    }
};

#endif