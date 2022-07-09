#ifndef PLAYER_INCLUDED
#define PLAYER_INCLUDED

#include <SDL2/SDL.h>
#include "Tiles.hpp"
#include "Textures.hpp"
#include "Entities/Entities.hpp"

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
    OptionalEntityT<Entities::Player> entity;
    Direction facingDirection = DirectionUp;

    unsigned int numHotbarSlots = 9;
    ItemStack* heldItemStack = NULL;
    int selectedHotbarStack = 0;

    OptionalEntity<> selectedEntity;

    int grenadeThrowCooldown = 0;
    static constexpr int GrenadeCooldown = 10;

    Player() {}

    Player(ECS* ecs) {
        entity = Entities::Player(ecs, Vec2(0.0, 0.0));
    }

    Vec2 getSize() const {
        if (entity.Exists()) {
            auto size = entity.Get<EC::Size>();
            if (size) {
                return size->toVec2();
            }
        }
        return {1, 1};
    }

    Vec2 getPosition() const {
        if (entity.Exists()) {
            auto position = entity.Get<EC::Position>();
            if (position)
                return *position;
        }
        return {0, 0};
        
    }

    void setPosition(Vec2 pos) {
        if (entity.Exists()) {
            auto position = entity.Get<EC::Position>();
            if (position)
                *position = pos;
        }
    }

    Inventory* inventory() const {
        if (entity.Exists()) {
            auto component = entity.Get<EC::Inventory>();
            if (component) {
                return &component->inventory;
            }
        }
        return NULL;        
    }

    bool tryShootSandGun(ECS* ecs, Vec2 aimingPosition) {
        if (heldItemStack)
        if (heldItemStack->item == Items::SandGun) {
            Entity sand = ecs->New();
            ecs->Add<
                EC::Position,
                EC::Size,
                EC::Motion,
                EC::Render
            >(sand);

            // adjust position by half a tile to make the tile's center spawn on top of the player,
            // instead of the top left corner 
            *ecs->Get<EC::Position>(sand) = 
                EC::Position(getPosition().x - 0.5f, getPosition().y - 0.5f);
            *ecs->Get<EC::Size>(sand) = {1, 1};
            // adjust position by half a tile to make center of the sand go towards the aiming point,
            // instead of the top left corner
            *ecs->Get<EC::Motion>(sand) = 
                EC::Motion({aimingPosition.x - 0.5f, aimingPosition.y - 0.5f}, 2);
            *ecs->Get<EC::Render>(sand) = EC::Render(Textures.Tiles.sand, RenderLayer::Particles);
            return true;
        }
        return false;
    }

    bool tryThrowGrenade(ECS* ecs, Vec2 aimingPosition) {
        if (heldItemStack)
        if (heldItemStack->item == Items::Grenade && heldItemStack->quantity > 0) {
            if (grenadeThrowCooldown <= 0) {
                Entity grenade = Entities::ThrownGrenade(ecs, getPosition(), aimingPosition);
                grenadeThrowCooldown = GrenadeCooldown;
                heldItemStack->reduceQuantity(1);
            }
        }
        
        return false;
    }

    void selectHotbarSlot(int index) {
        selectedHotbarStack = index;
        if (inventory()->items[index].item) {
            heldItemStack = &inventory()->items[index];
        }
    }

    void pickupItem(ItemStack* stack) {
        releaseHeldItem();
        heldItemStack = stack;
    }

    void releaseHeldItem() {
        if (heldItemStack) {
            heldItemStack = NULL;
        }
    }

    bool tryPlaceHeldItem(Tile* targetTile) {
        if (heldItemStack)
        if (ItemData[heldItemStack->item].flags & Items::Placeable) {
            if (heldItemStack->quantity > 0) {
                TileType newTileType = ItemData[heldItemStack->item].placeable.tile;
                if (targetTile->type != newTileType) {
                    // place it
                    targetTile->type = newTileType;
                    heldItemStack->reduceQuantity(1);
                    // when held item stack runs out, automatically drop it so the player isnt holding nothing
                    if (!heldItemStack->item) {
                        heldItemStack = NULL;
                    }
                    return true;
                }
            }
        }
        return false;
    }

    bool tryMineTile(Tile* tile) {
        if (TileTypeData[tile->type].flags & TileTypes::Mineable) {
            // add tile to inventory
            inventory()->addItemStack(ItemStack(TileTypeData[tile->type].mineable.item));
            tile->type = TileTypes::Space;
            return true;
        }
        return false;
    }
};

#endif