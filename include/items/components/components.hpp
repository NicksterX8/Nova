#ifndef ITEMS_COMPONENTS_INCLUDED
#define ITEMS_COMPONENTS_INCLUDED

#include "utils/ints.hpp"
#include "ECS/generic/components.hpp"
#include "items/Item.hpp"
#include <functional>

// forward declare this
typedef Uint16 TileType;

namespace items {

namespace ComponentIDs {
    #define ITEM_REGULAR_COMPONENTS_LIST Durability, Wetness, TileC, Display
    #define ITEM_PROTO_COMPONENTS_LIST Edible, Fuel, Wettable, StartDurability, Placeable, Usable
    #define ITEM_COMPONENTS_LIST ITEM_REGULAR_COMPONENTS_LIST, ITEM_PROTO_COMPONENTS_LIST
    GEN_IDS(ComponentIDs, ComponentID, ITEM_COMPONENTS_LIST, Count);
}

#define BEGIN_COMPONENT(name) struct name {\
    constexpr static ComponentID ID = ComponentIDs::name;\
    constexpr static bool PROTOTYPE = false;\
    constexpr static const char* NAME = TOSTRING(name);

#define END_COMPONENT(name) }; static_assert(true | ComponentIDs::name, "Checking for id");

#define BEGIN_PROTO_COMPONENT(name) struct name {\
    constexpr static ComponentID ID = ComponentIDs::name;\
    constexpr static bool PROTOTYPE = true;\
    constexpr static const char* NAME = TOSTRING(name);

#define END_PROTO_COMPONENT(name) }; static_assert(true | ComponentIDs::name, "Checking for id");

#define FLAG_COMPONENT(name) BEGIN_COMPONENT(name) END_COMPONENT(name)
#define FLAG_PROTO_COMPONENT(name) BEGIN_PROTO_COMPONENT(name) END_PROTO_COMPONENT(name)

namespace ITC {
    constexpr auto TotalNumComponents = ComponentIDs::Count;

    BEGIN_COMPONENT(Durability)
        int level;
    END_COMPONENT(Durability)

    BEGIN_COMPONENT(Wetness)
        int h;
    END_COMPONENT(Wetness)

    BEGIN_COMPONENT(TileC)
        TileType type;
    END_COMPONENT(TileC)

    BEGIN_COMPONENT(Display)
        TextureID inventoryIcon;
    END_COMPONENT(Display)

    namespace Proto {
        BEGIN_PROTO_COMPONENT(Edible)
            float hungerValue;
            float saturation;
        END_PROTO_COMPONENT(Edible)

        FLAG_PROTO_COMPONENT(Wettable)

        BEGIN_PROTO_COMPONENT(StartDurability)
            Durability durability;
        END_PROTO_COMPONENT(StartDurability)

        BEGIN_PROTO_COMPONENT(Fuel)
            float value;
            float duration;
        END_PROTO_COMPONENT(Fuel)

        BEGIN_PROTO_COMPONENT(Placeable)
            TileType tile;
        END_PROTO_COMPONENT(Placeable)

        BEGIN_PROTO_COMPONENT(Usable)
            std::function<void()> onUse;
        END_PROTO_COMPONENT(Usable)
    }

    using namespace Proto;
}

#undef BEGIN_COMPONENT
#undef END_COMPONENT
#undef FLAG_COMPONENT

}

namespace ITC = items::ITC;

#endif