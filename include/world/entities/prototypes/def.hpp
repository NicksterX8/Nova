#ifndef WORLD_ENTITIES_PROTOTYPES_DEF_INCLUDED
#define WORLD_ENTITIES_PROTOTYPES_DEF_INCLUDED

#include "ECS/PrototypeManager.hpp"

namespace ECS {

struct EntityMaker {
private:
    // char buffer[1024];
    SmallVector<char, 256> buffer;
public:
    ECS::Signature components = {0};
    ECS::PrototypeID prototype = -1;

    
    SmallVector<Uint16, 12> componentSizes;

    EntityMaker(ECS::PrototypeID prototype) : prototype(prototype) {
    
    }
    
    EntityMaker(const EntityMaker& other) = delete;

    template<class C>
    void Add(const C& value) {
        components.setComponent<C>();
        size_t offset = getAlignmentOffset((size_t)buffer.data(), alignof(C));
        buffer.resize(buffer.size() + sizeof(C) + offset);
        *((C*)(buffer.end() - sizeof(C))) = value;
        componentValues.push_back(ComponentValue{C::ID, (Uint16)(buffer.size() - sizeof(C)), sizeof(C)});
    }

    template<class... Cs>
    void Add(const Cs&... components) {
        FOR_EACH_VAR_TYPE(Add<Cs>(components));
    }

    Entity output(World::EntityCommandOutput out) {
        Entity entity = out.newEntity(prototype);
        out.addSignature(entity, components, {buffer.data(), componentValues});
        // set
        // done
        return entity;
    }

    // template<typename... Args>
    // void buffer(EntityCommandBuffer* commandBuffer, Args... args) {

    // }

    // template<typename... Args>
    // Entity make(EntityWorld* ecs, Args... args) {

    // }
};

}

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
    public:
        // static ECS::EntityMaker& startMake() {
        //     threadEntityMaker.reset();
        //     threadEntityMaker.prototype = ID;
        //     return threadEntityMaker;
        // }
    };
}

#endif