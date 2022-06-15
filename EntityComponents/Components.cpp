#include "Components.hpp"

PositionComponent::PositionComponent(float x, float y) {
    this->x = x;
    this->y = y;
}
PositionComponent::PositionComponent(Vec2 vec) {
    this->x = vec.x;
    this->y = vec.y;
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

MotionComponent::MotionComponent(Vec2 target, float speed)
: target(target), speed(speed) {

}

AngleMotionEC::AngleMotionEC(float rotationTarget, float rotationSpeed)
: rotationTarget(rotationTarget), rotationSpeed(rotationSpeed) {

}