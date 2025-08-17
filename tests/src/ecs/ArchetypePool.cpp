#include <gtest/gtest.h>

#include "ECS/ArchetypePool.hpp"

using namespace ECS;

struct ArchetypePoolTest : testing::Test {
    ArchetypePool pool;
    ArchetypePoolTest() : pool(Archetype::Null()) {

    }
};

TEST_F(ArchetypePoolTest, Clone) {

}