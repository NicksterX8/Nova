#ifndef ENTITY_PROTOTYPE_INCLUDED
#define ENTITY_PROTOTYPE_INCLUDED

#include "Entity.hpp"

#include <stdint.h>

using Uint32 = uint32_t;

namespace EntityPrototype {

    using TypeID = Uint32;
    constexpr TypeID NullTypeID = 0;

    struct Ref {
        TypeID id;

        Ref() : id(NullTypeID) {}

        Ref(TypeID id) {

        }

        Entity create() {
            return NullEntity;
        }

        const char* getName() {
            return NULL;
        }
    };
}

#define NEW_ENTITY(name) {}

#endif