#ifndef UTILS_ALLOCATOR_TYPES_INCLUDED
#define UTILS_ALLOCATOR_TYPES_INCLUDED

#include "utils/allocators.hpp"
#include "llvm/Allocator.h"
#include <vector>

using GameStructureAllocator = ScratchAllocator<BlockAllocator<1024, 4>*>;

template<typename Allocator>
AbstractAllocator* trackAllocator(std::string_view name, Allocator* allocator);

// Allocators for global use ONLY ON MAIN THREAD
struct GlobalAllocatorsType {
    BlockAllocator<4096, 8> gameBlockAllocator{};
    GameStructureAllocator gameScratchAllocator{16 * 1024, &gameBlockAllocator};
    llvm::BumpPtrAllocatorImpl<decltype(gameBlockAllocator)> bumpPtr{&gameBlockAllocator};

    std::vector<AbstractAllocator*> allocators;

    GlobalAllocatorsType() {
        trackAllocator("Game block allocator", &gameBlockAllocator);
        trackAllocator("Game scratch allocator", &gameScratchAllocator);
        trackAllocator("Global BumpPtr", &bumpPtr);
        trackAllocator("mallocator", &mallocator);
    }
};

extern GlobalAllocatorsType GlobalAllocators;

template<typename Allocator>
AbstractAllocator* trackAllocator(std::string_view name, Allocator* allocator) {
    AbstractAllocator* abstract = makeAbstract(allocator);
    abstract->setName(name);
    GlobalAllocators.allocators.push_back(abstract);
    return abstract;
}

#endif