#ifndef ENTITY_TYPE_INCLUDED
#define ENTITY_TYPE_INCLUDED

#include "Entity.hpp"
#include <assert.h>
#include <type_traits>

namespace ECS {

/*
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

    template<class... C>
    constexpr SpecialEntity(_EntityType<C...> entity) : _EntityType<Components...>(entity) {}

    constexpr SpecialEntity(Entity entity) : _EntityType<Components...>(entity.id, entity.version) {}

    template<class C, class... Vs>
    SpecialEntity(C* creator, Vs... args) : _EntityType<Components...>(creator->New(args...).template cast<Components...>()) {
        
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
};

template<>
constexpr SpecialEntity<>::SpecialEntity(Entity entity) : _EntityType<>(entity.id, entity.version) {}
*/

template<class T>
struct OptionalEntityT : public T {
    using T::T;

    OptionalEntityT() : T(NullEntity.castType<T>()) {}

    OptionalEntityT(T entity) : T(entity) {}
};

template<class... Components>
using OptionalEntity = EntityType<Components...>;

}

#endif