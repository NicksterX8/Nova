#ifndef ECS_COMMAND_BUFFER_INCLUDED
#define ECS_COMMAND_BUFFER_INCLUDED

#include "Entity.hpp"

namespace ECS {

struct EntityCommandBuffer {
    struct Command {

        struct Add {
            Entity target;
            ComponentID component;
            void* componentValue;
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

    std::vector<Command> commands;
    bool executeInSequence = true;

    template<class C>
    void addComponent(Entity entity, const C& value) {
        // TODO: use better allocation method
        C* componentStorage = new C(value);
        commands.push_back(Command{
            .type = Command::CommandAdd,
            .value.add = {
                .target = entity,
                .component = C::ID,
                .componentValue = componentStorage
            }
        });
    }

    template<class C>
    void removeComponent(Entity entity) {
        commands.push_back(Command{
            .type = Command::CommandRemove,
            .value.remove = {
                .target = entity,
                .component = C::ID
            }
        });
    }

    void deleteEntity(Entity entity) {
        commands.push_back(Command{
            .type = Command::CommandDelete,
            .value.del = {
                .target = entity
            }
        });
    }
};

}

#endif