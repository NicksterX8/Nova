#include "PlayerControls.hpp"

MouseState getMouseState() {
    int mouseX,mouseY;
    Uint32 buttons = SDL_GetMouseState(&mouseX,&mouseY);
    mouseX *= SDLPixelScale;
    mouseY *= SDLPixelScale;
    return {
        mouseX,
        mouseY,
        buttons
    };
}

OptionalEntity<EC::Position, EC::Size, EC::Render>
findPlayerFocusedEntity(const ECS* ecs, const ChunkMap& chunkmap, Vec2 playerMousePos) {
    Vec2 target = playerMousePos;
    OptionalEntity<EC::Position, EC::Size, EC::Render> focusedEntity;
    int focusedEntityLayer = RenderLayer::Lowest;
    Uint32 focusedEntityRenderIndex = 0;
    forEachEntityNearPoint(ecs, &chunkmap, target,
    [&](EntityType<EC::Position> entity){
        if (!entity.Has<EC::Size, EC::Render>()) {
            return 0;
        }

        auto render = entity.Get<EC::Render>();
        int entityLayer = render->layer;
        Uint32 entityRenderIndex = render->renderIndex;

        if (pointInsideEntityBounds(target, entity.cast<EC::Position, EC::Size>())) {
            if (entityLayer > focusedEntityLayer || (entityLayer == focusedEntityLayer && entityRenderIndex > focusedEntityRenderIndex)) {
                focusedEntity = entity.cast<EC::Position, EC::Size, EC::Render>();
                focusedEntityLayer = entityLayer;
                focusedEntityRenderIndex = entityRenderIndex;
            }
        }

        return 0;
    });
    return focusedEntity;
}