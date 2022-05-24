#include "Components.hpp"

Uint32 componentCount = 0;

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

}

ExplosionComponent::ExplosionComponent(float radius, float damage, float life):
radius(radius), damage(damage), life(life) {
    life = 0;
}

ExplosionComponent grenadeExplosion = ExplosionComponent(4, 35, 30);

ExplosiveComponent::ExplosiveComponent(ExplosionComponent* explosion):
explosion(explosion) {

}

MotionComponent::MotionComponent(Vec2 target, float speed)
: target(target), speed(speed) {

}
