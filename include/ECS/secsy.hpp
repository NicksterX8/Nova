#include <stdlib.h>
#include <type_traits>
#include "llvm/SmallVector.h"

typedef int ComponentID;

template<typename T>
ComponentID getID() {
    return 0; // temp
}

struct ComponentType {
    int id;
};

struct AnyComponent {
    ComponentID id;
    void* component;

    /*
    AnyComponent() = default;

    AnyComponent(ComponentID id, size_t size) {
        this->component = malloc(size);
        this->id = id;
    }
    */

    void destroy() {
        free(component);
    }
};

template<typename T>
AnyComponent makeAnyComponent() {
    AnyComponent component;
    component.id = getID<T>();
    component.component = malloc(sizeof(T));
    return component;
}

// O(n) where n is the number of components for most operations involving it. Optimized for small numbers of components (0-4 or so)
class Entity {
protected:
    llvm::SmallVector<AnyComponent, 0> components;
    // possibly use some sort of bitset to be able to check if an entity has a component faster.
    // Annoying because ids would need to be in some sort of range so we can index by them
public:
    template<typename T>
    const T* get() const {
        constexpr ComponentID id = /* getID<T>() */ 0;
        for (auto& component : components) {
            if (component.id == id)
                return static_cast<T*>(component.component);
        }
        return nullptr; 
        sizeof(llvm::SmallVector<long long, 0>)
    }

    template<typename T>
    T* get() {
        return const_cast<T*>(static_cast<const Entity*>(this)->get<T>());
    }

    template<typename T>
    bool set(const T& value) {
        auto component = get<T>();
        if (component) {
            *component = value;
            return true;
        }
        return false;
    }
    
    template<typename T>
    int add(const T& value) {
        // hope the component isn't getting added twice I guess.
        // It would take a long time to check if the component already exists by iterating though the vector
        components.push_back(makeAnyComponent<T>());
        components.back() = value;
        return 0; // it could probably just return void cause no way of checking for failure
    }

    template<typename T>
    int remove() {
        constexpr ComponentID id = getID<T>();
        for (auto& component : components) {
            if (component.id == id) {
                components.erase(&component); // idk if this will work
                return 0;
            }
        }
        // couldn't find component in list
        return -1;
    }

    template<typename T>
    bool has() const {
        for (auto& component : components) {
            if (component.id == id)
                return true;
        }
        return false;
    }

    void destroy() {
        for (auto& component : components) {
            component.destroy();
        }
    }
};

class LargeEntity {
    // use a map instead of a vector to make component accesses faster

};

template<typename... Ts>
constexpr size_t sumSize() {
    size_t sizes[] = {sizeof(Ts) ...};
    size_t size = 0;
    for (int i = 0; i < sizeof...(Ts); i++) {
        size += sizes[i];
    }
    return size;
}

#define FOR_EACH_VAR_TYPE(func_call) int COMBINE(_dummy_for_each_var_type_helper, __LINE__)[] = {0, (func_call, 0) ...}; (void)COMBINE(_dummy_for_each_var_type_helper, __LINE__);

template<typename... Components> 
Entity makeEntity() {
    /*
    // allocate all components at once
    size_t totalSize = sumSize<Components...>();
    void* components = malloc(totalSize);
    */

    // allocate each component one at a time
    constexpr size_t numComponents = (int)sizeof...(Components);
    std::vector<AnyComponent> components;
    components.reserve(numComponents);
    FOR_EACH_VAR_TYPE(
        ([&](size_t componentSize, ComponentID id){
            components.push_back(AnyComponent(id, componentSize));
        })(sizeof(Components), /* getID<Components>() */ 0));

    Entity entity;
    entity.components = std::move(components);
    return entity;
}

// entity constructed with components
template<typename... Components>
struct CustomEntity : Entity {
    CustomEntity() : Entity(makeEntity<Components...>()) {}

    CustomEntity(Components... components) : Entity(makeEntity<Components...>()) {
        FOR_EACH_VAR_TYPE(set<Components>(components)); // prolly does not work
    }
};

/*
Probably not necessary to have a component base class
Actually if they all inherit from a base class we can use that to find bugs sooner,
so get<int> wouldn't work because it does not inherit from Component. 
*/

struct HealthComponent {
    float health;
};

struct HungerComponent {
    float hunger;
    int saturation;
};

struct TestEntity : CustomEntity<HealthComponent, HungerComponent> {
    TestEntity(float hunger) : CustomEntity<HealthComponent, HungerComponent>({100.0f}, {hunger}) {
        
    }
};

void test() {
    TestEntity entity(1.0f);
    auto health = entity.get<HealthComponent>();
    auto fake = entity.get<int>(); // shouldn't work as "int" is not a component of entity
    assert(fake == nullptr);
    health->health += 2;
    entity.destroy();
}