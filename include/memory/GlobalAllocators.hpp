#ifndef UTILS_ALLOCATOR_TYPES_INCLUDED
#define UTILS_ALLOCATOR_TYPES_INCLUDED

#include "BlockAllocator.hpp"
#include "ScratchAllocator.hpp"
#include "llvm/Allocator.h"
#include <vector>

template<typename Allocator>
FullVirtualAllocator* trackAllocator(const char* name, Allocator* allocator);

FullVirtualAllocator* findTrackedAllocator(AllocatorI* allocatorPtr);

using GameBlockAllocator = BlockAllocator<4096, 8>;
// using GameStructureAllocator = ScratchAllocator<GameBlockAllocator*>;

// Allocators for global use ONLY ON MAIN THREAD
struct GlobalAllocatorsType {
    GameBlockAllocator gameBlockAllocator{};
    FullVirtualAllocator* virtualGameBlockAllocator;
    ScratchAllocator<GameBlockAllocator&> gameScratchAllocator{16 * 1024, gameBlockAllocator};
    llvm::BumpPtrAllocatorImpl<> frame;
    FullVirtualAllocator* virtualGameScratchAllocator;

    // pointers must be stable
    std::vector<FullVirtualAllocator*> allocators;

    GlobalAllocatorsType() {
        virtualGameBlockAllocator = trackAllocator("Game block allocator", &gameBlockAllocator);
        virtualGameScratchAllocator = trackAllocator("Game scratch allocator", &gameScratchAllocator);
        trackAllocator("Main frame allocator", &frame);
        //trackAllocator("Global BumpPtr", &bumpPtr);
    }
};

extern GlobalAllocatorsType GlobalAllocators;

using GameStructureAllocator = decltype(GlobalAllocators.gameScratchAllocator);

template<typename Allocator>
FullVirtualAllocator* trackAllocator(const char* name, Allocator* allocator) {
    FullVirtualAllocator* virt = makeFullVirtual(allocator);
    virt->setName(name);
    GlobalAllocators.allocators.push_back(virt);
    return virt;
}

#endif