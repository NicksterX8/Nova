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
      
class AllocatorI;

void debugNewed(void* pointer, size_t bytes, const char* file, int line, AllocatorI* allocator = nullptr);

void debugDeleted(void* pointer, size_t bytes, const char* file, int line, AllocatorI* allocator = nullptr);

inline void* operator new(size_t size, AlignWrapper align) {
    return malloc(size);
}

template<typename Allocator>
void* operator new(size_t size, AlignWrapper align, Allocator& allocator) {
    return allocator.allocate(size, align.align);
}

inline void* operator new(size_t size, AlignWrapper align, const char* file, int line) {
    void* ptr = malloc(size);
    debugNewed(ptr, size, file, line);
    return ptr;
}

template<typename Allocator>
void* operator new(size_t size, AlignWrapper align, const char* file, int line, Allocator& allocator) {
    void* ptr = allocator.allocate(size, align.align);
    debugNewed(ptr, size, file, line, (AllocatorI*)(void*)&allocator);
    return ptr;
}

template<typename T>
void deleteArray(T* pointer, size_t count) {
    for (int i = 0; i < count; i++) {
        pointer[i].~T();
    }
    delete pointer;
}

template<typename T, typename Allocator>
void deleteArray(T* pointer, Allocator& allocator, size_t count) {
    allocator.Delete(pointer, count);
}

template<typename T, typename Allocator>
void deleteArray(T* pointer, const char* file, int line, Allocator& allocator, size_t count) {
    debugDeleted(pointer, sizeof(T) * count, file, line, &allocator);
    allocator.Delete(pointer, count);
}

// needed for deleting class from its base class pointer
template<typename T, typename Allocator>
void deleteArray(T* pointer, size_t objectSize, Allocator& allocator, size_t count) {
    allocator.DeleteWithSize(pointer, objectSize, count);
}

// needed for deleting class from its base class pointer
template<typename T, typename Allocator>
void deleteArray(T* pointer, const char* file, int line, size_t objectSize, Allocator& allocator, size_t count) {
    debugDeleted(pointer, objectSize * count, file, line, &allocator);
    allocator.DeleteWithSize(pointer, objectSize, count);
}

template<typename T>
T* newArray(size_t count) {
    return new T[count];
}

template<typename T>
T* newArray(size_t count, const char* file, int line) {
    T* ptr = new T[count];
    debugNewed(ptr, count * sizeof(T), file, line);
    return ptr;
}

template<typename T, typename Allocator>
T* newArray(size_t count, Allocator& allocator) {
    return allocator.template New<T>(count);
}

template<typename T, typename Allocator>
T* newArray(size_t count, const char* file, int line, Allocator& allocator) {
    T* ptr = allocator.template New<T>(count);
    debugNewed(ptr, count * sizeof(T), file, line, &allocator);
    return ptr;
}

#if MEMORY_DEBUG_LEVEL >= 1
#define ALLOCATION_DEBUG_LOCATION 1
#endif

#ifndef ALLOCATION_DEBUG_LOCATION
    #define NEW(constructor, ...) new (AlignWrapper{alignof(decltype(constructor))}, ##__VA_ARGS__) constructor
    #define NEW_ARR(type, count, ...) newArray<type>(count)
    #define DELETE(pointer, ...) deleteArray(pointer, ##__VA_ARGS__, 1)
    #define DELETE_ARR(pointer, count, ...) deleteArray(pointer, ##__VA_ARGS__, count)
#else
    #define NEW(constructor, ...) new (AlignWrapper{alignof(decltype(constructor))}, __FILE__, __LINE__, ##__VA_ARGS__) constructor
    // #define NEW_ARR(constructor, ...) new (AlignWrapper{alignof(constructor)}, ##__VA_ARGS__) constructor
    #define NEW_ARR(type, count, ...) newArray<type>(count, __FILE__, __LINE__, ##__VA_ARGS__)
    #define DELETE(pointer, ...) deleteArray(pointer, __FILE__, __LINE__, ##__VA_ARGS__, 1)
    #define DELETE_ARR(pointer, count, ...) deleteArray(pointer, __FILE__, __LINE__, ##__VA_ARGS__, count)
#endif

#endif