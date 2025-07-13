#ifndef MEMORY_ALLOCATORS_INCLUDED
#define MEMORY_ALLOCATORS_INCLUDED

#include "ArenaAllocator.hpp"
#include "BlockAllocator.hpp"
#include "FreelistAllocator.hpp"
#include "ScratchAllocator.hpp"

// double allocator holder use single value for double allocator for same allocator
static_assert(sizeof(DoubleAllocatorHolder<ScratchAllocator<>, SameAllocator>) == sizeof(ScratchAllocator<>), "Double allocator holder space optimization failed");
// double allocator holder use single reference for double allocator with same allocator
static_assert(sizeof(DoubleAllocatorHolder<ScratchAllocator<>&, SameAllocator>) == sizeof(ScratchAllocator<>*), "Double allocator holder space optimization failed");

constexpr int x = sizeof(ScratchAllocator<>&);
#endif