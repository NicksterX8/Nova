#include "PlayerControls.hpp"

MouseState getMouseState() {
    int mouseX,mouseY;
    Uint32 buttons = SDL_GetMouseState(&mouseX,&mouseY);
    mouseX *= SDL::pixelScale;
    mouseY *= SDL::pixelScale;
    return {
        mouseX,
        mouseY,
        buttons
    };
}

OptionalEntity<EC::Position, EC::Size, EC::Render>
findPlayerFocusedEntity(const ComponentManager<EC::Position, const EC::Size, const EC::Render>& ecs, const ChunkMap& chunkmap, Vec2 playerMousePos) {
    Vec2 target = playerMousePos;
    OptionalEntity<EC::Position, EC::Size, EC::Render> focusedEntity;
    int focusedEntityLayer = RenderLayer::Lowest;
    Uint32 focusedEntityRenderIndex = 0;
    forEachEntityNearPoint(ecs, &chunkmap, target,
    [&](EntityT<EC::Position> entity){
        if (!entity.Has<EC::Size, EC::Render>(&ecs)) {
            return 0;
        }

        auto render = entity.Get<const EC::Render>(&ecs);
        int entityLayer = render->layer;
        Uint32 entityRenderIndex = 0;

        if (pointInsideEntityBounds(target, entity.cast<EC::Position, EC::Size>(), ecs)) {
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