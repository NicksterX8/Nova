#ifndef ITEMS_COMPONENTMANAGER_INCLUDED
#define ITEMS_COMPONENTMANAGER_INCLUDED

#include "Item.hpp"

namespace items {

struct ComponentManager : ItemECS::ArchetypalComponentManager {
    ComponentManager() {}
};

template<class C>
void addComponent(ItemStack& stack, ComponentManager& manager, const C& startValue) {
    manager.addComponent(stack.componentsLoc, C::ID);
    C* component = (C*)manager.getComponent(stack.componentsLoc, C::ID);
    *component = startValue;
    stack.signature.set(C::ID);
}

template<class C>
void addComponent(ItemStack& stack, ComponentManager& manager) {
    manager.addComponent(stack.componentsLoc, C::ID);
    stack.signature.set(C::ID);
}

template<class C>
C* getRegularComponent(const Item& item, const ComponentManager& manager) {
    return manager.getComponent(item.componentsLoc, C::ID);
}

} // namespace items

#endif