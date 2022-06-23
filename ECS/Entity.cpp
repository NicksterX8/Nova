#include "Entity.hpp"
#include "ECS.hpp"

const char* componentNames[NUM_COMPONENTS] = {NULL};

static ECS* globalECS;

bool EntityBase::Exists() const {
    return globalECS->EntityExists(Entity(this->id, this->version));
}

bool EntityBase::Null() const {
    return id == NULL_ENTITY_ID || version == NULL_ENTITY_VERSION;
}

void setGlobalECS(ECS* _ecs) {
    globalECS = _ecs;
}