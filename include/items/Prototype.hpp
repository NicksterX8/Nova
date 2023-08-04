#ifndef ITEM_PROTOTYPE_INCLUDED
#define ITEM_PROTOTYPE_INCLUDED

#include "Item.hpp"
#include "My/Vec.hpp"
#include "rendering/textures.hpp"

struct AllocListRef {
    void* buffer;
    void** allocations;

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

namespace items {

struct ItemPrototype {
    ItemType id = ItemTypes::None;
    ComponentSignature signature = 0;
    ItemECS::LargeComponentList components = ItemECS::LargeComponentList::New();

    ComponentInfoRef componentInfo;

    /* Customizable, universal components */

    ItemQuantity stackSize = 0;

    /* Constructors */

    ItemPrototype() {}

    ItemPrototype(AllocListRef componentsStorage, ComponentInfoRef componentInfo, ItemType id, ComponentSignature signature = 0)
    : id(id), signature(signature), componentInfo(componentInfo) {
        int i = 0;
        signature.forEachSet([&](auto componentID){
            components.add(componentID, componentsStorage.allocations[i]);
            i++;
        });
        componentsStorage.destroy();
    }

    virtual ~ItemPrototype() {}

    /* Methods */
    /* Virtual */
    // these could be std::functions in the future
    virtual void onDestroy() {}
    virtual void onCreate() {}

    /* Regular */

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
            LogError("Attempted to set unowned prototype component (%s)", componentInfo.name(C::ID));
        }
    }
};

struct PrototypeManager {
    ComponentInfoRef componentInfo; // default constructed to null
    std::vector<ItemPrototype> prototypes;
    My::BucketArray<char, 1024> componentAllocator = My::BucketArray<char, 1024>::Empty();

    PrototypeManager() = default;

    PrototypeManager(ComponentInfoRef componentInfo, int numPrototypes) : componentInfo(componentInfo) {
        prototypes.resize(numPrototypes, ItemPrototype());
    }

    const ItemPrototype& get(ItemType type) const {
        return prototypes[type];
    }

    int numPrototypes() const {
        return prototypes.size();
    }

    // add prototype
    void add(const ItemPrototype& prototype) {
        auto id = prototype.id;
        prototypes[id] = prototype;
    }

    AllocListRef allocateComponents(ComponentSignature signature) {
        const auto count = signature.count();
        size_t* offsets = (size_t*)alloca(count * sizeof(size_t));
        size_t totalSize = 0;
        int i = 0;
        signature.forEachSet([&](auto componentID){
            const auto componentSize = componentInfo.size(componentID);
            const auto componentAlignment = componentInfo.alignment(componentID);
            const auto offset = totalSize + totalSize % componentAlignment;
            offsets[i] = offset;
            totalSize += offset;
            i++;
        });
        auto buffer = componentAllocator.reserveBack(totalSize);
        memset(buffer, 0, totalSize); // unnecessary, just for safety
        return AllocListRef(buffer, {offsets, count});
    }

    void destroy() {
        for (auto& prototype : prototypes) {
            prototype.components.destroy();
        }
        prototypes = std::vector<ItemPrototype>(); // deallocate vector explicitly, in case the destructor is never called
        componentAllocator.destroy();
    }
};

template<class... Components>
ItemPrototype New(PrototypeManager* manager, ItemType typeID) {
    constexpr auto signature = getComponentSignature<Components...>();
    auto componentsStorage = manager->allocateComponents(signature);
    return ItemPrototype(componentsStorage, manager->componentInfo, typeID, signature);
}

} // namespace items

using items::ItemPrototype;

#endif