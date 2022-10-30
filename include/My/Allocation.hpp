#ifndef MY_ALLOCATION_INCLUDED
#define MY_ALLOCATION_INCLUDED

#include "MyInternals.hpp"
#include "../memory.hpp"

MY_CLASS_START

// CRTP based allocator
template<typename A>
struct IAllocator {
    A base;
    using size_type = typename A::size_type;
    using Size_T = size_type; // shorter to type

    void* Alloc(Size_T size) {
        return base.Alloc(size);
    }

    template<typename T>
    T* Alloc(Size_T count) {
        return (T*)base.Alloc(count * sizeof(T));
    }

    void Free(void* ptr) {
        base.Free(ptr);
    }

    void* Realloc(void* ptr, Size_T size) {
        return base.Realloc(ptr, size);
    }

    template<typename T>
    T* Realloc(T* ptr, Size_T count) {
        return (T*)base.Realloc(ptr, count * sizeof(T));
    }
};

struct SystemAllocator {
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

void test() {
    InheritanceWrapper< IAllocator<SystemAllocator> > allocator;
    allocator.Alloc<int>(2);
    allocator.Alloc(3);
    
}

using DefaultAllocator = Mem::DefaultAllocator;

MY_CLASS_END

#endif