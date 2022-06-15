#ifndef RENDERING_SYSTEMS_INCLUDED
#define RENDERING_SYSTEMS_INCLUDED

#include <vector>
#include "../ECS/EntitySystem.hpp"
#include "../ECS/ECS.hpp"
#include "../GameViewport.hpp"
#include "../Log.hpp"
#include "../Rendering/Drawing.hpp"
#include "../Debug.hpp"

class RenderSizeSystem : public EntitySystem {
public:
    const GameViewport* gameViewport;

    #define SYSTEM_ACCESS(component) (sys.componentAccess[getID<component>()])

    RenderSizeSystem(ECS* ecs) : EntitySystem(ecs) {
        using namespace ComponentAccess;
        sys.GiveAccess<RenderComponent>(Read | Write);
        SYSTEM_ACCESS(SizeComponent) = Read;
    }

    bool Query(ComponentFlags entitySignature) const {
        return ((entitySignature & componentSignature<SizeComponent, RenderComponent>()) == componentSignature<SizeComponent, RenderComponent>());
    }

    void Update() {
        ForEach([&](Entity entity){
            SDL_FRect* renderDst = &sys.GetReadWrite<RenderComponent>(entity)->destination;
            renderDst->w = sys.GetReadOnly<SizeComponent>(entity)->width * TileWidth;
            renderDst->h = sys.GetReadOnly<SizeComponent>(entity)->height * TileHeight;
        });
    }

private:

};

class RenderPositionSystem : public EntitySystem {
public:
    const GameViewport* gameViewport;

    RenderPositionSystem(ECS* ecs) : EntitySystem(ecs) {
        using namespace ComponentAccess;
        sys.GiveAccess<PositionComponent>(Read);
        sys.GiveAccess<RenderComponent>(Read | Write);
    }

    bool Query(ComponentFlags entitySignature) const {
        return ((entitySignature & componentSignature<PositionComponent, RenderComponent>()) == componentSignature<PositionComponent, RenderComponent>());
    }

    void Update() {
        ForEach([&](Entity entity){
            // copy rotation to render component
            SDL_FRect* renderDst = &sys.GetReadWrite<RenderComponent>(entity)->destination;
            Vec2 dstPosition = gameViewport->worldToPixelPositionF(*sys.GetReadOnly<PositionComponent>(entity));
            renderDst->x = dstPosition.x;
            renderDst->y = dstPosition.y;
        });
    }

private:

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
        signature = componentSignature<RenderComponent>();

        using namespace ComponentAccess;
        sys.GiveAccess<RenderComponent>(ReadWrite);
        sys.GiveAccess<CenteredRenderFlagComponent>(Flag);
        sys.GiveAccess<SizeComponent>(Read);
        sys.GiveAccess<PositionComponent>(Read);
    }

    bool Query(ComponentFlags entitySignature) const {
        if ((entitySignature & signature) == signature) {
            return true;
        }
        return false;
    }

    void Update() {
        ForEach<RenderComponent, CenteredRenderFlagComponent, SizeComponent>([&](auto entity){
            SDL_FRect* renderDst = &sys.GetReadWrite<RenderComponent>(entity)->destination;
            renderDst->x -= renderDst->w / 2.0f;
            renderDst->y -= renderDst->h / 2.0f;
        });

        std::vector<RenderTarget> renderLayers[NUM_RENDER_LAYERS];
        ForEach([&](Entity entity){
            const RenderComponent* render = sys.GetReadOnly<RenderComponent>(entity);
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

        ForEach<HealthComponent, RenderComponent>([&](EntityType<HealthComponent, RenderComponent> entity){   
            auto destRect = &sys.GetReadOnly<RenderComponent>(entity)->destination;
            SDL_Rect _dest = FC_GetBounds(FreeSans, destRect->x, destRect->y, FC_ALIGN_LEFT, FC_MakeScale(0.5, 0.5), "%.1f", entity.Get<HealthComponent>()->healthValue);
            auto dest = &_dest;
            if ((dest->x + dest->w > gameViewport->display.x && dest->x < gameViewport->displayRightEdge()) &&
                (dest->y + dest->h > gameViewport->display.y && dest->y < gameViewport->displayBottomEdge())) {
                FC_DrawScale(FreeSans, renderer, destRect->x, destRect->y, FC_MakeScale(0.5, 0.5), "%.1f", entity.Get<HealthComponent>()->healthValue);
            }
    
        });

        if (Debug.settings.drawEntityIDs) {
            int nRenderedIDs = 0;
            ForEach([&](Entity entity){
                auto destRect = &sys.GetReadOnly<RenderComponent>(entity)->destination;
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

        if (Debug.settings.drawEntityRects) {
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