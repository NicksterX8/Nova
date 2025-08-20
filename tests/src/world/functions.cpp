#include <gtest/gtest.h>
#include "world/functions.hpp"
#include "world/EntityWorld.hpp"
#include "world/components/components.hpp"

using namespace World;

struct EntityWorldTest : testing::Test {
    EntityWorld ecs;
    Entity aEntity;

    EntityWorldTest() {
        ecs.init();
        aEntity = ecs.createEntity(-1);
    }

    void initEntity(Vec2 position, Vec2 size) {
        ecs.Add<EC::Position>(aEntity, position);
        ecs.Add<EC::ViewBox>(aEntity, EC::ViewBox::BottomLeft(size));
    }
};

TEST_F(EntityWorldTest, PointInEntity) {
    this->initEntity({0, 0}, {2, 2});

    EXPECT_TRUE(pointInEntity({1, 1}, this->aEntity, this->ecs));
    EXPECT_FALSE(pointInEntity({0, 1}, this->aEntity, this->ecs));
    EXPECT_FALSE(pointInEntity({2, 2}, this->aEntity, this->ecs));

    EXPECT_FALSE(pointInEntity({0, 0}, ECS::NullEntity, this->ecs));
}

TEST_F(EntityWorldTest, GetEntityViewBoxBounds) {
    Entity e = this->ecs.createEntity(-1);
    this->ecs.Add<EC::Position>(e, {10, 10});
    this->ecs.Add<EC::ViewBox>(e, {{1, 1}, {3, 3}});

    Box box = getEntityViewBoxBounds(&this->ecs, e);
    Vec2 min = {11, 11};
    Vec2 size = {3, 3};
    EXPECT_EQ(box.min, min);
    EXPECT_EQ(box.size, size);


}