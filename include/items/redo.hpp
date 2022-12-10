#include "utils/ints.hpp"
#include "utils/templates.hpp"
#include "My/Exp.hpp"
#include "My/Bitset.hpp"
#include "My/Vec.hpp"
#include "My/SparseSets.hpp"
#include "generic.hpp"
#include "Prototype.hpp"

namespace items {

struct IDGetter {
    template<class C>
    constexpr Sint16 get() const {
        return C::ID;
    }
};

#define MAX_ITEM_COMPONENTS 64

using ItemECS = ECS_Utils<MAX_ITEM_COMPONENTS, IDGetter>;
constexpr ItemECS ItemEcs;
using ComponentID = ItemECS::ComponentID;
using ComponentPtr = ItemECS::ComponentPtr;
using ComponentSignature = ItemECS::Signature; // 64 components or flags is probably good
struct ComponentManager : ItemECS::ArchetypalComponentManager {

};
using ComponentsAddress = ComponentManager::ElementAddress;

using ItemType = Uint16; // 65k should be way more than enough
using ItemQuantity = Sint32;
constexpr ItemQuantity ItemQuantityInfinity = (ItemQuantity)-1;

struct Item {
    ComponentSignature signature;
    ComponentsAddress componentsLoc;
    ItemType type;
};

// item stack, independent of inventory
struct ItemStack {
    ComponentSignature signature;
    ItemQuantity quantity;
    ComponentsAddress componentsLoc;
    ItemType type;

    Item item() const {
        return {signature, componentsLoc, type};
    }
};

namespace ItemTypes {
    enum IDs : ItemType {
        None=0,
        Grenade
    };
}

/*
struct Inventory {
    using SizeType = int;

    //ItemType*       types;
    //ItemSignature*  signatures;
    //ItemInstanceID* ids;
    //ItemQuantity*   quantities;

    void* buffer; // buffer for all 4 arrays

    ItemTypeID* types() {
        return (ItemTypeID*)buffer;
    }

    ComponentSignature* signatures() {
        return (ComponentSignature*)(types() + capacity);
    }

    ItemInstanceID* components() {
        return (ItemInstanceID*)(signatures() + capacity);
    }

    ItemQuantity* quantities() {
        return (ItemQuantity*)(ids() + capacity);
    }

    ItemTypeID& type(SizeType slot) {
        return types()[slot];
    }

    ComponentSignature& signature(SizeType slot) {
        return signatures()[slot];
    }

    ItemInstanceID& id(SizeType slot) {
        return ids()[slot];
    }

    ItemQuantity& quantity(SizeType slot) {
        return quantities()[slot];
    }

    // My::Vec< My::Vec<char> > components; // vec of array of components for each type
    //ItemComponentManager* components;
    
    SizeType size;
    SizeType capacity;

    // possible additional info like first and last items and stuff
    SizeType numPopulatedSlots;
    SizeType padding1;
    SizeType padding2;
    SizeType padding3;
};
*/

}