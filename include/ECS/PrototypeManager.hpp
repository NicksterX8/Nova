#ifndef ECS_PROTOTYPE_MANAGER_INCLUDED
#define ECS_PROTOTYPE_MANAGER_INCLUDED

#include <vector>
#include "My/BucketArray.hpp"
#include "Prototype.hpp"
#include "ComponentInfo.hpp"

namespace ECS {

struct PrototypeManager {
    ComponentInfoRef componentInfo; // default constructed to null
    std::vector<Prototype> prototypes;
    My::BucketArray<char, 1024> componentAllocator = My::BucketArray<char, 1024>::Empty();

    PrototypeManager() = default;

    PrototypeManager(ComponentInfoRef componentInfo, int numPrototypes) : componentInfo(componentInfo) {
        prototypes.resize(numPrototypes, Prototype());
    }

    Prototype New(PrototypeID id) {
        auto prototype = Prototype(AllocListRef(), componentInfo, id, {0});
        return prototype;
    }

    const Prototype* get(PrototypeID id) const {
        if (id >= prototypes.size() || id < 0) return nullptr;
        return &prototypes[id];
    }

    int numPrototypes() const {
        return prototypes.size();
    }

    // add prototype
    void add(const Prototype& prototype) {
        auto id = prototype.id;
        prototypes[id] = prototype;
    }

    AllocListRef allocateComponents(Signature signature) {
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
        prototypes = std::vector<Prototype>(); // deallocate vector explicitly, in case the destructor is never called
        componentAllocator.destroy();
    }
};

}

#endif