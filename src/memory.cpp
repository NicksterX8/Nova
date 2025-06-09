#include "memory.hpp"
#include <stdio.h>
#include <execinfo.h>
#include <signal.h>
#include <unistd.h>

namespace Mem {

std::function<void()> needMemoryCallback;
std::function<void()> memoryFailCallback;

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

MemoryBlock failedToAlloc(bool required, AllocCallback callback) {
    printf("FAILED TO ALLOC!\n");
    static bool allocFailed = false;
    if (!allocFailed) {
        needMemoryCallback();
        // guard callback with fail so this can't be recursively called
        allocFailed = true;
        auto block = callback();
        allocFailed = false;
        return block;
    } else if (required) {
        memoryFailCallback(); // exit process
    }
    return {nullptr, 0}; // if not required then return null
}

/* variations */

MemoryBlock min_malloc(size_t min, size_t preferred, bool nonnull) {
    assert(preferred >= min && "preferred can't be less than minimum");
    if (min == 0) return {nullptr, 0};

    void* preferredMem = _malloc(preferred);
    if (preferredMem) return {preferredMem, preferred};
    void* minMem = _malloc(min);
    if (minMem) return {minMem, min};

    return failedToAlloc(nonnull, [=](){
        return min_malloc(min, preferred, nonnull);
    });
}

MemoryBlock min_aligned_malloc(size_t min, size_t preferred, size_t alignment, bool nonnull) {
    assert(preferred >= min && "preferred can't be less than minimum");
    if (min == 0) return {nullptr, 0};

    void* preferredMem = aligned_malloc(preferred, alignment, false);
    if (preferredMem) return {preferredMem, preferred};
    void* minMem = aligned_malloc(min, alignment, false);
    if (minMem) return {minMem, min};

    return failedToAlloc(nonnull, [=](){
        return min_aligned_malloc(min, preferred, alignment, nonnull);
    });
}
/*
MemoryBlock min_realloc(void* ptr, size_t min, size_t preferred, bool nonnull) {
    assert(preferred >= min && "preferred can't be less than minimum");
    if (min == 0) return {nullptr, 0};

    void* preferredMem = _realloc(ptr, preferred);
    if (preferredMem) return {preferredMem, preferred};
    void* minMem = _realloc(ptr, min);
    if (minMem) return {minMem, min};

    return failedToAlloc(nonnull, [=](){
        return min_realloc(ptr, min, preferred, nonnull);
    });
}
*/

void handler(int sig) {
  void *array[10];
  size_t size;

  // get void*'s for all entries on the stack
  size = backtrace(array, 10);

  // print out all the frames to stderr
  fprintf(stderr, "Error: signal %d:\n", sig);
  backtrace_symbols_fd(array, size, STDERR_FILENO);
  exit(1);
}


void* safe_malloc(size_t size) {
    void* ptr = _malloc(size);
    if (ptr) return ptr;
    else if (size == 0) return nullptr;
    else return failedToAlloc(true, [=](){ return MemoryBlock{safe_malloc(size), size}; }).ptr;
}

void* safe_realloc(void* oldMem, size_t size) {
    void* newMem = _realloc(oldMem, size);
    if (newMem || !size) return newMem;
    return failedToAlloc(true, [oldMem, size](){
        return MemoryBlock{safe_realloc(oldMem, size), size};
    }).ptr;
}

MemoryBlock aligned_min_realloc(void* mem, size_t min, size_t preferred, size_t alignment, bool nonnull) {
    void* preferredMem = aligned_malloc(preferred, alignment, nonnull);
    if (preferredMem) {
        memcpy(preferredMem, mem, preferred);
        aligned_free(mem);
        return {preferredMem, preferred};
    }
    void* minMem = aligned_malloc(min, alignment, nonnull);
    if (minMem) {
        memcpy(minMem, mem, min);
        aligned_free(mem);
        return {minMem, min};
    }
    return failedToAlloc(nonnull, [=](){
        return aligned_min_realloc(mem, min, preferred, alignment, nonnull);
    });
}

/* debug */

void* debug_malloc(size_t size, const char* file, int line) {
    printf("Allocated %lu bytes at %s:%d\n", size, file, line);
    return safe_malloc(size);
}

void debug_free(void* ptr, const char* file, int line) noexcept {
    printf("Freed %p at %s:%d\n", ptr, file, line);
    _free(ptr);
}

}

void* operator new(size_t size, const char* file, int line) {
    return Mem::debug_malloc(size, file, line);
}

void operator delete(void* ptr, const char* file, int line) noexcept {
    Mem::debug_free(ptr, file, line);
}