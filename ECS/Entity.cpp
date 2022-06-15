#include "Entity.hpp"
#include "ECS.hpp"

const char* componentNames[NUM_COMPONENTS] = {NULL};

static ECS* globalECS;

bool EntityBase::IsAlive() const {
    return globalECS->EntityLives(Entity(this->id, this->version));
}

void setGlobalECS(ECS* _ecs) {
    globalECS = _ecs;
}