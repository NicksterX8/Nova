#ifndef GUI_PROTOTYPES_INCLUDED
#define GUI_PROTOTYPES_INCLUDED

#include "ECS/EntityManager.hpp"

namespace GUI {

using Prototype = ECS::Prototype;

namespace ElementTypes {
    enum {
        Normal,
        Epic,
        Count
    };
}

namespace Prototypes {

    struct Normal : Prototype {
        Normal(ECS::PrototypeManager& manager) : Prototype(manager.New(ElementTypes::Normal)) {
            setName("normal");
        }
    };

    struct Epic : Prototype {
        Epic(ECS::PrototypeManager& manager) : Prototype(manager.New(ElementTypes::Normal)) {
            setName("epic");
        }
    };

}

}

#endif