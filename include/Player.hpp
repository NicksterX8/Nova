#ifndef PLAYER_INCLUDED
#define PLAYER_INCLUDED

#include <SDL3/SDL.h>
#include "Tiles.hpp"
#include "rendering/textures.hpp"
#include "world/entities/entities.hpp"
#include "world/entities/methods.hpp"
#include "items/items.hpp"
#include"items/manager.hpp"
#include "items/Item.hpp"

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
    Entity entity;
    const EntityWorld* ecs = NULL;

    unsigned int numHotbarSlots = 9;
    ItemHold heldItemStack;
    int selectedHotbarSlot = -1; // -1 means no slot is selected. TODO: rename this to slot

    Entity selectedEntity = NullEntity;

    int grenadeThrowCooldown = 0;
    static constexpr int GrenadeCooldown = 10;

    Player() {}

    Player(EntityWorld* ecs, Vec2 startPosition, ItemManager& inventoryAllocator) : ecs(ecs) {
        entity = World::Entities::Player::make(ecs, startPosition, inventoryAllocator);
        World::Entities::giveName(entity, "player", ecs);
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
        if (ecs->EntityExists(entity)) {
            auto position = ecs->Get<World::EC::Position>(entity);
            if (position) {
                position->x = pos.x;
                position->y = pos.y;
            } else {
                LogError("No dynamic position component on player!");
            }
        }
    }

    const Inventory* inventory() const {
        if (ecs->EntityExists(entity)) {
            auto component = ecs->Get<World::EC::Inventory>(entity);
            if (component) {
                return &component->inventory;
            }
        }
        return nullptr;        
    }
    Inventory* inventory() {
        if (ecs->EntityExists(entity)) {
            auto component = ecs->Get<World::EC::Inventory>(entity);
            if (component) {
                return &component->inventory;
            }
        }
        return nullptr;
    }

    float getSpeed() const {
        return PLAYER_SPEED;
    }

    void selectHotbarSlot(int index) {
        if (!ecs->EntityHas<World::EC::Inventory>(entity)) return;

        selectedHotbarSlot = index;
        auto* slot = &inventory()->get(index);
        if (!slot->empty()) { 
            if (heldItemStack.type != ItemHold::Value) {
                heldItemStack = ItemHold(ItemHold::Inventory, ItemHold::DataType(slot));
            }
        }
    }

    bool isHoldingItem() {
        return (bool)heldItemStack;
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
        if (stack.quantity > 0 && itemManager.entityHas<ITC::Placeable>(stack.item)) {
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