#ifndef PLAYER_INCLUDED
#define PLAYER_INCLUDED

#include <SDL2/SDL.h>
#include "Tiles.hpp"
#include "rendering/textures.hpp"
#include "ECS/entities/entities.hpp"
#include "ECS/entities/methods.hpp"
#include "items/items.hpp"
#include "items/Item.hpp"

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
    const EntityWorld* ecs = NULL;
    Direction facingDirection = DirectionUp;

    unsigned int numHotbarSlots = 9;
    ItemStack* heldItemStack = NULL; // need more information for this, pointer isnt enough
    int selectedHotbarStack = -1; // -1 means no slot is selected. TODO: rename this to slot

    OptionalEntity<> selectedEntity;

    int grenadeThrowCooldown = 0;
    static constexpr int GrenadeCooldown = 10;

    Player() {}

    Player(EntityWorld* ecs, Vec2 startPosition, ItemManager& inventoryAllocator) : ecs(ecs) {
        entity = Entities::Player(ecs, startPosition, inventoryAllocator);
    }

    template<class C>
    const C* get() const {
        return ecs->Get<C>(entity);
    }

    template<class C>
    C* get() {
        return ecs->Get<C>(entity);
    }

    void setPosition(ChunkMap& chunkmap, Vec2 pos) {
        if (entity.Exists(ecs)) {
            auto position = entity.Get<EC::Position>(ecs);
            if (position) {
                Vec2 oldPosition = position->vec2();
                position->x = pos.x;
                position->y = pos.y;
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
        if (heldItemStack && heldItemStack->item.type == ItemTypes::SandGun) {
            Entity sand = ecs->New("sand");
            MARK_START_ENTITY_CREATION(ecs);
            ecs->Add<
                EC::Position,
                EC::ViewBox,
                EC::Motion,
                EC::Render
            >(sand);

            // adjust position by half a tile to make the tile's center spawn on top of the player,
            // instead of the top left corner 
            Vec2 position = get<EC::Position>()->vec2() - Vec2{0.5f, 0.5f};

            constexpr Vec2 size = {1.0f, 1.0f};
            *ecs->Get<EC::ViewBox>(sand) = EC::ViewBottomLeft1x1;

            // adjust position by half a tile to make center of the sand go towards the aiming point,
            // instead of the top left corner
            aimingPosition -= Vec2{0.5f, 0.5f};
            constexpr float speed = 2.0f;
            *ecs->Get<EC::Motion>(sand) = EC::Motion(aimingPosition, speed);
            *ecs->Get<EC::Render>(sand) = EC::Render(TextureIDs::Tiles::Sand, RenderLayers::Particles);
            MARK_END_ENTITY_CREATION(ecs);
            return true;
        }
        return false;
    }

    bool tryThrowGrenade(EntityWorld* ecs, Vec2 aimingPosition) {
        if (heldItemStack)
        if (heldItemStack->item.type == ItemTypes::Grenade && heldItemStack->quantity > 0) {
            if (grenadeThrowCooldown <= 0) {
                Entities::ThrownGrenade(ecs, get<EC::Position>()->vec2(), aimingPosition);
                grenadeThrowCooldown = GrenadeCooldown;
                heldItemStack->reduceQuantity(1);
                return true;
            }
        }
        
        return false;
    }

    void selectHotbarSlot(int index) {
        if (!entity.Has<EC::Inventory>(ecs)) return;

        selectedHotbarStack = index;
        auto* slot = &inventory()->get(index);
        if (!slot->empty()) {
            heldItemStack = slot;
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

    bool canPlaceItemStack(ItemStack stack) {
        if (heldItemStack->item.has<ITC::Placeable>()) {
            if (stack.quantity > 0) {
                return true;
            }
        }
        return false;
    }

    bool tryPlaceItemStack(ItemStack stack, Tile* targetTile, ItemManager& itemManager) {
        if (canPlaceItemStack(stack)) {
            auto placeable = items::getComponent<ITC::Placeable>(stack.item, itemManager);
            TileType newTileType = placeable->tile;
            if (targetTile->type != newTileType) {
                // place it
                targetTile->type = newTileType;
                heldItemStack->reduceQuantity(1);
                // when held item stack runs out, automatically drop it so the player isnt holding nothing
                if (stack.empty()) {
                    heldItemStack = NULL;
                }
                return true;
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