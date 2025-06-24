#ifndef ECS_COMMAND_BUFFER_INCLUDED
#define ECS_COMMAND_BUFFER_INCLUDED

#include "Entity.hpp"

namespace ECS {

struct EntityCommandBuffer {
    struct Command {

        struct Add {
            Entity target;
            ComponentID component;
            ssize_t componentValueIndex;
        };

        struct Remove {
            Entity target;
            ComponentID component;
        };

        struct Delete {
            Entity target;
        };

        union {
            Add add;
            Remove remove;
            Delete del;
        } value;

        enum CommandType {
            CommandAdd,
            CommandRemove,
            CommandDelete
        } type;
    };

    My::Vec<Command> commands = My::Vec<Command>::Empty();
    My::Vec<char> valueBuffer = My::Vec<char>::Empty();
    bool executeInSequence = true;

    bool empty() const {
        return commands.empty() && valueBuffer.empty();
    }

    void combine(EntityCommandBuffer& added) {
        int indexDiff = this->valueBuffer.size;
        for (auto& command : added.commands) {
            if (command.type == Command::CommandAdd) {
                command.value.add.componentValueIndex += indexDiff;
            }
        }
        commands.push(added.commands.data, added.commands.size);
        valueBuffer.push(added.valueBuffer.data, added.valueBuffer.size);
        added.destroy();
    }

    void addComponent(Entity entity, ComponentID component, const void* value, size_t componentSize) {
        auto componentPos = valueBuffer.size;
        valueBuffer.push((char*)value, componentSize);
        memcpy(&valueBuffer[componentPos], value, componentSize);
        commands.push(Command{
            .type = Command::CommandAdd,
            .value.add = {
                .target = entity,
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
            .value.remove = {
                .target = entity,
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
            .value.del = {
                .target = entity
            }
        });
    }

    void destroy() {
        commands.destroy();
        valueBuffer.destroy();
    }
};

}

#endif