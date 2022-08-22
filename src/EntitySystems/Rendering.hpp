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
#include "../constants.hpp"

//using ECS::EntitySystem;

/*
class RenderSizeSystem : public EntitySystem {
public:
    const GameViewport* gameViewport;

    #define SYSTEM_ACCESS(component) (sys.componentAccess[getID<component>()])

    RenderSizeSystem(EntityWorld* ecs) : EntitySystem(ecs) {
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
*/

class SimpleRectRenderSystem {
public:
    typedef ECS::EntityQuery<
        ECS::RequireComponents<EC::Render, EC::Size, EC::Position>
    > Query;

    SDL_Renderer* renderer = NULL;
    float scale = 1.0f;
    const GameViewport* gameViewport = NULL;

    struct RenderTarget {
        SDL_Texture* texture;
        const SDL_FRect* destination;
        float degreesRotation;
    };

    SimpleRectRenderSystem(SDL_Renderer* renderer, const GameViewport* gameViewport) {
        this->renderer = renderer;
        this->gameViewport = gameViewport;
    }

    using QueriedEntity = EntityT<EC::Render, EC::Size, EC::Position>;

    template<class EntityQueryT = Query>
    inline void ForEach(const ComponentManager<>& ecs, std::function<void(QueriedEntity entity)> callback) {
        ecs.ForEach<Query>(callback);
    }

    void Update(const ComponentManager<EC::Render, const EC::Size, const EC::Position, const EC::Health>& ecs, const ChunkMap& chunkmap) {
        
        Uint32 index = 0;
        std::vector<RenderTarget> renderLayers[NUM_RENDER_LAYERS];
        /*
        forEachEntityInBounds(&chunkmap, 
            Vec2(gameViewport->world.x + gameViewport->world.w/2.0f, gameViewport->world.y + gameViewport->world.h/2.0f), 
            Vec2(gameViewport->world.w, gameViewport->world.h), 
        [&](EntityT<EC::Position> entity){
            if (Query::check(ecs.EntitySignature(entity))) {
                auto render = ecs.Get<EC::Render>(entity);
                SDL_FRect* renderDst = &render->destination;
                Vec2 size = ecs.Get<const EC::Size>(entity)->vec2();
                renderDst->w = size.x * TileWidth;
                renderDst->h = size.y * TileHeight;
                Vec2 position = ecs.Get<const EC::Position>(entity)->vec2();
                Vec2 dstPosition = gameViewport->worldToPixelPositionF(position);
                renderDst->x = dstPosition.x - renderDst->w * 0.5f;
                renderDst->y = dstPosition.y - renderDst->w * 0.5f;

                if (render->texture != NULL) {
                    renderLayers[render->layer].push_back({render->texture, &render->destination, render->rotation});
                    render->renderIndex = index++;
                }
            }
        });


        forEachEntityInBounds(&chunkmap, 
            Vec2(gameViewport->world.x + gameViewport->world.w/2.0f, gameViewport->world.y + gameViewport->world.h/2.0f), 
            Vec2(gameViewport->world.w, gameViewport->world.h), 
        [&](EntityT<EC::Position> entity){
            if (Query::check(ecs.EntitySignature(entity))) {
                
            }
        });
        */

        /*
        ForEach(ecs, [&](Entity entity){
            SDL_FRect* renderDst = &ecs.Get<EC::Render>(entity)->destination;
            renderDst->w = ecs.Get<const EC::Size>(entity)->width * TileWidth;
            renderDst->h = ecs.Get<const EC::Size>(entity)->height * TileHeight;
        });

        ecs.ForEach<Query>([&](QueriedEntity entity){
            // copy position component to render component
            SDL_FRect* renderDst = &ecs.Get<EC::Render>(entity)->destination;
            Vec2 dstPosition = gameViewport->worldToPixelPositionF(ecs.Get<const EC::Position>(entity)->vec2());
            renderDst->x = dstPosition.x;
            renderDst->y = dstPosition.y;
        });

        ecs.ForEach<Query>([&](Entity entity){
            SDL_FRect* renderDst = &ecs.Get<EC::Render>(entity)->destination;
            renderDst->x -= renderDst->w * 0.5f;
            renderDst->y -= renderDst->h * 0.5f;
        });

        Uint32 index = 0;

        std::vector<RenderTarget> renderLayers[NUM_RENDER_LAYERS];
        ecs.ForEach<Query>([&](Entity entity){
            EC::Render* render = ecs.Get<EC::Render>(entity);
            if (render->texture != NULL) {
                const SDL_FRect* dest = &render->destination;
                if ((dest->x + dest->w > gameViewport->display.x && dest->x < gameViewport->displayRightEdge()) &&
                    (dest->y + dest->h > gameViewport->display.y && dest->y < gameViewport->displayBottomEdge())) {
                    
                    renderLayers[render->layer].push_back({render->texture, dest, render->rotation});
                    render->renderIndex = index++;
                }
            }
        });
        */

        /*
        int nRenderedEntities = 0;
        for (int l = 0; l < NUM_RENDER_LAYERS; l++) {
            std::vector<RenderTarget>& renderLayer = renderLayers[l];
            for (size_t r = 0; r < renderLayer.size(); r++) {
                SDL_RenderCopyExF(renderer, renderLayer[r].texture, NULL, renderLayer[r].destination, renderLayer[r].degreesRotation, NULL, SDL_FLIP_NONE);
                nRenderedEntities++;
            }
        }*/

        /*
        ecs.ForEach<ECS::EntityQuery<
            ECS::RequireComponents<EC::Render, EC::Health>
        >>([&](EntityT<EC::Health, EC::Render> entity){
            auto destRect = &ecs.Get<EC::Render>(entity)->destination;
            SDL_Rect _dest = FC_GetBounds(FreeSans, destRect->x, destRect->y, FC_ALIGN_LEFT, FC_MakeScale(0.5, 0.5), "%.1f", entity.Get<const EC::Health>(&ecs)->health);
            auto dest = &_dest;
            if ((dest->x + dest->w > gameViewport->display.x && dest->x < gameViewport->displayRightEdge()) &&
                (dest->y + dest->h > gameViewport->display.y && dest->y < gameViewport->displayBottomEdge())) {
                FC_DrawScale(FreeSans, renderer, destRect->x, destRect->y, FC_MakeScale(0.5, 0.5), "%.1f", entity.Get<const EC::Health>(&ecs)->health);
            }
    
        });
        */

        if (Debug->settings.drawEntityIDs) {
            int nRenderedIDs = 0;
            ecs.ForEach<Query>([&](Entity entity){
                auto destRect = &ecs.Get<EC::Render>(entity)->destination;
                // TODO: TEXT RENDER
                /*
                SDL_Rect _dest = FC_GetBounds(FreeSans, destRect->x, destRect->y, FC_ALIGN_LEFT, FC_MakeScale(0.5, 0.5), "%u", entity.id);
                auto dest = &_dest;
                if ((dest->x + dest->w > gameViewport->display.x && dest->x < gameViewport->displayRightEdge()) &&
                    (dest->y + dest->h > gameViewport->display.y && dest->y < gameViewport->displayBottomEdge())) {
                    FC_DrawScale(FreeSans, renderer, destRect->x, destRect->y, FC_MakeScale(0.5, 0.5), "%u", entity.id);
                    nRenderedIDs++;
                }
                */
            });
            Log.Info("rendered %d ids", nRenderedIDs);
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
/*
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

        ForEach<EC::Health, EC::Render>([&](EntityT<EC::Health, EC::Render> entity){   
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

*/

#endif