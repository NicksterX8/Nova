#include "memory/memory.hpp"
#include <stdio.h>
#include <unistd.h>
#include "constants.hpp"
#include "memory/GlobalAllocators.hpp"

struct TrackedAllocData {
    AllocatorI* allocator;
    void* pointer;
    size_t bytes;
    const char* file;
    int line;
};

// live allocations
std::unordered_map<void*, TrackedAllocData> trackedAllocs;

static std::mutex trackedAllocsMutex;

void trackAlloc(AllocatorI* allocator, void* pointer, size_t bytes, const char* file, int line) {
    std::lock_guard lock{trackedAllocsMutex};
    trackedAllocs.insert({
        pointer, 
        {allocator, pointer, bytes, file, line}
    });
}

void logAllocError(const TrackedAllocData& dealloc, const TrackedAllocData& alloc) {
    LogError("%s:%d - Error deallocating pointer %p of size %zu. Allocated at: ", dealloc.file, dealloc.line, dealloc.pointer, dealloc.bytes);
    LogError("%s:%d", alloc.file, alloc.line);
}

void trackDealloc(AllocatorI* allocator, void* pointer, size_t bytes, const char* file, int line) {
    auto alloc = TrackedAllocData{
        allocator, pointer, bytes, file, line
    };
    std::lock_guard lock{trackedAllocsMutex};
    auto trackedAllocIt = trackedAllocs.find(pointer);
    if (UNLIKELY(trackedAllocIt == trackedAllocs.end())) {
        LogError("%s:%d - Deallocation without matching allocation! Ptr: %p. Size: %zu. Check for double delete or mismatching regular new with DELETE", file, line, pointer, bytes);
    } else {
        auto& trackedAlloc = trackedAllocIt->second;
        if (UNLIKELY(trackedAlloc.allocator != alloc.allocator)) {
            FullVirtualAllocator* allocAllocator = findTrackedAllocator(trackedAlloc.allocator);
            FullVirtualAllocator* deallocAllocator = findTrackedAllocator(alloc.allocator);
            // if neither of them are tracked we can't know 
            if (allocAllocator || deallocAllocator)  {
                logAllocError(alloc, trackedAlloc);
                // used different allocator to allocate and deallocate
                const char* allocAllocatorName = allocAllocator ? allocAllocator->getName() : "Default Allocator";
                const char* deallocAllocatorName = deallocAllocator ? deallocAllocator->getName() : "Default Allocator";
                LogError("Allocated pointer with allocator %s and deallocated pointer with allocator %s",
                    allocAllocatorName, deallocAllocatorName);
            }
        }
        if (UNLIKELY(trackedAlloc.bytes != alloc.bytes)) {
            // deallocated different number of bytes than allocated
            LogError(
                "Allocated pointer (%p) as %zu bytes and deallocated as %zu bytes.\n"
                "Allocated at: %s:%d\n"
                "Deallocated at: %s:%d",
                 alloc.pointer, trackedAlloc.bytes, alloc.bytes,
                 trackedAlloc.file, trackedAlloc.line,
                 alloc.file, alloc.line
            );
        }

        // remove it from live allocs
        trackedAllocs.erase(trackedAllocIt);
    }
}

void debugAllocated(void* pointer, size_t bytes, const char* file, int line, AllocatorI* allocator) {
    trackAlloc(allocator, pointer, bytes, file, line);
}

void debugDeallocated(void* pointer, size_t bytes, const char* file, int line, AllocatorI* allocator) {
    trackDealloc(allocator, pointer, bytes, file, line);
}