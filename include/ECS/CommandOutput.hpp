#pragma once

#include "llvm/PointerUnion.h"
#include "ECS/EntityManager.hpp"
#include "ECS/CommandBuffer.hpp"

namespace ECS {

struct EntityCommandOutput : private llvm::PointerUnion<EntityManager*, EntityCommandBuffer*> {
    using Base = llvm::PointerUnion<EntityManager*, EntityCommandBuffer*>;
    EntityCommandOutput(EntityManager* ecs) : Base(ecs) {}

    EntityCommandOutput(EntityCommandBuffer* buffer) : Base(buffer) {}

    bool isCommandBuffer() const {
        return is<EntityCommandBuffer*>();
    }

    bool isEntityManager() const {
        return is<EntityManager*>();
    }

    Entity createEntity(PrototypeID prototype) {
        if (isEntityManager()) {
            return get<EntityManager*>()->createEntity(prototype);
        } else {
            return get<EntityCommandBuffer*>()->createEntity(prototype);
        }
    }

    template<class C>
    void addComponent(Entity entity, const C& component) {
        if (isEntityManager()) {
            get<EntityManager*>()->addComponent<C>(entity, component);
        } else {
            Base::get<EntityCommandBuffer*>()->addComponent(entity, component);
        }
    }

    void addSignature(Entity entity, Signature signature, ArrayRef<char> buffer, ArrayRef<EntityCommandBuffer::VoidComponentValue> components) {
        if (isEntityManager()) {
            auto* ecs = get<EntityManager*>();
            ecs->addSignature(entity, signature);
            for (auto& component : components) {
                void* value = ecs->getComponent(entity, component.id);
                assert(value);
                memcpy(value, buffer.data() + component.bufferIndex, component.size);
            }
        } else {
            auto* ecs = get<EntityCommandBuffer*>();
            ecs->addSignature(entity, signature, buffer, components);
        }
    }
};

}