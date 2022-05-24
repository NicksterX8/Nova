#ifndef PLAYER_INCLUDED
#define PLAYER_INCLUDED

#include <SDL2/SDL.h>
#include "Tiles.hpp"
#include "Textures.hpp"
#include "Entity.hpp"

enum Direction {
    DirectionUp,
    DirectionRight,
    DirectionLeft,
    DirectionDown,
    DirectionUpRight,
    DirectionDownRight,
    DirectionUpLeft,
    DirectionDownLeft
};

struct Player {
    float x = 0.0;
    float y = 0.0;
    Direction facingDirection = DirectionUp;

    Inventory inventory;
    ItemStack* heldItemStack = NULL;
    int selectedHotbarStack = 0;

    int grenadeThrowCooldown = 0;

    Player() {}
    Player(float x, float y): x(x), y(y) {

    }

    void shootSandGun(ECS* ecs, Vec2 aimingPosition) {
        Entity sand = ecs->New();
        ecs->Add<
            PositionComponent,
            SizeComponent,
            MotionComponent,
            RenderComponent
        >(sand);

        // adjust position by half a tile to make the tile's center spawn on top of the player,
        // instead of the top left corner 
        *ecs->Get<PositionComponent>(sand) = 
            PositionComponent(x - 0.5f, y - 0.5f);
        *ecs->Get<SizeComponent>(sand) = {1, 1};
        // adjust position by half a tile to make center of the sand go towards the aiming point,
        // instead of the top left corner
        *ecs->Get<MotionComponent>(sand) = 
            MotionComponent({aimingPosition.x - 0.5f, aimingPosition.y - 0.5f}, 2);
        *ecs->Get<RenderComponent>(sand) = RenderComponent(Textures.Tiles.sand, RenderLayer::Particles);
    }

    void throwGrenade(ECS* ecs, Vec2 aimingPosition) {
        if (grenadeThrowCooldown <= 0) {
            Entity grenade = Entities::ThrownGrenade(ecs, {x, y}, aimingPosition);
            grenadeThrowCooldown = 30;
        }
    }

    void selectHotbarSlot(int index) {
        selectedHotbarStack = index;
        heldItemStack = &inventory.items[index];
    }

    bool tryPlaceHeldItem(Tile* targetTile) {
        if (ItemData[heldItemStack->item].flags & Items::Placeable) {
            if (heldItemStack->quantity > 0) {
                TileType newTileType = ItemData[heldItemStack->item].placeable.tile;
                //if (targetTile->type == TileTypes::Space) {
                if (targetTile->type != newTileType) {
                    // place it
                    targetTile->type = newTileType;
                    heldItemStack->reduceQuantity(1);
                    return true;
                }
            }
        }
        return false;
    }

    bool tryMineTile(Tile* tile) {
        if (TileTypeData[tile->type].flags & TileTypes::Mineable) {
            // add tile to inventory
            inventory.addItemStack(ItemStack(TileTypeData[tile->type].mineable.item));
            tile->type = TileTypes::Space;
            return true;
        }
        return false;
    }
};

#endif