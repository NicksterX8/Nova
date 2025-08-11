#include "memory/memory.hpp"
#include <stdio.h>
#include <unistd.h>
#include "constants.hpp"
#include "memory/GlobalAllocators.hpp"

namespace Mem {

// dont accidentally use macros in implementation
#undef malloc
#undef free
#undef realloc

/* wrappers */

void* _malloc(size_t size) {
    return malloc(size);
}

void _free(void* mem) {
    free(mem);
}

void* _realloc(void* ptr, size_t size) {
    return realloc(ptr, size);
}


/* debug */

void* debug_malloc(size_t size, const char* file, int line) {
    printf("Allocated %lu bytes at %s:%d\n", size, file, line);
    return malloc(size);
}

void debug_free(void* ptr, const char* file, int line) noexcept {
    printf("Freed %p at %s:%d\n", ptr, file, line);
    free(ptr);
}

}

#define TRACK_ALL_ALLOCS MEMORY_DEBUG_LEVEL >= 2

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
    LogDebug("%s:%d - Error deallocating pointer %p of size %zu. Allocated at: ", dealloc.file, dealloc.line, dealloc.pointer, dealloc.bytes);
    LogDebug("%s:%d", alloc.file, alloc.line);
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
            logAllocError(alloc, trackedAlloc);
            // deallocated different number of bytes than allocated
            LogError("Allocated pointer of %zu bytes and deallocated as %zu bytes", trackedAlloc.bytes, alloc.bytes);
        }

        // remove it from live allocs
        trackedAllocs.erase(trackedAllocIt);
    }
}

void debugNewed(void* pointer, size_t bytes, const char* file, int line, AllocatorI* allocator) {
#if TRACK_ALL_ALLOCS
    trackAlloc(allocator, pointer, bytes, file, line);
#endif
}

void debugDeleted(void* pointer, size_t bytes, const char* file, int line, AllocatorI* allocator) {
#if TRACK_ALL_ALLOCS
    trackDealloc(allocator, pointer, bytes, file, line);
#endif
}