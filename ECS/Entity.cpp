#include "Entity.hpp"
#include "ECS.hpp"

namespace ECS {

bool EntityBase::Null() const {
    return id >= NULL_ENTITY_ID || version == NULL_ENTITY_VERSION;
}

bool EntityBase::NotNull() const {
    return !Null();
}

}