
#define SYSTEMS RenderSystem, RenderPositionSystem, RenderSizeSystem, \
    RotationSystem, MotionSystem, ExplosivesSystem, \
    GrowthSystem, DyingSystem, InserterSystem, \
    RotatableSystem, ExplosionSystem, HealthSystem, \
    InventorySystem, FollowSystem, PositionSystem


namespace _SYSTEM_COUNT_ {
    enum SystemCount {
        SYSTEMS,
        NUM_SYSTEMS
    };
}


#define NUM_SYSTEMS _SYSTEM_COUNT_::NUM_SYSTEMS