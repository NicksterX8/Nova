#ifndef ITEMS_COMPONENTMANAGER_INCLUDED
#define ITEMS_COMPONENTMANAGER_INCLUDED

#include "Item.hpp"

namespace items {

struct ComponentManager : ItemECS::ArchetypalComponentManager {
    ComponentManager() {}
};

} // namespace items

#endif