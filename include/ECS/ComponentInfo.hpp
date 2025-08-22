#ifndef ECS_COMPONENT_INFO_INCLUDED
#define ECS_COMPONENT_INFO_INCLUDED

#include <array>
#include "ADT/ArrayRef.hpp"
#include "utils/ints.hpp"
#include "Entity.hpp"

namespace ECS {

struct ComponentInfo {
    Sint32 size = 0;
    Sint32 alignment = 0;
    const char* name = "null";
    bool prototype = false;
    bool empty = false; // the component has no members -> std::is_empty
};

template<class Component>
static constexpr ComponentInfo getComponentInfo() {
    return {sizeof(Component), alignof(Component), Component::NAME, Component::PROTOTYPE, std::is_empty_v<Component>};
}

template<class... Components>
static constexpr std::array<ComponentInfo, sizeof...(Components)> getComponentInfoList() {
    return {getComponentInfo<Components>() ...};
}

class ComponentInfoRef {
    ArrayRef<ComponentInfo> components;
public:
    ComponentInfoRef() {}

    ComponentInfoRef(ArrayRef<ComponentInfo> componentInfoList) : components(componentInfoList) {
        
    }

    const ComponentInfo& get(ComponentID component) const {
        return components[component];
    }

    int size() const {
        return components.size();
    }
};

template<class C>
constexpr static bool isPrototype() {
    return C::PROTOTYPE;
}

template<class ...RegCs>
static Signature getNonProtoSignature(Signature signature) {
    static constexpr Signature allRegularComponents = getSignature<RegCs...>();
    return signature & allRegularComponents;
}

}

#endif