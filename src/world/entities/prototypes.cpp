#include "world/entities/prototypes/prototypes.hpp"

namespace World {

namespace Entities {

    void makePrototypes(EntityWorld& ecs) {
        auto* allocator = &GlobalAllocators.gameScratchAllocator;
        auto& pm = ecs.prototypes;
        using namespace World;
        std::vector<ItemPrototype*> prototypes;
        #define MAKE_PROTOTYPE(name) auto* name = allocator->New<Entities::name>(pm); prototypes.push_back(name);
        MAKE_PROTOTYPE(Player);
        MAKE_PROTOTYPE(ItemStack);
        MAKE_PROTOTYPE(Monster);
        MAKE_PROTOTYPE(Spider);
        MAKE_PROTOTYPE(Grenade);
        MAKE_PROTOTYPE(Cannon);

        for (int i = 0; i < prototypes.size(); i++) {
            pm.add(prototypes[i]);
        }
    }

}

}