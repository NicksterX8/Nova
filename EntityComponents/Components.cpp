#include "Components.hpp"

PositionComponent::PositionComponent(float x, float y) {
    this->x = x;
    this->y = y;
}
PositionComponent::PositionComponent(Vec2 vec) {
    this->x = vec.x;
    this->y = vec.y;
}

SizeComponent::SizeComponent(float width, float height)
: width(width), height(height) {

}

Vec2 SizeComponent::toVec2() const {
    return Vec2(width, height);
}

RenderComponent::RenderComponent(SDL_Texture* texture, int layer)
: texture(texture), layer(layer) {
    rotation = 0;
}

ExplosionComponent::ExplosionComponent(float radius, float damage, float life, int particleCount):
radius(radius), damage(damage), life(life), particleCount(particleCount) {
    life = 0;
}

ExplosiveComponent::ExplosiveComponent(ExplosionComponent* explosion):
explosion(explosion) {

}

void NametagComponent::setName(const char* name) {
    assert(strlen(name) < MAX_ENTITY_NAME_LENGTH && "entity name too long");
    strcpy(this->name, name);
}

void NametagComponent::setType(const char* type) {
    assert(strlen(type) < MAX_ENTITY_NAME_LENGTH && "entity type name too long");
    strcpy(this->type, type);
}

NametagComponent::NametagComponent() {
    name[0] = '\0';
    type[0] = '\0';
}

NametagComponent::NametagComponent(const char* type, const char* name) {
    setType(type);
    setName(name);
}

MotionComponent::MotionComponent(Vec2 target, float speed)
: target(target), speed(speed) {

}

AngleMotionEC::AngleMotionEC(float rotationTarget, float rotationSpeed)
: rotationTarget(rotationTarget), rotationSpeed(rotationSpeed) {

}

RotationComponent::RotationComponent(float degrees) : degrees(degrees) {

}