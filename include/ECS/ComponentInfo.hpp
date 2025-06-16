#ifndef ECS_COMPONENT_INFO_INCLUDED
#define ECS_COMPONENT_INFO_INCLUDED

#include <array>
#include "llvm/ArrayRef.h"
#include "utils/ints.hpp"
#include "Entity.hpp"

namespace ECS {

struct ComponentInfo {
    size_t size = 0;
    size_t alignment = 0;
    const char* name = "null";
};

template<class Component>
static constexpr ComponentInfo getComponentInfo() {
    return {sizeof(Component), alignof(Component), Component::NAME};
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

    int numComponents() const {
        return components.size();
    }

    size_t size(ComponentID component) const {
        return components[component].size;
    }

    size_t alignment(ComponentID component) const {
        return components[component].alignment;
    }

    const char* name(ComponentID component) const {
        return components[component].name;
    }
};

template<class C>
constexpr static bool isPrototype() {
    return C::PROTOTYPE;
}

template<class ...RegCs>
static Signature getNonProtoSignature(Signature signature) {
    constexpr Signature allRegularComponents = getSignature<RegCs...>();
    return signature & allRegularComponents;
}

}

#endif