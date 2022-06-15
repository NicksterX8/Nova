#ifndef ENTITY_TYPE_INCLUDED
#define ENTITY_TYPE_INCLUDED

#include "Entity.hpp"
#include "ECS.hpp"

namespace SpecialEntityStatic {
    extern ECS* ecs; // PRIVATE, DO NOT USE OUT OF SpecialEntity OR SpecialEntityStatic
    void Init(ECS* _ecs);
}

template<class... Components>
struct SpecialEntity : public _EntityType<Components...> {
private:
    using Self = SpecialEntity<Components...>;
public:

    SpecialEntity() {}

    template<class... C>
    SpecialEntity(_EntityType<C...> entity) {
        this->id = entity.id;
        this->version = entity.version;
    }

    template<class T>
    inline int Add() {
        return SpecialEntityStatic::ecs->Add<T>(*this);
    }

    template<class T>
    inline int Add(T startValue) {
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

    operator Self&() const {
        return *(static_cast<_EntityType<Components...>*>(this));
    }

    Entity& toEntity() const {
        return *((Entity*)(this));
    }
};

template<class... Components>
struct SafeEntity : public SpecialEntity<Components...> {
private:
    using Self = SafeEntity<Components...>;
public:

    SafeEntity() {}

    template<class... C>
    SafeEntity(_EntityType<C...> entity) {
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

    operator Self&() {
        return *(static_cast<_EntityType<Components...>*>(this));
    }
};

template<class... Components>
using EntityType = SpecialEntity<Components...>;

#endif