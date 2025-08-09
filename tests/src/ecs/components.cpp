#include <gtest/gtest.h>
#include "ECS/EntityManager.hpp"
#include "ECS/componentMacros.hpp"

using namespace ECS;

namespace ComponentIDs {
    #define COMPONENTS Position, Health
    GEN_IDS(ids, ComponentID, COMPONENTS, Count)
}

BEGIN_COMPONENT(Position)
    int x;
    int y;

    bool operator==(const Position& pos) const {
        return pos.x == this->x && pos.y == this->y;
    }
END_COMPONENT(Position)

BEGIN_COMPONENT(Health)
    float health;
END_COMPONENT(Health)

class EntityManagerTest : public testing::Test {
protected:
    EntityManager manager;
    EntityManagerTest() {
        static constexpr auto info = getComponentInfoList<Position, Health>();
        manager.init(ArrayRef(info), 0);
    }
};

EntityManager* makeEntityManager() {
    EntityManager* m = new EntityManager();
    static constexpr auto info = getComponentInfoList<Position, Health>();
    m->init(ArrayRef(info), 0);
    return m;
}

TEST_F(EntityManagerTest, AddComponent) {
    Entity entity = this->manager.newEntity(-1);
    EXPECT_TRUE(this->manager.entityExists(entity)) << "Failed to create an entity!";

    EXPECT_FALSE(this->manager.entityHas<Position>(entity));

    Position* nullPos = this->manager.getComponent<Position>(entity);
    EXPECT_EQ(nullPos, nullptr);

    Position posValue = {5, 10};
    EXPECT_TRUE(this->manager.addComponent<Position>(entity, posValue));
    Position* pos = this->manager.getComponent<Position>(entity);
    EXPECT_NE(pos, nullptr);

    EXPECT_EQ(*pos, posValue);

    EXPECT_TRUE(this->manager.entityHas<Position>(entity));

    this->manager.removeComponent<Position>(entity);
    EXPECT_FALSE(this->manager.entityHas<Position>(entity));
    EXPECT_FALSE(this->manager.getComponent<Position>(entity));
    this->manager.deleteEntity(entity);
}

TEST_F(EntityManagerTest, UsingDeletedEntity) {
    Entity entity = this->manager.newEntity(-1);
    this->manager.addComponent<Position>(entity, {1, 2});
    this->manager.deleteEntity(entity);
    EXPECT_FALSE(this->manager.getComponent<Position>(entity));
    EXPECT_FALSE(this->manager.entityExists(entity));
}