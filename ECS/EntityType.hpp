#ifndef ENTITY_TYPE_INCLUDED
#define ENTITY_TYPE_INCLUDED

#include "Entity.hpp"
#include "ECS.hpp"
#include <assert.h>

namespace SpecialEntityStatic {
    extern ECS* ecs; // PRIVATE, DO NOT USE OUT OF SpecialEntity OR SpecialEntityStatic
    void Init(ECS* _ecs);
}

template<class... Components>
struct SpecialEntity : public _EntityType<Components...> {
    using Self = SpecialEntity<Components...>;
    //using _EntityType<Components...>::_EntityType;
private:
    //SpecialEntity() {}
    //SpecialEntity(EntityBase base) : _EntityType<Components...>(base) {}
public:
    constexpr SpecialEntity() : _EntityType<Components...>(NullEntity.id, NullEntity.version) {}

    constexpr SpecialEntity(EntityID ID, EntityVersion Version) : _EntityType<Components...>(ID, Version) {}

    constexpr SpecialEntity(ECS* ecs) : _EntityType<Components...>(NULL_ENTITY_ID, NULL_ENTITY_VERSION) {
        *this = ecs->New().cast<Components...>();
    }

    template<class... C>
    constexpr SpecialEntity(_EntityType<C...> entity) : _EntityType<Components...>(entity) {}

    constexpr SpecialEntity(Entity entity);

    // SpecialEntity(EntityID id, EntityVersion version) : _EntityType<Components...>(id, version) {}

    template<class T>
    inline int Add() {
        return SpecialEntityStatic::ecs->Add<T>(*this);
    }

    template<class T>
    inline int Add(const T& startValue) {
        return SpecialEntityStatic::ecs->Add<T>(*this, startValue);
    }

    template<class T>
    inline T* Get() const {
        return SpecialEntityStatic::ecs->Get<T>(*this);
    }

    template<class... C>
    bool Has() const {
        constexpr ComponentFlags componentFlags = componentSignature<C...>();
        return (componentFlags & SpecialEntityStatic::ecs->entityComponents(this->id)) == componentFlags;
    }

    bool assertType() const {
        constexpr ComponentFlags typeFlags = componentSignature<Components...>();
        assert((typeFlags & SpecialEntityStatic::ecs->entityComponents(this->id)) == typeFlags && "entity type correct");
        return true;
    }

    /*
    operator Self&() const {
        return *(static_cast<_EntityType<Components...>*>(this));
    }
    */

    Entity& toEntity() const {
        return *((Entity*)(this));
    }
};

template<>
constexpr SpecialEntity<>::SpecialEntity(Entity entity) : _EntityType<>(entity.id, entity.version) {

}

template<class... Components>
struct SafeEntity : public SpecialEntity<Components...> {
private:
    using Self = SafeEntity<Components...>;
public:

    // SafeEntity() {}

    template<class... C>
    SafeEntity(_EntityType<C...> entity) : SpecialEntity<Components...>(entity) {
        this->id = entity.id;
        this->version = entity.version;
    }

    template<class T>
    inline SafeEntity<T, Components...>& Add() {
        static_assert(!componentInComponents<T, Components...>(), "can't add component twice");
        return *static_cast<SafeEntity<T, Components...>*>(&(SpecialEntityStatic::ecs->AddT<T>(*this)));
    }

    template<class T>
    inline SafeEntity<T, Components...>& Add(T startValue) {
        return *static_cast<SafeEntity<T, Components...>*>(&(SpecialEntityStatic::ecs->AddT<T>(*this, startValue)));
    }

    /*
    operator Self&() {
        return *(static_cast<_EntityType<Components...>*>(this));
    }
    */
};

template<class... Components>
using EntityType = SpecialEntity<Components...>;

/*
template<class... Components>
struct OptionalEntity {
protected:
    SpecialEntity<Components...> entity;
    using E = SpecialEntity<Components...>;
public:

    constexpr OptionalEntity() : entity(NULL_ENTITY_ID, NULL_ENTITY_VERSION) {}

    // OptionalEntity(Entity entity) : entity(entity) {}

    constexpr OptionalEntity(E entity) : entity(entity) {}

private:
    template<class... C2, class... C1>
    static SpecialEntity<C2...>& getEntity(OptionalEntity<C1...>& a) {
        return *((SpecialEntity<C2...>*)&a);
    }

    template<class... C2, class... C1>
    static const SpecialEntity<C2...>& getEntity(const OptionalEntity<C1...>& a) {
        return *((SpecialEntity<C2...>*)&a);
    }
public:

    template<class... C>
    constexpr OptionalEntity(OptionalEntity<C...> other) : entity(getEntity(other)) {}

    constexpr OptionalEntity(EntityID id, EntityVersion version) : entity(id, version) {}

    E& Unwrap() {
        assert(entity.IsValid() && "Failed to unwrap invalid entity");
        return entity;
    }

    const E& Unwrap() const {
        assert(entity.IsValid() && "Failed to unwrap invalid entity");
        return entity;
    }

    template<class... C>
    bool Has() const {
        constexpr ComponentFlags componentFlags = componentSignature<C...>();
        return IsValid() && ((componentFlags & SpecialEntityStatic::ecs->entityComponents(entity.id)) == componentFlags);
    }

    bool IsValid() const {
        return entity.IsValid();
    }

    bool Exists() const {
        return (IsValid() && entity.Exists());
    }

    //operator bool() const {
     //   return IsValid();
    //}

    void operator=( const _EntityType<Components...>& entity) {
        memcpy(this, &entity, sizeof(Entity));
    }

    bool operator==(const _Entity& base) const {
        return entity == base;
    }

    bool operator!=(const _Entity& base) const {
        return operator==(base);
    }

    template<class... C>
    bool operator==(const OptionalEntity<C...>& rhs) const {
        return entity == getEntity(rhs);
    }

    template<class... C>
    bool operator!=(const OptionalEntity<C...>& rhs) const {
        return !operator==(rhs);
    }
};
*/

/*
template<class T = SpecialEntity<>>
struct OptionalEntityT {
private:
    T entity;
public:

    constexpr OptionalEntityT() : entity(NULL_ENTITY_ID, NULL_ENTITY_VERSION) {}

    // OptionalEntity(Entity entity) : entity(entity) {}

    constexpr OptionalEntityT(T entity) : entity(entity) {}

    constexpr OptionalEntityT(EntityID id, EntityVersion version) : entity(id, version) {}

    T& Unwrap() {
        assert(entity.IsValid() && "Failed to unwrap invalid entity");
        return entity;
    }

    const T& Unwrap() const {
        assert(entity.IsValid() && "Failed to unwrap invalid entity");
        return entity;
    }

    template<class... C>
    bool Has() const {
        constexpr ComponentFlags componentFlags = componentSignature<C...>();
        return IsValid() && ((componentFlags & SpecialEntityStatic::ecs->entityComponents(entity.id)) == componentFlags);
    }

    bool IsValid() const {
        return entity.IsValid();
    }

    bool Exists() const {
        return (IsValid() && entity.Exists());
    }

    operator bool() const {
        return IsValid();
    }
};
*/


template<class T>
using OptionalEntityT = T;
/*
template<class T>
struct OptionalEntityT : public T {
    using T::T;
};
*/

template<class... Components>
using OptionalEntity = SpecialEntity<Components...>;
/*
struct OptionalEntity : public SpecialEntity<Components...> {
    using SpecialEntity<Components...>::SpecialEntity;
};
*/

#endif