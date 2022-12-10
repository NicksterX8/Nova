#ifndef ITEMS_ITEMS_INCLUDED
#define ITEMS_ITEMS_INCLUDED

#include "Item.hpp"
#include "components.hpp"
#include "Tiles.hpp"
#include "manager.hpp"
#include "Inventory.hpp"

namespace items {

template<typename itemID>
ItemStack makeItemStack() {
    NewItemStack stack;
    stack.id = itemID;
    stack.components = nullptr;
    stack.quantity = 0;
    return stack;
}



ItemStack makeGrenadeStack(ComponentManager& manager, ItemQuantity quantity = 1) {
    ItemStack stack(ItemTypes::Grenade, quantity);
    addComponent<ITC::Wetness>(stack, manager, {10});
    addComponent(stack, manager, ITC::Durability{10});
    return stack;
}
/*
template<ItemID itemID, typename... Components>
struct CustomItemStack : ItemStackBase, SECS::CustomEntity<Components...> {
    CustomItemStack(ItemManager& manager) : ItemStackBase(itemID), SECS::CustomEntity<Components...>(manager) {

    }

    CustomItemStack(ItemManager& manager, const Components& ...components)
    : ItemStackBase(itemID), SECS::CustomEntity(manager, components...) {

    }
};


namespace Items {
    using namespace ITC;

    template<ItemID itemID, typename... Components>
    struct Base : ItemStack {
        Base(ItemManager& manager) : ItemStack(itemID) {
            //this->addComponent(manager, )
        }
    };
    /*
    struct GrenadeItem : CustomItemStack<ItemIDs::Grenade, Wetness, Durability> {
        GrenadeItem(ItemManager& manager) : CustomItemStack(manager) {
            
        }

        GrenadeItem(ItemManager& manager, ItemQuantity quantity = 1) : CustomItemStack(manager, Wetness{0}, Durability{100.0f}) {

        }
    };
    /

    struct Tile : Base<ItemIDs::Tile> {
        Tile(ItemManager& manager, TileType tileType) : Base(manager) {
            add<ITC::Placeable>(manager, {tileType});
        }
    };
}

template<typename T>
using ConstRef = const T&;

template<ItemID itemID, class... Components>
ItemStack maker(ItemManager& manager, const Components& ...components) {
    auto stack = ItemStack(itemID);
    FOR_EACH_VAR_TYPE(stack.add<Components>(manager, components));
    return stack;
}

ItemStack TileMaker(ItemManager& manager, TileType tileType) {
    /*
    auto s = ItemStack(ItemIDs::Tile);
    s.addComponent<ITC::Placeable>(manager, {tileType});
    return s;
    /
    
    return maker<ItemTypes::Tile>(manager, ITC::Placeable{tileType});
}

struct ItemCreator {
    ItemManager* manager;

    template<class T, typename... Params>
    auto make(ItemQuantity quantity, Params ...params) {
        auto stack = T(*manager, params);
        stack.quantity = quantity;
        return stack;
    }

    template<typename... Params, class Functor>
    ItemStack make(Functor functor, Params ...params) {
        auto stack = functor(*manager, params);
        stack.quantity = quantity;
        return stack;
    }

    ItemStack grenade() {
        
    }
};
*/

namespace Prototypes {
    namespace IDs = ItemTypes;
    using namespace ITC::Proto;

    template<class... Components>
    ItemPrototype New(PrototypeManager* manager, ItemType typeID) {
        auto componentsStorage = manager->allocateComponents(signature);
        constexpr auto signature = getComponentSignature<Components...>();
        return ItemPrototype(componentsStorage, typeID, manager->componentInfo, signature);
    }

    struct None : ItemPrototype {
        None(PrototypeManager* manager) : ItemPrototype(New(manager, IDs::None)) {

        }
    };

    struct Grenade : ItemPrototype {
        Grenade(PrototypeManager* manager) : ItemPrototype(New<Fuel, Edible>(manager, IDs::Grenade)) {
            this->stackSize = 64;
            set<Fuel>({100.0f, 20.0f});
            set<Edible>({.hungerValue = 1.0f, .saturation = 2.0f});
        }

        void onCreate() {

        }

        void onDestroy() {

        }
    };

    void user() {
        
        
    }

    /*
    template<class... Components>
    using Prototype = ItemPrototypeSpecialization<Components...>;

    struct GrenadePrototype : Prototype<ITPC::Wettable, ITPC::StartDurability> {
        GrenadePrototype() : Prototype<ITPC::Wettable, ITPC::StartDurability>() {

        }
    };
    */
};

PrototypeManager* makePrototypes(ComponentInfoRef componentInfo) {
    auto manager = new PrototypeManager(componentInfo, ItemTypes::Count);
    namespace ids = ItemTypes;
    using namespace Prototypes;
    manager->add(Prototypes::None(manager));
    manager->add(Prototypes::Grenade(manager));

    // check if we forgot any
    for (ItemType type = ids::None+1; type < ItemTypes::Count; type++) {
        if (manager->prototypes[type].id == ids::None) {
            LogError("FAIL ON PROTOTYPE MAKING!");
            assert(false);
        }
    }
    return manager;
}

inline const ItemPrototype* getPrototype(ItemType type, const ItemManager& manager) {
    //TODO: check for null
    return &manager.prototypeManager.prototypes[type];
}

template<class C>
C* getProtoComponent(const Item& item, const ItemManager& manager) {
    // TODO: check for null
    return getPrototype(item.type, manager)->get<C>();
}

template<class C>
C* getComponent(const Item& item, const ItemManager& manager) {
    if (C::PROTOTYPE) {
        return getProtoComponent<C>(item, manager);
    } else {
        return getRegularComponent<C>(item, manager);
    }
}

inline float getFuelValue(ItemManager& manager, ItemStack stack) {
    auto fuelComponent = getComponent<ITC::Fuel>(stack.item, manager);

    float itemFuelValue = fuelComponent ? fuelComponent->value : NAN;
    return stack.quantity * itemFuelValue;
}

void serializeItem(Item item, void* out) {
    
}

void deserializeItem(Item item, void* in) {
    
}

void serializeComponents() {
    
}

} // namespace items

#endif