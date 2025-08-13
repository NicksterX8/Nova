#ifndef ENTITY_INCLUDED
#define ENTITY_INCLUDED

#include <stdint.h>
#include <string.h>
#include <array>
#include "constants.hpp"

typedef uint32_t Uint32;

namespace ECS {

typedef Uint32 EntityID;
typedef Uint32 EntityVersion;
typedef Sint16 ComponentID;
constexpr ComponentID NullComponentID = -1;

#define MAX_ENTITIES 64000
#define ECS_BAD_COMPONENT_ID(id) (id < 0)

constexpr EntityID NULL_ENTITY_ID = (MAX_ENTITIES-1);
constexpr EntityVersion NULL_ENTITY_VERSION = 0;
constexpr EntityVersion WILDCARD_ENTITY_VERSION = UINT32_MAX;

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

using ECS::Entity;
using ECS::NullEntity;

#endif