#include <gtest/gtest.h>

#include "ECS/ArchetypePool.hpp"

using namespace ECS;

struct ArchetypePoolTest : testing::Test {
    ArchetypePool pool;
    Mallocator mallocator{};
    ArchetypePoolTest() : pool({0}, &mallocator) {

    }
};

TEST_F(ArchetypePoolTest, Clone) {

}