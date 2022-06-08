#ifndef TESTS_INCLUDED
#define TESTS_INCLUDED

#include "GameState.hpp"
#include "Items.hpp"
#include <vector>
#include <string>
#include <iostream>
#include "Entities/Entities.hpp"
#include "Log.hpp"

typedef int(*TestFunction)(void);

int testCounter = 0;

struct Test {
    std::string name;
    TestFunction testFunction;
    int id;

    Test(std::string name, TestFunction testFunction):
    name(name), testFunction(testFunction) {
        id = testCounter++;
    }
};

class TestSuite {
    std::vector<Test> tests;
public:
    void test(std::string name, TestFunction function) {
        Test newTest = Test(name, function);
        tests.push_back(newTest);
    }

    std::vector<Test> runAllTests() {
        std::vector<Test> failedTests;
        for (size_t i = 0; i < tests.size(); i++) {
            LogCategory = LOG_CATEGORY_TEST;
            int result = tests[i].testFunction();
            LogCategory = LOG_CATEGORY_MAIN;
            if (result) {
                Log("Test failed! Name: \"%s\", Code: %d\n", tests[i].name.c_str(), result);
                failedTests.push_back(tests[i]);
            }
        }
        return failedTests;
    }
};

namespace Tests {
    int testWorldDisplayCoordinates() {
        GameViewport viewport;
        viewport.display = {
            20,
            13,
            3502,
            1258
        };
        viewport.world = {
            56,
            -40.2f,
            0,
            20.4f
        }; 
        int fail = 0;
        fail |= (viewport.displayToWorldPosition(0, 0) != (Vec2){viewport.world.x, viewport.world.y});
        fail |= (viewport.displayToWorldPosition(viewport.display.w, 0) != (Vec2){viewport.world.x + viewport.world.w, viewport.world.y});
        return fail;
    }

    int basicItemStacking() {
        int fail = 0;

        Inventory inventory = Inventory(32);
        inventory.items[0] = ItemStack(Items::SpaceFloor, 2);

        Uint32 stackSize = ItemData[Items::SpaceFloor].stackSize;
        ItemStack stack = ItemStack(Items::SpaceFloor, stackSize + 2);
        inventory.addItemStack(stack);

        fail |= (inventory.items[0].quantity != stackSize);
        fail |= (inventory.items[1].quantity != 4);

        return fail;
    }

    int advancedItemStacking() {
        int fail = 0;

        Inventory inventory = Inventory(32);

        int stackSize = ItemData[Items::SpaceFloor].stackSize;
        ItemStack emptyStack = ItemStack(0, stackSize + 2);
        inventory.addItemStack(emptyStack);

        ItemStack stack = ItemStack(Items::SpaceFloor, 3);
        inventory.addItemStack(stack);

        fail |= (inventory.items[0].item != stack.item);

        return fail;
    }

    class ECSTest {
    public:
        ComponentFlags entitySignature(ECS* ecs, Entity entity) {
            return ecs->entityComponentFlags[entity.id];
        }
    } ECStest;

    int ECS_removing_entities() {
        int fail = 0;

        ECS ecs;
        Entity ent1 = ecs.New();
        ecs.Add<PositionComponent, GrowthComponent>(ent1);
        PositionComponent* position = ecs.Get<PositionComponent>(ent1);
        ecs.Remove(ent1);

        fail |= (ecs.Get<PositionComponent>(ent1) != NULL);

        return fail;
    }

    int ECS_componentpool_size_entity_removal() {
        int fail = 0;

        ECS ecs;
        Entity ent1 = ecs.New();
        fail |= (ecs.componentPoolSize<HealthComponent>() != 0);
        ecs.Add<HealthComponent, SizeComponent>(ent1);
        fail |= (ecs.componentPoolSize<HealthComponent>() != 1);
        ecs.Remove(ent1);
        fail |= (ecs.componentPoolSize<HealthComponent>() != 0);

        return fail;
    }

    int ECS_componentpool_size_component_removal() {
        int fail = 0;

        ECS ecs;
        Entity ent1 = ecs.New();
        ecs.Add<HealthComponent, SizeComponent>(ent1);
        fail |= (ecs.componentPoolSize<SizeComponent>() != 1);
        ecs.RemoveComponents<SizeComponent>(ent1);
        fail |= (ecs.componentPoolSize<SizeComponent>() != 0);
        fail |= (ecs.componentPoolSize<HealthComponent>() != 1);

        return fail;
    }

    int ECS_remove_null_entity_components() {
        int fail = 0;

        ECS ecs;
        Entity ent1 = ecs.New();
        ComponentFlags signature = componentSignature<RenderComponent, SizeComponent>();
        ecs.Add(ent1, signature);
        ecs.Remove(ent1);
        ecs.RemoveComponents<SizeComponent>(ent1);
        fail |= (ecs.componentPoolSize<SizeComponent>() != 0);
        fail |= (ecs.componentPoolSize<RenderComponent>() != 0);
        fail |= (ECStest.entitySignature(&ecs, ent1) != 0);

        return fail;
    }

    int ECS_remove_nonexistent_components() {
        int fail = 0;

        ECS ecs;
        Entity ent1 = ecs.New();
        ComponentFlags signature = componentSignature<RenderComponent, SizeComponent>();
        ecs.Add(ent1, signature);
        int code = ecs.RemoveComponents<MotionComponent>(ent1);
        fail |= (code == 0);
        fail |= (ECStest.entitySignature(&ecs, ent1) != signature);
        code = ecs.RemoveComponents<RenderComponent, SizeComponent, ExplosionComponent>(ent1);
        fail |= (code == 0);
        fail |= (ECStest.entitySignature(&ecs, ent1) != 0);

        return fail;
    }

    int ECS_num_live_entities() {
        int fail = 0;

        ECS ecs;
        Entity entities[10];
        for (int i = 0; i < 10; i++) {
            Entity ent1 = ecs.New();
            entities[i] = ent1;
            ComponentFlags signature = componentSignature<RenderComponent, SizeComponent>();
        }

        fail |= (ecs.numLiveEntities() != 10);

        int code = 0;
        for (int i = 0; i < 8; i++) {
            code |= ecs.Remove(entities[i]);
        }
        
        fail |= (ecs.numLiveEntities() != 2);

        fail |= (code != 0);

        return fail;
    }

    int ECS_pool_component_count() {
        int fail = 0;

        ECS ecs;
        Entity entities[10];
        for (int i = 0; i < 10; i++) {
            Entity ent1 = ecs.New();
            entities[i] = ent1;
            ComponentFlags signature = componentSignature<RenderComponent, SizeComponent>();
            ecs.Add(ent1, signature);
        }

        fail |= (ecs.componentPoolSize<RenderComponent>() != 10);

        int code = 0;
        for (int i = 0; i < 8; i++) {
            code |= ecs.RemoveComponents<RenderComponent>(entities[i]);
        }
        
        fail |= (ecs.componentPoolSize<RenderComponent>() != 2);

        fail |= (code != 0);

        return fail;
    }

    int ECS_removing_entities_2() {

        int x;
        int fail = 0;

        ECS ecs;

        int nEnts = 300;

        Entity entities[nEnts];
        for (int i = 0; i < nEnts; i++) {
            Entity ent1 = ecs.New();
            entities[i] = ent1;
            ComponentFlags signature;
            if (i%2) {
                signature = componentSignature<RenderComponent, SizeComponent>();
            } else {
                signature = componentSignature<GrowthComponent, HealthComponent>();
            }
            
            ecs.Add(ent1, signature);
        }

        int code = 0;
        for (int i = 0; i < nEnts-2; i += 2) {
            code |= ecs.Remove(entities[i]);
        }

        for (int i = 0; i < nEnts-2; i += 2) {
            if (ecs.EntityLives(entities[i])) {
                fail |= 1;
            }
            if (!ecs.EntityLives(entities[i+1])) {
                fail |= 1;
            }
        }

        for (int i = 0; i < nEnts-2; i += 2) {
            if (ecs.entityComponents(entities[i].id) != 0) {
                fail |= 1;
            }
            if (ecs.entityComponents(entities[i+1].id) != componentSignature<RenderComponent, SizeComponent>()) {
                fail |= 1;
            }
        }

        Entity entities2[100];
        for (int n = 0; n < 20; n++) {
            Entity entity = ecs.New();
            ecs.Add<NametagComponent, GrowthComponent>(entity);
            entities2[n] = entity;
        }

        for (int i = 0; i < nEnts-2; i += 2) {
            if (!ecs.EntityLives(entities[i+1])) {
                fail |= 1;
            }
        }

        for (int i = 0; i < nEnts-2; i += 2) {
            if (ecs.entityComponents(entities[i+1].id) != componentSignature<RenderComponent, SizeComponent>()) {
                fail |= 1;
            }
        }

        for (int i = 0; i < 20; i++) {
            if (ecs.entityComponents(entities2[i].id) != componentSignature<NametagComponent, GrowthComponent>()) {
                fail |= 1;
            }
        }

        fail |= (code != 0);

        return fail;
    }

    int ECS_component_flags() {
        int fail = 0;

        ECS ecs;
        Entity ent1 = ecs.New();
        ComponentFlags signature = componentSignature<RenderComponent, SizeComponent>();

        fail |= (!(signature[2] && signature[1]));
        fail |= ((signature[0] && signature[3]));

        ComponentFlags signature2 = componentSignature<PositionComponent, RenderComponent>();

        fail |= (signature2[0] != 1);

        if ((signature & signature2) == signature) {
            fail |= 1;
        }

        ComponentFlags signature3 = componentSignature<PositionComponent, RenderComponent, NametagComponent>();

        if (!((signature3 & signature2) == signature2)) {
            fail |= 1;
        }

        return fail;
    }

    bool runAll() {
        Log("Running tests...\n");

        TestSuite suite = TestSuite();

        suite.test("Basic Item Stacking", basicItemStacking);
        suite.test("Advanced Item Stacking", advancedItemStacking);
        suite.test("world units", testWorldDisplayCoordinates);
        suite.test("ECS Getting a component of a removed entity", ECS_removing_entities);
        suite.test("ECS component pools retain accurate sizes after entity removal", ECS_componentpool_size_entity_removal);
        suite.test("ECS component pools retain accurate sizes after component removal", ECS_componentpool_size_component_removal);
        suite.test("ECS remove null entity components", ECS_remove_null_entity_components);
        suite.test("ECS remove nonexistent components", ECS_remove_nonexistent_components);
        suite.test("ECS live entities count is accurate", ECS_num_live_entities);
        suite.test("ECS component pool component count is accurate", ECS_pool_component_count);
        suite.test("ECS removing entities 2", ECS_removing_entities_2);
        suite.test("ECS component flags are correct", ECS_component_flags);
        
        std::vector<Test> failedTests = suite.runAllTests();


        bool success = (failedTests.size() == 0);
        if (success) {
            Log("SUCCESS: All tests succeeded.\n");
        } else {
            LogError("FAILURE: %lu tests failed.\n", failedTests.size());
        }

        return success;
    }
}

#endif