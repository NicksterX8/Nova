#include "Components.hpp"

namespace EC {

Position::Position(float x, float y) {
    this->x = x;
    this->y = y;
}
Position::Position(Vec2 vec) {
    this->x = vec.x;
    this->y = vec.y;
}

Size::Size(float width, float height)
: width(width), height(height) {

}

Vec2 Size::toVec2() const {
    return Vec2(width, height);
}

Render::Render(SDL_Texture* texture, int layer)
: texture(texture), layer(layer) {
    rotation = 0;
    renderIndex = 0;
}

Explosion::Explosion(float radius, float damage, float life, int particleCount):
radius(radius), damage(damage), life(life), particleCount(particleCount) {
    life = 0;
}

Explosive::Explosive(EC::Explosion* explosion):
explosion(explosion) {

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