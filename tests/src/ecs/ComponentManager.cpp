#include <gtest/gtest.h>

#include "ECS/ArchetypalComponentManager.hpp"
#include "world/components/components.hpp"

using namespace ECS;

struct ComponentManagerTest : testing::Test {
    ArchetypalComponentManager manager;
    ComponentManagerTest() {
        manager.init(ArrayRef{World::EC::ComponentInfoList.data(), World::EC::ComponentInfoList.size()});
    }
};

TEST_F(ComponentManagerTest, Clone) {
    Entity entity = this->manager.createEntity(-1);
    World::EC::Position pos{{1.0f, 2.0f}};
    this->manager.addComponent(entity, World::EC::Position::ID, &pos);

    Entity* clones = (Entity*)malloc(2000 * sizeof(Entity));

    ECS::EntityCreationError error;

    ASSERT_TRUE(this->manager.clone(entity, 1000, clones, &error));
    this->manager.deleteEntities({clones, 1000});

    for (int j = 0; j < 10; j++) {
        this->manager.clone(entity, 1, clones + j, &error);
        this->manager.deleteEntity(*(clones + j));
    }

    this->manager.clone(entity, 1000, clones, &error);

    Entity testE[] = {clones[0], clones[5], clones[1000-1], clones[500]};
    for (int i = 0; i < sizeof(testE) / sizeof(testE[0]); i++) {
        auto* component = (World::EC::Position*)this->manager.getComponent(testE[i], World::EC::Position::ID);
        ASSERT_NE(component, nullptr);
        EXPECT_EQ(component->vec2(), pos.vec2());
    }

    this->manager.deleteEntities({clones, 1000});

    Entity* clones2 = (Entity*)malloc(5000 * sizeof(Entity));
    this->manager.clone(entity, 5000, clones2, &error);

    

    free(clones);
    free(clones2);
}