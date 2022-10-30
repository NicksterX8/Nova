#ifndef MY_ALLOCATION_INCLUDED
#define MY_ALLOCATION_INCLUDED

#include "MyInternals.hpp"
#include "../memory.hpp"

MY_CLASS_START

struct SystemAllocator {
    void* Alloc(size_t size) const {
        return malloc(size);
    }

    void Free(void* ptr) const {
        free(ptr);
    }

    void* Realloc(void* ptr, size_t size) const {
        return realloc(ptr, size);
    }
};

using DefaultAllocator = Mem::DefaultAllocator;

MY_CLASS_END

#endif