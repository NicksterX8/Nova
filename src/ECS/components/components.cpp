#include "ECS/components/components.hpp"

namespace EC {

Size::Size(float width, float height)
: width(width), height(height) {}
Size::Size(Vec2 vec) : width(vec.x), height(vec.y) {}

Render::Render(TextureID texture, int layer) {
    this->layer = layer;
    this->texture = texture;
}

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

Motion::Motion(Vec2 target, float speed)
: target(target), speed(speed) {

}

AngleMotion::AngleMotion(float rotationTarget, float rotationSpeed)
: rotationTarget(rotationTarget), rotationSpeed(rotationSpeed) {

}

Rotation::Rotation(float degrees) : degrees(degrees) {

}

}