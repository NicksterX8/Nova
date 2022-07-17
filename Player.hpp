#ifndef PLAYER_INCLUDED
#define PLAYER_INCLUDED

#include <SDL2/SDL.h>
#include "Tiles.hpp"
#include "Textures.hpp"
#include "Entities/Entities.hpp"
#include "Entities/Methods.hpp"

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
    EntityWorld* ecs = NULL;
    Direction facingDirection = DirectionUp;

    unsigned int numHotbarSlots = 9;
    ItemStack* heldItemStack = NULL;
    int selectedHotbarStack = 0;

    OptionalEntity<> selectedEntity;

    int grenadeThrowCooldown = 0;
    static constexpr int GrenadeCooldown = 10;

    Player() {}

    Player(EntityWorld* ecs) : ecs(ecs) {
        entity = Entities::Player(ecs, Vec2(0.0, 0.0));
    }

    Vec2 getSize() const {
        if (entity.Exists(ecs)) {
            auto size = entity.Get<EC::Size>(ecs);
            if (size) {
                return size->vec2();
            }
        }
        return {1, 1};
    }

    Vec2 getPosition() const {
        if (entity.Exists(ecs)) {
            auto position = entity.Get<EC::Position>(ecs);
            if (position)
                return position->vec2();
        }
        return {0, 0};
        
    }

    void setPosition(ChunkMap& chunkmap, Vec2 pos) {
        if (entity.Exists(ecs)) {
            auto position = entity.Get<EC::Position>(ecs);
            if (position) {
                Vec2 oldPosition = position->vec2();
                *position = pos;
                entityPositionChanged(&chunkmap, ecs, entity, oldPosition);
            }
        }
    }

    const Inventory* inventory() const {
        if (entity.Exists(ecs)) {
            auto component = entity.Get<EC::Inventory>(ecs);
            if (component) {
                return &component->inventory;
            }
        }
        return NULL;        
    }
    Inventory* inventory() {
        if (entity.Exists(ecs)) {
            auto component = entity.Get<EC::Inventory>(ecs);
            if (component) {
                return &component->inventory;
            }
        }
        return NULL;
    }

    bool tryShootSandGun(EntityWorld* ecs, Vec2 aimingPosition) {
        if (heldItemStack)
        if (heldItemStack->item == Items::SandGun) {
            Entity sand = ecs->New("sand");
            MARK_START_ENTITY_CREATION(ecs);
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
            MARK_END_ENTITY_CREATION(ecs);
            return true;
        }
        return false;
    }

    bool tryThrowGrenade(EntityWorld* ecs, Vec2 aimingPosition) {
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
        if (inventory()->get(index).item) {
            heldItemStack = &inventory()->get(index);
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