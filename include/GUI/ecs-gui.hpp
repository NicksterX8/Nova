

#include "ECS/generic/generic stuff.hpp"

namespace GuiEcs {

using ComponentID = GECS::ComponentID;


using GuiElement = PECS::Entity;
using ElementID = GECS::ArchetypeElementAddress;

struct Manager {
    PECS::EntityManager entities; 
};

int test() {
    Manager manager;

}


}