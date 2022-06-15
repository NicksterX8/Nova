#include "EntityType.hpp"

ECS* SpecialEntityStatic::ecs = NULL;

void SpecialEntityStatic::Init(ECS* _ecs) {
    SpecialEntityStatic::ecs = _ecs;
}