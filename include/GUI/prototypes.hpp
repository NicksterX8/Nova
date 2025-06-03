#ifndef GUI_PROTOTYPES_INCLUDED
#define GUI_PROTOTYPES_INCLUDED

#include "ECS/generic/ecs.hpp"

namespace GUI {

using Prototype = GECS::Prototype;

namespace ElementTypes {
    enum {
        Normal,
        Epic,
        Count
    };
}

namespace Prototypes {

    struct Normal : Prototype {
        Normal(GECS::PrototypeManager& manager) : Prototype(manager.New(ElementTypes::Normal)) {

        }
    };

    struct Epic : Prototype {
        Epic(GECS::PrototypeManager& manager) : Prototype(manager.New(ElementTypes::Normal)) {
            
        }
    };

}

}

#endif