#ifndef ENTITY_INCLUDED
#define ENTITY_INCLUDED

#include <stdint.h>
#include <string.h>
#include <array>
#include "Component.hpp"
#include "utils/Log.hpp"
#include "constants.hpp"
#include "My/Bitset.hpp"
#include "My/String.hpp"
#include "Signature.hpp"

typedef uint32_t Uint32;

namespace ECS {

typedef Uint32 EntityID;
typedef Uint32 EntityVersion;
typedef Sint32 ComponentID;
constexpr ComponentID NullComponentID = -1;

template<class C>
constexpr ComponentID getID() {
    return C::ID;
}

struct IDGetter {
    template<class C>
    constexpr static ComponentID get() {
        return getID<C>();
    }
};

using ComponentFlags = Signature;

#define MAX_ENTITIES 64000
#define ECS_BAD_COMPONENT_ID(id) (id < 0)

constexpr EntityID NULL_ENTITY_ID = (MAX_ENTITIES-1);
constexpr EntityVersion NULL_ENTITY_VERSION = 0;
constexpr EntityVersion WILDCARD_ENTITY_VERSION = UINT32_MAX;

// template<class... Components>
// constexpr ComponentFlags componentSignature() {
//     constexpr ComponentID ids[] = {getID<Components>() ...};
//     // sum component signatures
//     ComponentFlags result(0);
//     for (size_t i = 0; i < sizeof...(Components); i++) {
//         if (!ECS_BAD_COMPONENT_ID(ids[i]))
//             result.set((Uint32)ids[i]);
//     }
//     return result;
// }

// template<class... Components>
// constexpr ComponentFlags componentMutSignature() {
//     constexpr ComponentID   ids[] = {getID<Components>() ...};
//     constexpr bool         muts[] = {!std::is_const<Components>() ...};

//     ComponentFlags result(0);
//     for (size_t i = 0; i < sizeof...(Components); i++) {
//         if (muts[i])
//             result.set(ids[i]);
//     }
//     return result;
// }

// template<class... Components>
// constexpr std::array<ComponentID, sizeof...(Components)> getComponentIDs() {
//     return {getID<Components>() ...};
// }

// template<class T, class... Components>
// constexpr inline bool componentInComponents() {
//     constexpr ComponentFlags signature = componentSignature<Components...>();
//     //constexpr bool result = signature.getComponent<T>();
//     constexpr bool result = signature[getID<T>()];
//     return result;
// }

template<class... Components>
struct EntityType;

struct Entity {
    EntityID id;
    EntityVersion version;

    Entity() {};
    
    constexpr Entity(EntityID ID, EntityVersion Version)
    : id(ID), version(Version) {}

    constexpr bool operator==(Entity rhs) const {
        return (id == rhs.id && version == rhs.version);
    }

    constexpr bool operator!=(Entity rhs) const {
        return !operator==(rhs);
    }

    inline bool Null() const {
        return id >= NULL_ENTITY_ID || version == NULL_ENTITY_VERSION;
    }

    inline bool NotNull() const {
        return !Null();
    }

    /*
    * Get a string listing the entity's properties, useful for debug logs.
    * Warning: This is a slow method and should not be used frequently
    */
    const char* DebugStr() const {
        static thread_local char buffer[128];
        if (id != NULL_ENTITY_ID) {
            if (version != NULL_ENTITY_VERSION) {
                snprintf(buffer, 128, "ID: %u | Version: %u", id, version);
            } else {
                snprintf(buffer, 128, "ID: %u | Version: null", id);
            }
        } else {
            if (version != NULL_ENTITY_VERSION) {
                snprintf(buffer, 128, "ID: null | Version: %u", version);
            } else {
                return "ID: null | Version: null";
            }
        }
        static_assert(std::is_unsigned<EntityID>::value == true, "change snprintf format specifier!");
        static_assert(std::is_unsigned<EntityVersion>::value == true, "change snprintf format specifier!");
        return buffer;
    }
};

constexpr Entity NullEntity = Entity(NULL_ENTITY_ID, NULL_ENTITY_VERSION);

}

#endif