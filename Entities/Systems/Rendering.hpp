#ifndef RENDERING_SYSTEMS_INCLUDED
#define RENDERING_SYSTEMS_INCLUDED

#include <vector>
#include "../EntitySystem.hpp"
#include "../EntityManager.hpp"
#include "../../GameViewport.hpp"
#include "../../Log.hpp"
#include "../../Rendering/Drawing.hpp"
#include "../../Debug.hpp"

class RenderSizeSystem : public EntitySystem {
public:
    RenderSizeSystem() : EntitySystem() {

    }

    bool Query(ComponentFlags entitySignature) const {
        return ((entitySignature & componentSignature<SizeComponent, RenderComponent>()) == componentSignature<SizeComponent, RenderComponent>());
    }

    void Update(ECS* ecs, const GameViewport* gameViewport) {
        ForEach([&](Entity entity){
            SDL_FRect* renderDst = &ecs->Get<RenderComponent>(entity)->destination;
            renderDst->w = ecs->Get<SizeComponent>(entity)->width * TilePixels;
            renderDst->h = ecs->Get<SizeComponent>(entity)->height * TilePixels;
        });
    }

private:

};

class RenderPositionSystem : public EntitySystem {
public:
    RenderPositionSystem() : EntitySystem() {

    }

    bool Query(ComponentFlags entitySignature) const {
        return ((entitySignature & componentSignature<PositionComponent, RenderComponent>()) == componentSignature<PositionComponent, RenderComponent>());
    }

    void Update(ECS* ecs, const GameViewport* gameViewport) {
        ForEach([&](Entity entity){
            // copy rotation to render component
            SDL_FRect* renderDst = &ecs->Get<RenderComponent>(entity)->destination;
            Vec2 dstPosition = gameViewport->worldToPixelPositionF(*ecs->Get<PositionComponent>(entity));
            renderDst->x = dstPosition.x;
            renderDst->y = dstPosition.y;
        });
    }

private:

};

// draw simple render entities
class RenderSystem : public EntitySystem {
    SDL_Renderer* renderer = NULL;
    float scale = 1.0f;
    const GameViewport* gameViewport = NULL;
    ComponentFlags signature;

    struct RenderTarget {
        SDL_Texture* texture;
        SDL_FRect* destination;
        float degreesRotation;
    };
public:
    RenderSystem() {}

    RenderSystem(SDL_Renderer* renderer, const GameViewport* gameViewport) : EntitySystem() {
        this->renderer = renderer;
        this->gameViewport = gameViewport;
        signature = componentSignature<RenderComponent>();
    }

    bool Query(ComponentFlags entitySignature) const {
        if ((entitySignature & signature) == signature) {
            return true;
        }
        return false;
    }

    void Update(ECS* ecs, float scale) {
        this->scale = scale;

        ForEach<RenderComponent, CenteredRenderFlagComponent, SizeComponent>(ecs, [&](auto entity){
            SDL_FRect* renderDst = &ecs->Get<RenderComponent>(entity)->destination;
            renderDst->x -= renderDst->w / 2.0f;
            renderDst->y -= renderDst->h / 2.0f;
        });

        std::vector<RenderTarget> renderLayers[NUM_RENDER_LAYERS];
        ForEach([&](Entity entity){
            RenderComponent* render = ecs->Get<RenderComponent>(entity);
            if (render->texture != NULL) {
                SDL_FRect* dest = &render->destination;
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

        ForEach<HealthComponent, RenderComponent>(ecs, [&](EntityType<HealthComponent, RenderComponent> entity){
            auto dest = &ecs->Get<RenderComponent>(entity)->destination;
            FC_DrawScale(FreeSans, renderer, dest->x + dest->w/2.0f, dest->y + dest->h, FC_MakeScale(0.5, 0.5), "%.1f", entity.Get<HealthComponent>()->healthValue);
        });

        if (Debug.settings.drawEntityIDs) {
            ForEach([&](Entity entity){
                auto dest = &ecs->Get<RenderComponent>(entity)->destination;
                FC_DrawScale(FreeSans, renderer, dest->x, dest->y, FC_MakeScale(0.5, 0.5), "%u", entity.id);
            });
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