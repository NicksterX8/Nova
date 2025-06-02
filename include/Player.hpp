#ifndef PLAYER_INCLUDED
#define PLAYER_INCLUDED

#include <SDL2/SDL.h>
#include "Tiles.hpp"
#include "rendering/textures.hpp"
#include "ECS/entities/entities.hpp"
#include "ECS/entities/methods.hpp"
#include "items/items.hpp"
#include"items/manager.hpp"
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

struct ItemHold {
    union DataType {
        ItemStack value;
        ItemStack* pointer;

        DataType(ItemStack* ptr) : pointer(ptr) {

        }

        DataType(const ItemStack& value) : value(value) {}
    } data;

    enum Type {
        Null,
        Inventory, // ref
        Value
    } type;

    ItemHold() : data(nullptr), type(Null) {}
    ItemHold(const ItemStack& value) : data(value), type(Value) {}
    ItemHold(Type type, DataType value) : data(value), type(type) {}

    ItemStack* get() {
        if (type == Value) return &data.value;
        else return data.pointer; // could probably just always return data.pointer
    }

    const ItemStack* get() const {
        if (type == Value) return &data.value;
        else return data.pointer; // could probably just always return data.pointer
    }

    operator bool() const {
        auto item = this->get();
        return item && !item->empty();
    }
};

struct Player {
    OptionalEntityT<Entities::Player> entity;
    const EntityWorld* ecs = NULL;
    Direction facingDirection = DirectionUp;

    unsigned int numHotbarSlots = 9;
    ItemHold heldItemStack;
    int selectedHotbarStack = -1; // -1 means no slot is selected. TODO: rename this to slot

    OptionalEntity<> selectedEntity;

    int grenadeThrowCooldown = 0;
    static constexpr int GrenadeCooldown = 10;

    Player() {}

    Player(EntityWorld* ecs, Vec2 startPosition, ItemManager& inventoryAllocator) : ecs(ecs) {
        entity = Entities::Player(ecs, startPosition, inventoryAllocator);
        Entities::giveName(entity, "player", ecs);
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
                position->x = pos.x;
                position->y = pos.y;
            } else {
                LogError("No dynamic position component on player!");
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

    void selectHotbarSlot(int index) {
        if (!entity.Has<EC::Inventory>(ecs)) return;

        selectedHotbarStack = index;
        auto* slot = &inventory()->get(index);
        if (!slot->empty()) {
            heldItemStack = ItemHold(*slot);
        }
    }

    bool isHoldingItem() {
        return heldItemStack.type != ItemHold::Null;
    }

    bool pickupItem(const ItemStack& stack) {
        if (isHoldingItem()) return false;
        heldItemStack = ItemHold(ItemHold::Value, stack);
        return true;
    }

    void releaseHeldItem() {
        if (heldItemStack.type == ItemHold::Inventory) {
        } else if (heldItemStack.type == ItemHold::Value) {
            inventory()->addItemStack(*heldItemStack.get());
        }
        heldItemStack = ItemHold();
    }

    bool canPlaceItemStack(ItemStack stack, const ItemManager& itemManager) {
        if (stack.quantity > 0 && itemManager.elementHas<ITC::Placeable>(stack.item)) {
            return true;
        }
        return false;
    }

    bool tryPlaceItemStack(ItemStack* stack, Tile* targetTile, ItemManager& itemManager) {
        if (!stack || !targetTile) return false;
        if (canPlaceItemStack(*stack, itemManager)) {
            auto placeable = itemManager.getComponent<ITC::Placeable>(stack->item);
            TileType newTileType = placeable->tile;
            if (targetTile->type != newTileType) {
                // place it
                targetTile->type = newTileType;
                stack->reduceQuantity(1);
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