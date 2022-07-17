#ifndef ENTITY_INCLUDED
#define ENTITY_INCLUDED

#include <stdint.h>
#include <string.h>
#include <array>
#include <bitset>
#include <sstream>
#include <string>
#include "Component.hpp"
#include "../ComponentMetadata/macro.hpp"
#include "../ComponentMetadata/getID.hpp"
#include "../Log.hpp"
#include "../constants.hpp"

typedef uint32_t Uint32;

namespace ECS {

typedef Uint32 EntityID;
typedef Uint32 EntityVersion;
typedef Uint32 ComponentID;

template<std::size_t N>
class MyBitset {
public:
    constexpr static size_t nInts = (N/64) + ((N % 64) != 0);
    Uint64 bits[nInts] = {0};
    using Self = MyBitset<N>;

    constexpr MyBitset() {}

    constexpr MyBitset(bool startValue) {
        for (size_t i = 0; i < nInts; i++) {
            if (startValue)
                bits[i] = (Uint64)(-1);
            else
                bits[i] = 0;
        }
    }

    constexpr size_t size() const {
        return N;
    }

    constexpr bool operator[](Uint32 position) const {
        return bits[position/64] & (1 << (position%64));
    }

    constexpr void set(Uint32 position) {
        if (position > size()) return;
        Uint32 position64 = position >> 6;
        Uint64 digit = (1 << (position - (position64 << 6)));
        bits[position64] |= digit;
    }

    constexpr bool any() const {
        for (size_t i = 0; i < nInts; i++) {
            if (bits[i])
                return true;
        }
        return false;
    }

    constexpr size_t count() const {
        size_t c = 0;
        for (size_t i = 0; i < N; i++) {
            if (bits[i/64] & (1 << (i%64))) {
                c++;
            }
        }
        return c;
    }

    constexpr bool hasAll(MyBitset aBitset) const {
        return (*this & aBitset) == aBitset;
    }

    constexpr bool hasNone(MyBitset aBitset) const {
        return !(*this & aBitset).any();
    }

    constexpr Self operator&(Self rhs) const {
        Self result;
        for (size_t i = 0; i < nInts; i++) {
            result.bits[i] = bits[i] & rhs.bits[i];
        }
        return result;
    }

    constexpr Self& operator&=(Self rhs) {
        for (size_t i = 0; i < nInts; i++) {
            bits[i] &= rhs.bits[i];
        }
        return *this;
    }

    constexpr Self operator|(Self rhs) const {
        Self result;
        for (size_t i = 0; i < nInts; i++) {
            result.bits[i] = bits[i] | rhs.bits[i];
        }
        return result;
    }

    constexpr Self& operator|=(Self rhs) {
        for (size_t i = 0; i < nInts; i++) {
            bits[i] |= rhs.bits[i];
        }
        return *this;
    }

    constexpr Self operator^(Self rhs) const {
        Self result;
        for (size_t i = 0; i < nInts; i++) {
            result.bits[i] = bits[i] ^ rhs.bits[i];
        }
        return result;
    }

    constexpr Self& operator^=(Self rhs) {
        for (size_t i = 0; i < nInts; i++) {
            bits[i] ^= rhs.bits[i];
        }
        return *this;
    }

    constexpr bool operator==(Self rhs) const {
        for (size_t i = 0; i < nInts; i++) {
            if (bits[i] != rhs.bits[i]) {
                return false;
            }
        }
        return true;
    }

    constexpr bool operator!=(Self rhs) const {
        return !operator==(rhs);
    }
};
typedef MyBitset<NUM_COMPONENTS> ComponentFlags;

#define MAX_ENTITIES 100000

constexpr EntityID NULL_ENTITY_ID = (MAX_ENTITIES-1);
constexpr EntityVersion NULL_ENTITY_VERSION = 0;
constexpr EntityVersion WILDCARD_ENTITY_VERSION = UINT32_MAX;

template<class... Components>
constexpr ComponentFlags componentSignature() {
    constexpr ComponentID ids[] = {getID<Components>() ...};
    // sum component signatures
    ComponentFlags result;
    for (size_t i = 0; i < sizeof...(Components); i++) {
        result.set(ids[i]);
    }
    return result;
}

template<class... Components>
constexpr ComponentFlags componentMutSignature() {
    constexpr ComponentID   ids[] = {getID<Components>() ...};
    constexpr bool         muts[] = {componentIsMutable<Components>() ...};

    ComponentFlags result;
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
constexpr bool componentInComponents() {
    constexpr ComponentFlags signature = componentSignature<Components...>();
    return signature[getID<T>()];
}

template<class... Components>
struct _EntityType;

template<class EntityTypeT, class... Components>
constexpr bool entityHasComponents() {
    constexpr ComponentFlags componentsSignature = componentSignature<Components...>();
    constexpr ComponentFlags entitySignature = EntityTypeT::typeComponentFlags();
    return entitySignature.hasAll(componentsSignature);
}

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

    bool Null() const;

    bool NotNull() const;

    /*
    * Get a string listing the entity's properties, useful for debug logs.
    * Warning: This is a slow method, should only be used for debugging.
    */
    std::string DebugStr() const {
        std::ostringstream stream;
        if (id == NULL_ENTITY_ID)
            stream << "ID: null";
        else
            stream << "ID: " << id;

        if (version == NULL_ENTITY_VERSION)
            stream << " | Version: null";
        else
            stream << " | Version: " << version;
        return stream.str();
    }
};

template<class... Components>
struct _EntityType : public EntityBase {
    using Self = _EntityType<Components...>;
    // using EntityBase::EntityBase;

    constexpr _EntityType(EntityID ID, EntityVersion Version) : EntityBase(ID, Version) {}

    template<class... C>
    _EntityType(_EntityType<C...> entity) {
        static_assert(componentSignature<C...>().hasAll(componentSignature<Components...>()), "need correct components to cast");

        id = entity.id;
        version = entity.version;
    }

    constexpr static ComponentFlags typeComponentFlags() {
        return componentSignature<Components...>();
    }

    template<class... NewComponents>
    constexpr _EntityType<NewComponents...>& cast() const {
        return *((_EntityType<NewComponents...>*)(this));
    }

    constexpr operator Self&() const {
        return cast<>();
    }
};

struct _Entity : public EntityBase {

    constexpr _Entity() : EntityBase(NULL_ENTITY_ID, NULL_ENTITY_VERSION) {}

    constexpr _Entity(EntityID ID, EntityVersion Version) : EntityBase(ID, Version) {}

    template<class... C>
    constexpr _Entity(_EntityType<C...> entityType) {
        id = entityType.id;
        version = entityType.version;
    }

    template<class... NewComponents>
    constexpr inline _EntityType<NewComponents...>& cast() const {
        return *((_EntityType<NewComponents...>*)(this));
    }

    template<class Type>
    constexpr inline Type& castType() const {
        return *((Type*)(this));
    }

    constexpr operator _EntityType<>() const {
        return cast<>();
    }
};

typedef _Entity Entity;

constexpr Entity NullEntity = Entity(NULL_ENTITY_ID, NULL_ENTITY_VERSION);

}

#endif