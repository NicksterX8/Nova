#include "ECS/components/components.hpp"

namespace EC {

Explosion::Explosion(float radius, float damage, float life, int particleCount):
radius(radius), damage(damage), life(life), particleCount(particleCount) {
    life = 0;
}

void Nametag::setName(const char* name) {
    assert(strlen(name) < MAX_ENTITY_NAME_LENGTH && "entity name too long");
    strcpy(this->name, name);
}

void Nametag::setType(const char* type) {
    assert(strlen(type) < MAX_ENTITY_NAME_LENGTH && "entity type name too long");
    strcpy(this->type, type);
}

Nametag::Nametag() {
    name[0] = '\0';
    type[0] = '\0';
}

Nametag::Nametag(const char* type, const char* name) {
    setType(type);
    setName(name);
}

}