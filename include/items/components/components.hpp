#ifndef ITEMS_COMPONENTS_INCLUDED
#define ITEMS_COMPONENTS_INCLUDED

#include "utils/ints.hpp"
#include "ECS/generic/components.hpp"
#include "items/Item.hpp"

struct Game;

// forward declare this
typedef Uint16 TileType;

namespace items {

namespace ComponentIDs {
    #define ITEM_REGULAR_COMPONENTS_LIST Durability, Wetness, TileC, Display, Placeable
    #define ITEM_PROTO_COMPONENTS_LIST StackSize, Edible, Fuel, Wettable, StartDurability, Usable
    #define ITEM_COMPONENTS_LIST ITEM_REGULAR_COMPONENTS_LIST, ITEM_PROTO_COMPONENTS_LIST
    GEN_IDS(ComponentIDs, ComponentID, ITEM_COMPONENTS_LIST, Count);
}

namespace ITC {
    constexpr auto TotalNumComponents = ComponentIDs::Count;

    BEGIN_COMPONENT(Durability)
        int level;
    END_COMPONENT(Durability)

    BEGIN_COMPONENT(Wetness)
        int h;
    END_COMPONENT(Wetness)

    // unused
    BEGIN_COMPONENT(TileC)
        TileType type;
    END_COMPONENT(TileC)

    BEGIN_COMPONENT(Display)
        TextureID inventoryIcon;
    END_COMPONENT(Display)

    BEGIN_COMPONENT(Placeable)
        TileType tile;
    END_COMPONENT(Placeable)

    namespace Proto {
        BEGIN_PROTO_COMPONENT(StackSize)
            items::ItemQuantity quantity;
        END_PROTO_COMPONENT(StackSize)

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

        BEGIN_PROTO_COMPONENT(Usable)
            bool (*onUse)(Game*); // return: if the item was used successfully
            bool oneTimeUse; // if the item should be destroyed after being used
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