#ifndef WORLD_ENTITIES_PROTOTYPES_DEF_INCLUDED
#define WORLD_ENTITIES_PROTOTYPES_DEF_INCLUDED

#include "ECS/PrototypeManager.hpp"
#include "ECS/CommandOutput.hpp"

namespace ECS {

struct EntityMaker {
private:
    SmallVector<char, 256> buffer;
public:
    ECS::Signature components = {0};
    ECS::PrototypeID prototype = -1;

    SmallVector<EntityCommandBuffer::VoidComponentValue, 8> componentValues;

    EntityMaker(ECS::PrototypeID prototype) : prototype(prototype) {
    
    }
    
    EntityMaker(const EntityMaker& other) = delete;

    template<class C>
    void Add(const C& value) {
        components.setComponent<C>();
        if (std::is_empty_v<C>) return; // don't need a value
        size_t offset = getAlignmentOffset((size_t)buffer.data(), alignof(C));
        buffer.resize(buffer.size() + sizeof(C) + offset);
        *((C*)(buffer.end() - sizeof(C))) = value;
        componentValues.push_back({C::ID, (Uint16)(buffer.size() - sizeof(C)), sizeof(C)});
    }

    template<class... Cs>
    void Add(const Cs&... components) {
        FOR_EACH_VAR_TYPE(Add<Cs>(components));
    }

    Entity output(ECS::EntityCommandOutput out) {
        Entity entity = out.createEntity(prototype);
        out.addSignature(entity, components, buffer, componentValues);
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

    template<PrototypeID TemplateId>
    struct PrototypeDecl : Prototype {
        static constexpr PrototypeID ID = TemplateId;

        PrototypeDecl(PrototypeManager& manager) : Prototype(manager.New(ID)) {}
    public:

    };
}

#endif