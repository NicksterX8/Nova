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

EntityManager* makeEntityManager() {
    EntityManager* m = new EntityManager();
    static constexpr auto info = getComponentInfoList<Position, Health>();
    m->init(ArrayRef(info), 0);
    return m;
}

TEST(EcsComponents, GetComponent) {
    EntityManager& manager = *makeEntityManager();
    
    Entity entity = manager.newEntity(-1);
    EXPECT_TRUE(manager.entityExists(entity)) << "Failed to create an entity!";

    EXPECT_FALSE(manager.entityHas<Position>(entity));

    Position* nullPos = manager.getComponent<Position>(entity);
    EXPECT_EQ(nullPos, nullptr);

    Position posValue = {5, 10};
    EXPECT_TRUE(manager.addComponent<Position>(entity, posValue));
    Position* pos = manager.getComponent<Position>(entity);
    EXPECT_NE(pos, nullptr);

    EXPECT_EQ(*pos, posValue);

    EXPECT_TRUE(manager.entityHas<Position>(entity));
}