#ifndef UTILS_ALLOCATOR_TYPES_INCLUDED
#define UTILS_ALLOCATOR_TYPES_INCLUDED

#include "BlockAllocator.hpp"
#include "ScratchAllocator.hpp"
#include "llvm/Allocator.h"
#include <vector>

template<typename Allocator>
VirtualAllocator* trackAllocator(std::string_view name, Allocator* allocator);

using GameBlockAllocator = BlockAllocator<4096, 8>;
// using GameStructureAllocator = ScratchAllocator<GameBlockAllocator*>;

// Allocators for global use ONLY ON MAIN THREAD
struct GlobalAllocatorsType {
    GameBlockAllocator gameBlockAllocator{};
    ScratchAllocator<GameBlockAllocator&> gameScratchAllocator{16 * 1024, gameBlockAllocator};
    llvm::BumpPtrAllocatorImpl<GameBlockAllocator&> bumpPtr{gameBlockAllocator};

    // pointers must be stable
    std::vector<VirtualAllocator*> allocators;

    GlobalAllocatorsType() {
        trackAllocator("Game block allocator", &gameBlockAllocator);
        trackAllocator("Game scratch allocator", &gameScratchAllocator);
        trackAllocator("Global BumpPtr", &bumpPtr);
    }
};

extern GlobalAllocatorsType GlobalAllocators;

using GameStructureAllocator = decltype(GlobalAllocators.gameScratchAllocator);

template<typename Allocator>
VirtualAllocator* trackAllocator(std::string_view name, Allocator* allocator) {
    VirtualAllocator* virt = makeVirtual(allocator);
    virt->setName(name);
    GlobalAllocators.allocators.push_back(virt);
    return virt;
}

VirtualAllocator* findTrackedAllocator(AllocatorI* allocatorPtr);

#endif