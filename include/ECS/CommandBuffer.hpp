#ifndef ECS_COMMAND_BUFFER_INCLUDED
#define ECS_COMMAND_BUFFER_INCLUDED

#include "Entity.hpp"
#include "My/Vec.hpp"

namespace ECS {

using PrototypeID = Sint32;

struct EntityCommandBuffer {
    struct Command {
        Entity entity;
        struct Create {
            PrototypeID prototype;
        };

        struct Add {
            ComponentID component;
            ssize_t componentValueIndex;
        };

        struct AddSignature {
            Signature signature;
        };

        struct Set {
            ComponentID component;
            ssize_t componentValueIndex;
        };

        struct Remove {
            ComponentID component;
        };

        struct Delete {};

        union {
            Create create;
            Add add;
            AddSignature addSignature;
            Set set;
            Remove remove;
            Delete del;
        };

        enum CommandType {
            CommandCreate,
            CommandAdd,
            CommandAddSignature,
            CommandSet,
            CommandRemove,
            CommandDelete
        } type;
    };

    My::Vec<Command> commands = My::Vec<Command>::Empty();
    My::Vec<char> valueBuffer = My::Vec<char>::Empty();

    EntityID fakeEntityIDCounter = MAX_ENTITIES+1;

    bool empty() const {
        return commands.empty() && valueBuffer.empty();
    }

    void combine(EntityCommandBuffer& added) {
        int indexDiff = this->valueBuffer.size;
        for (auto& command : added.commands) {
            if (command.type == Command::CommandAdd) {
                command.add.componentValueIndex += indexDiff;
            }
        }
        commands.push(added.commands.data, added.commands.size);
        valueBuffer.push(added.valueBuffer.data, added.valueBuffer.size);
        added.destroy();
    }

    Entity newEntity(PrototypeID prototype) {
        EntityID fakeID = fakeEntityIDCounter++;
        commands.push(Command{
            .type = Command::CommandCreate,
            .entity = {fakeID, 0},
            .create = {
                .prototype = prototype
            }
        });
        return {fakeID, 0};
    }

    struct VoidComponentValue {
        ECS::ComponentID id;
        Uint16 bufferIndex;
        Uint16 size;
    };

    void addSignature(Entity entity, Signature signature, ArrayRef<char> buffer, ArrayRef<VoidComponentValue> values) {
        commands.reserve(1 + commands.size + values.size());
        commands.push(Command{
            .type = Command::CommandAddSignature,
            .entity = entity,
            .addSignature = {
                .signature = signature
            }
        });
        for (auto& value : values) {
            commands.push(Command{
                .type = Command::CommandSet,
                .entity = entity,
                .set = {
                    .component = value.id,
                    .componentValueIndex = valueBuffer.size + value.bufferIndex
                }
            });
        }
        
        valueBuffer.push(buffer);
    }

    void addComponent(Entity entity, ComponentID component, const void* value, size_t componentSize) {
        auto componentPos = valueBuffer.size;
        valueBuffer.push((char*)value, componentSize);
        memcpy(&valueBuffer[componentPos], value, componentSize);
        commands.push(Command{
            .type = Command::CommandAdd,
            .entity = entity,
            .add = {
                .component = component,
                .componentValueIndex = componentPos
            }
        });
    }

    template<class C>
    void addComponent(Entity entity, const C& value) {
        addComponent(entity, C::ID, &value, sizeof(C));
    }

    void removeComponent(Entity entity, ComponentID component) {
        commands.push(Command{
            .type = Command::CommandRemove,
            .entity = entity,
            .remove = {
                .component = component
            }
        });
    }

    template<class C>
    void removeComponent(Entity entity) {
        removeComponent(entity, C::ID);
    }

    void deleteEntity(Entity entity) {
        commands.push(Command{
            .type = Command::CommandDelete,
            .entity = entity,
            .del = {}
        });
    }

    void destroy() {
        commands.destroy();
        valueBuffer.destroy();

    }
};

}

#endif