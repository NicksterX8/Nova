#ifndef UTILS_ALLOCATOR_TYPES_INCLUDED
#define UTILS_ALLOCATOR_TYPES_INCLUDED

#include "BlockAllocator.hpp"
#include "ScratchAllocator.hpp"
#include "ArenaAllocator.hpp"
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
    LargeArenaAllocator frame; // all memory allocated will be freed at the end of the current frame
    FullVirtualAllocator* virtualGameScratchAllocator;

    // pointers must be stable
    std::vector<FullVirtualAllocator*> allocators;

    GlobalAllocatorsType() {
        virtualGameBlockAllocator = trackAllocator("Game block allocator", &gameBlockAllocator);
        virtualGameScratchAllocator = trackAllocator("Game scratch allocator", &gameScratchAllocator);
        trackAllocator("Main frame allocator", &frame);
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