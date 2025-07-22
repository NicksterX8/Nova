#ifndef WORLD_ENTITIES_PROTOTYPES_DEF_INCLUDED
#define WORLD_ENTITIES_PROTOTYPES_DEF_INCLUDED

#include "ECS/PrototypeManager.hpp"

namespace World {
    using PrototypeID = ECS::PrototypeID;
    namespace Entities {
        namespace PrototypeIDs {
            enum IDs {
                Default,
                SandParticle,
                Grenade,
                Airstrike,
                Player,
                TextBox,
                ItemStack,
                Tree,
                TransportBelt,
                Monster,
                Cannon,
                Laser,
                Count
            };
        }
    }

    struct Prototype : ECS::Prototype {
        Prototype(const ECS::Prototype& base) : ECS::Prototype(base) {}
    };

    using PrototypeManager = ECS::PrototypeManager;

    template<int TemplateId>
    struct PrototypeDecl : Prototype {
        static constexpr PrototypeID ID = TemplateId;

        PrototypeDecl(PrototypeManager& manager) : Prototype(manager.New(ID)) {}
    };
}

#endif