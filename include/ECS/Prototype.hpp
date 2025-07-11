#ifndef ECS_PROTOTYPE_INCLUDED
#define ECS_PROTOTYPE_INCLUDED

#include "ADT/ArrayRef.hpp"
#include "My/SparseSets.hpp"
#include "My/Vec.hpp"
#include "memory/memory.hpp"
#include "Signature.hpp"
#include "ComponentInfo.hpp"

struct AllocListRef {
    void* buffer;
    void** allocations;

    AllocListRef() {
        buffer = nullptr;
        allocations = nullptr;
    }

    AllocListRef(void* buffer, ArrayRef<size_t> offsets) : buffer(buffer) {
        allocations = Alloc<void*>(offsets.size());
        for (int i = 0; i < offsets.size(); i++) {
            allocations[i] = (char*)buffer + offsets[i];
        }
    }

    void destroy() {
        Free(allocations);
    }
};

namespace ECS {

using ComponentPtr = void*;

class LargeComponentList {
    using Set = My::DenseSparseSet<ComponentID, ComponentPtr, Uint16, MaxComponentID>;
    Set components;
public:

    static LargeComponentList New() {
        LargeComponentList self;
        self.components = Set::Empty();
        return self;
    }

    ComponentPtr get(ComponentID component) const {
        void** ptrToActualPtr = (void**)components.lookup(component);
        if (ptrToActualPtr) {
            return *ptrToActualPtr;
        }
        return nullptr;
    }

    // warning: slow and not too efficient
    bool has(ComponentID component) const {
        return this->components.contains(component);
    }

    // storage must be stable, allocated memory to hold the component value
    void add(ComponentID component, void* storage) {
        components.insert(component, storage);
    }

    void remove(ComponentID component) {
        components.remove(component);
    }

    void destroy() {
        components.destroy();
    }
};

using PrototypeID = Sint32;

struct Prototype {
    PrototypeID id = 0;
    Signature signature = 0;
    LargeComponentList components = LargeComponentList::New();

    ComponentInfoRef componentInfo;

    My::Vec<void*> allocations = My::Vec<void*>::Empty();

    /* universal components */
    std::string name;

    /* Constructors */

    Prototype() {}

    Prototype(AllocListRef componentsStorage, ComponentInfoRef componentInfo, PrototypeID id, Signature signature = {})
    : id(id), signature(signature), componentInfo(componentInfo) {
        int i = 0;
        signature.forEachSet([&](auto componentID){
            components.add(componentID, componentsStorage.allocations[i]);
            i++;
        });
        componentsStorage.destroy();
    }

    virtual ~Prototype() {}

    /* Methods */
    /* Virtual */
    // these could be std::functions in the future
    virtual void onDestroy() {}
    virtual void onCreate() {}

    /* Regular */

    void setName(const char* name) {
        this->name = name;
    }

    bool has(ComponentID id) const {
        return signature[id];
    }

    template<class C>
    bool has() const {
        return signature[C::ID];
    }

    void* get(ComponentID component) {
        return components.get(component);
    }

    const void* get(ComponentID component) const {
        return components.get(component);
    }

    template<class C>
    C* get() {
        return (C*)components.get(C::ID);
    }

    template<class C>
    const C* get() const {
        return (C*)components.get(C::ID);
    }

    template<class C>
    void set(const C& value) {
        auto component = get<C>();
        if (component) {
            *component = value;
        } else {
            LogError("Attempted to set unowned prototype component (%s)", componentInfo.get(C::ID).name);
        }
    }

    template<class C>
    void add(const C& value) {
        auto component = get<C>();
        if (component) {
            *component = value;
            LogWarnLoc("Component (%s) already attached to this prototype!", componentInfo.get(C::ID).name);
        } else {
            C* storage = Alloc<C>(1);
            allocations.push(storage);
            memcpy(storage, &value, sizeof(C));
            components.add(C::ID, storage);
            signature.set(C::ID);
        }
    }

    void destroy() {
        components.destroy();

        for (auto ptr : allocations) {
            Free(ptr);
        }
        allocations.destroy();
    }
};

}

#endif