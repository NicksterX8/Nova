#include <gtest/gtest.h>

#include "ECS/ArchetypePool.hpp"

using namespace ECS;

struct ArchetypePoolTest : testing::Test {
    Mallocator mallocator{};
    ArchetypePoolTest() {

    }
};

TEST_F(ArchetypePoolTest, Clone) {

}