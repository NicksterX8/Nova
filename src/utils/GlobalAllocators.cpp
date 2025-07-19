#include "memory/GlobalAllocators.hpp"

GlobalAllocatorsType GlobalAllocators = {};

FullVirtualAllocator* findTrackedAllocator(AllocatorI* allocatorPtr) {
    for (auto aallocator : GlobalAllocators.allocators) {
        if (aallocator->getPointer() == allocatorPtr) {
            return aallocator;
        }
    }
    // none with that pointer are tracked
    return nullptr;
}