#ifndef ENTITY_INCLUDED
#define ENTITY_INCLUDED

#include <stdint.h>
#include <string.h>
#include <array>
#include "Component.hpp"
#include "metadata/ecs/ecs.hpp"
#include "utils/Log.hpp"
#include "constants.hpp"
#include "My/Bitset.hpp"
#include "My/String.hpp"

typedef uint32_t Uint32;

namespace ECS {

typedef Uint32 EntityID;
typedef Uint32 EntityVersion;
typedef Sint32 ComponentID;

using ComponentFlagsBase = My::Bitset<NUM_COMPONENTS>;

struct ComponentFlags : ComponentFlagsBase {
    using ComponentFlagsBase::ComponentFlagsBase;
    
    constexpr ComponentFlags(ComponentFlagsBase bitset) : ComponentFlagsBase(bitset) {
        
    }

    template<class C>
    constexpr bool getComponent() const {
        constexpr ComponentID id = getID<C>();
        return this->operator[](id);
    }

    template<class C>
    constexpr void setComponent(bool val) {
        constexpr ComponentID id = getID<C>();
        set(id, val);
    }
};

#define MAX_ENTITIES 64000
#define ECS_BAD_COMPONENT_ID(id) (id < 0)

constexpr EntityID NULL_ENTITY_ID = (MAX_ENTITIES-1);
constexpr EntityVersion NULL_ENTITY_VERSION = 0;
constexpr EntityVersion WILDCARD_ENTITY_VERSION = UINT32_MAX;

template<class... Components>
constexpr ComponentFlags componentSignature() {
    constexpr ComponentID ids[] = {getID<Components>() ...};
    // sum component signatures
    ComponentFlags result(0);
    for (size_t i = 0; i < sizeof...(Components); i++) {
        if (!ECS_BAD_COMPONENT_ID(ids[i]))
            result.set((Uint32)ids[i]);
    }
    return result;
}

template<class... Components>
constexpr ComponentFlags componentMutSignature() {
    constexpr ComponentID   ids[] = {getID<Components>() ...};
    constexpr bool         muts[] = {!std::is_const<Components>() ...};

    ComponentFlags result(0);
    for (size_t i = 0; i < sizeof...(Components); i++) {
        if (muts[i])
            result.set(ids[i]);
    }
    return result;
}

template<class... Components>
constexpr std::array<ComponentID, sizeof...(Components)> getComponentIDs() {
    return {getID<Components>() ...};
}

template<class T, class... Components>
constexpr inline bool componentInComponents() {
    constexpr ComponentFlags signature = componentSignature<Components...>();
    //constexpr bool result = signature.getComponent<T>();
    constexpr bool result = signature[getID<T>()];
    return result;
}

template<class... Components>
struct EntityType;

struct EntityBase {
    EntityID id;
    EntityVersion version;

    EntityBase() {};
    
    constexpr EntityBase(EntityID ID, EntityVersion Version)
    : id(ID), version(Version) {}

    constexpr bool operator==(EntityBase rhs) const {
        return (id == rhs.id && version == rhs.version);
    }

    constexpr bool operator!=(EntityBase rhs) const {
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
    * Warning: This is a slow method, should only be used for debugging.
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

template<class... Components>
struct EntityType : EntityBase {
    using Self = EntityType<Components...>;

    constexpr EntityType() : EntityBase(NULL_ENTITY_ID, NULL_ENTITY_VERSION) {}
    constexpr EntityType(EntityID ID, EntityVersion Version) : EntityBase(ID, Version) {}

    template<class... C>
    EntityType(EntityType<C...> entity) {
        static_assert((componentSignature<C...>().hasAll(componentSignature<Components...>()) || (sizeof...(C) == 0)), "need correct components to cast");

        id = entity.id;
        version = entity.version;
    }

    template<class C, class... Vs>
    EntityType(C* creator, Vs... args) {
        *this = creator->New(args...).template cast<Components...>();
    }

    template<class S>
    inline bool Exists(const S* world) const {
        return world->EntityExists(*this);
    }

    template<class T, class S>
    inline T* Get(const S* getter) {
        return getter->template Get<T>(*this);
    }

    template<class T, class S>
    inline const T* Get(const S* getter) const {
        return getter->template Get<T>(*this);
    }

    template<class... Cs, class S>
    bool Has(const S* s) const {
       return s->template EntityHas<Cs...>(*this);
    }

protected:
    template<class T, class S>
    int Add(S* s, const T& startValue) {
        return s->template Add<T>(*this, startValue);
    }
public:

    constexpr static ComponentFlags typeComponentFlags() {
        return componentSignature<Components...>();
    }

    template<class... NewComponents>
    constexpr EntityType<NewComponents...>& cast() const {
        return *((EntityType<NewComponents...>*)(this));
    }

    template<class E>
    constexpr E& castType() const {
        return *((E*)(this));
    }

    constexpr operator Self&() const {
        return cast<>();
    }
};

struct ConstEntity : protected EntityType<> {
    EntityID ID() const {
        return this->id;
    }

    EntityVersion Version() const {
        return this->version;
    }

    EntityType<> CastMut() const {
        return *this;
    }
};

typedef EntityType<> Entity;

constexpr Entity NullEntity = Entity(NULL_ENTITY_ID, NULL_ENTITY_VERSION);

}

#endif