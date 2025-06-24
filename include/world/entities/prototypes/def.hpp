#ifndef WORLD_ENTITIES_PROTOTYPES_DEF_INCLUDED
#define WORLD_ENTITIES_PROTOTYPES_DEF_INCLUDED

namespace World {
    namespace Entities {
        namespace PrototypeIDs {
            enum IDs {
                Default,
                SandParticle,
                Grenade,
                Airstrike,
                Player,
                ItemStack,
                Tree,
                TransportBelt,
                Monster,
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
        static constexpr int ID = TemplateId;

        PrototypeDecl(PrototypeManager& manager) : Prototype(manager.New(ID)) {}
    };
}

#endif