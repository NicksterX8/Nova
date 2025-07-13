#ifndef ECS_PROTOTYPE_MANAGER_INCLUDED
#define ECS_PROTOTYPE_MANAGER_INCLUDED

#include <vector>
#include "My/BucketArray.hpp"
#include "Prototype.hpp"
#include "ComponentInfo.hpp"

namespace ECS {

struct PrototypeManager {
    ComponentInfoRef componentInfo; // default constructed to null
    Signature validComponents;
    std::vector<Prototype*> prototypes;
    std::unordered_map<std::string, Prototype*> namedPrototypes;
    My::BucketArray<char, 1024> componentAllocator;

    void init(ComponentInfoRef componentInfo, int numPrototypes) {
        this->componentInfo = componentInfo;
        validComponents = {0};
        for (int i = 0; i < componentInfo.size(); i++) {
            if (componentInfo.get(i).prototype)
                validComponents.set(i);
        }

        prototypes.resize(numPrototypes, nullptr);
    }

    Prototype New(PrototypeID id) {
        auto prototype = Prototype(AllocListRef(), componentInfo, id, {0});
        return prototype;
    }

    const Prototype* get(PrototypeID id) const {
        if (id >= prototypes.size() || id < 0) return nullptr;
        return prototypes[id];
    }

    // Search through all prototypes to get 
    const Prototype* get(const char* name) const {
        for (auto& prototype : prototypes) {
            if (prototype && prototype->name == name) {
                return prototype;
            }
        }
        return nullptr;
    }

    int numPrototypes() const {
        return prototypes.size();
    }

    // add prototype
    void add(Prototype* prototype) {
        auto id = prototype->id;
        prototypes[id] = prototype;
        if (!prototype->name.empty()) {
            if (namedPrototypes.find(prototype->name) == namedPrototypes.end()) {
                namedPrototypes.insert({prototype->name, prototypes[id]});
            } else {
                LogError("A prototype already has the name %s", prototype->name.c_str());
            }
        } else {
            LogWarn("Prototype has no name!");
        }
    }

    AllocListRef allocateComponents(Signature signature) {
        const auto count = signature.count();
        size_t* offsets = (size_t*)alloca(count * sizeof(size_t));
        size_t totalSize = 0;
        int i = 0;
        signature.forEachSet([&](auto componentID){
            const auto& info = componentInfo.get(componentID);
            const auto componentAlignment = info.alignment;
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
            prototype->components.destroy();
        }
        prototypes = std::vector<Prototype*>(); // deallocate vector explicitly, in case the destructor is never called
        componentAllocator.destroy();
    }
};

}

#endif