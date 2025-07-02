#include "world/components/components.hpp"

namespace World {

namespace EC {

Explosion::Explosion(float radius, float damage, float life, int particleCount):
radius(radius), damage(damage), life(life), particleCount(particleCount) {
    life = 0;
}

void Nametag::setName(const char* name) {
    assert(strlen(name) < MAX_ENTITY_NAMETAG_LENGTH && "entity name too long");
    strcpy(this->name, name);
}


}

}