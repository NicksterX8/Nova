#ifndef MY_ALLOCATION_INCLUDED
#define MY_ALLOCATION_INCLUDED

#include "MyInternals.hpp"
#include "memory/memory.hpp"

MY_CLASS_START

struct SystemAllocator : IAllocator<SystemAllocator> {
    using size_type = size_t;

    void* allocate(size_t size) const {
        return malloc(size);
    }

    void deallocate(void* ptr) const {
        free(ptr);
    }

    void* reallocate(void* ptr, size_t size) const {
        return realloc(ptr, size);
    }
};

/*
struct SystemAllocator2 : IAllocator<SystemAllocator2> {
    using size_type = size_t;

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

template<class Parent>
struct InheritanceWrapper : Parent {};
*/

using DefaultAllocator = Mem::DefaultAllocator;

MY_CLASS_END

#endif