#ifndef MEMORY2_INCLUDED
#define MEMORY2_INCLUDED

#include <stdlib.h>
#include <functional>
#include <cassert>
#include "utils/Log.hpp"

namespace Mem {

/* Wrappers */

void* _malloc(size_t size);
void  _free(void* ptr);
void* _realloc(void* ptr, size_t size);


/* Debug versions */

void* debug_malloc(size_t size, const char* file, int line);
void debug_free(void* ptr, const char* file, int line) noexcept;

} // namespace Mem

// new never returns null
// void* operator new(size_t size, const char* file, int line);
// void operator delete(void* ptr, const char* file, int line) noexcept;

namespace Mem {

/* Interface */

template<typename T>
T* Alloc(size_t count = 1) {
    return (T*)malloc(count * sizeof(T));
}

inline void* Alloc(size_t sizeBytes) {
    return malloc(sizeBytes);
}

inline void* Realloc(void* old, size_t newSize) {
    return realloc(old, newSize);
}

template<typename T>
T* Realloc(T* old, size_t newCount) {
    return (T*)realloc(old, newCount * sizeof(T));
}

inline void Free(void* ptr) {
    free(ptr);
}

inline void Free(void* ptr, size_t alignment) {
    free(ptr);
}

inline void* Alloc(size_t sizeBytes, size_t alignment) {
    return std::aligned_alloc(alignment, sizeBytes);
}

} // namespace Mem

namespace My { 

// CRTP based allocator
template<typename Derived>
struct IAllocator {
    //A base;
    using size_type = size_t;

    Derived& derived() {
        return *static_cast<Derived*>(this);
    }

    const Derived& derived() const {
        return *static_cast<Derived*>(this);
    }

    void* Alloc(size_t size) {
        return derived().allocate(size);
    }

    template<typename T>
    T* Alloc(size_t count) {
        return (T*)derived().allocate(count * sizeof(T));
    }

    void Free(void* ptr) {
        derived().deallocate(ptr);
    }

    template<typename T>
    T* Realloc(T* ptr, size_t count) {
        return (T*)Realloc<void>((void*)ptr, count * sizeof(T));
    }

    template<>
    void* Realloc<void>(void* ptr, size_t size) {
        return derived().reallocate(ptr, size);
    }
};

}

namespace Mem {

struct DefaultAllocator : My::IAllocator<DefaultAllocator> {
    using size_type = size_t;

    void* allocate(size_t size) const {
        return Mem::Alloc(size);
    }

    void deallocate(void* ptr) const {
        Mem::Free(ptr);
    }

    void* reallocate(void* ptr, size_t size) const {
        return Mem::Realloc(ptr, size);
    }
};

} // namespace Mem

using Mem::Alloc;
using Mem::Realloc;
using Mem::Free;

template<typename T>
T* New() {
    T* mem = Alloc<T>();
    new (mem) T();
    return mem;
}

template<typename T, typename... Args>
T* New(Args&... args) {
    T* mem = Alloc<T>();
    new (mem) T(args...);
    return mem;
}

template<typename T>
void Delete(T* pointer, size_t count = 1) {
    Free(pointer);
    for (int i = 0; i < count; i++) {
        pointer[i].~T();
    }
}

struct AlignWrapper {
    size_t align;
};

inline void* operator new(size_t size, AlignWrapper align) {
    return malloc(size);
}

inline void* operator new[](size_t size, AlignWrapper align) {
    return malloc(size);
}

template<typename Allocator>
void* operator new(size_t size, AlignWrapper align, Allocator& allocator) {
    return allocator.allocate(size, align.align);
}

template<typename Allocator>
void* operator new[](size_t size, AlignWrapper align, Allocator& allocator) {
    return allocator.allocate(size, align.align);
}

inline void* operator new(size_t size, AlignWrapper align, const char* file, int line) {
    LogInfo("%s:%d New", file, line);
    return malloc(size);
}

template<typename Allocator>
void* operator new(size_t size, AlignWrapper align, const char* file, int line, Allocator& allocator) {
    LogInfo("%s:%d New", file, line);
    return allocator.allocate(size, align.align);
}

#define ALLOCATION_DEBUG_LOCATION 1

#ifndef ALLOCATION_DEBUG_LOCATION
    #define NEW(constructor, ...) new (AlignWrapper{alignof(decltype(constructor))}, ##__VA_ARGS__) constructor
    #define NEW_ARR(constructor, ...) new (AlignWrapper{alignof(constructor)}, ##__VA_ARGS__) constructor
#else
    #define NEW(constructor, ...) new (AlignWrapper{alignof(decltype(constructor))}, __FILE__, __LINE__, ##__VA_ARGS__) constructor
    #define NEW_ARR(constructor, ...) new (AlignWrapper{alignof(constructor)}, ##__VA_ARGS__) constructor
#endif

#endif