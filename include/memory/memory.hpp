#ifndef MEMORY2_INCLUDED
#define MEMORY2_INCLUDED

#include <stdlib.h>
#include <functional>
#include <cassert>
#include "utils/Log.hpp"

template<typename T>
void destruct(T* elements, size_t count) {
    if constexpr (!std::is_trivially_destructible_v<T>) {
        for (size_t i = count; i-- > 0;) {
            (elements + i)->~T();
        }
    }
}

class AllocatorI;

namespace Mem {

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

using Mem::Alloc;
using Mem::Realloc;
using Mem::Free;

#if MEMORY_DEBUG_LEVEL >= 2
#define TRACK_ALLOCS 1
#endif

void debugAllocated(void* pointer, size_t bytes, const char* file, int line, AllocatorI* allocator = nullptr);
void debugDeallocated(void* pointer, size_t bytes, const char* file, int line, AllocatorI* allocator = nullptr);

#ifdef TRACK_ALLOCS
#define DEBUG_ALLOCATED(pointer, bytes, file, line, allocator) debugAllocated(pointer, bytes, file, line, allocator)
#define DEBUG_DEALLOCATED(pointer, bytes, file, line, allocator) debugDeallocated(pointer, bytes, file, line, allocator)
#else
#define DEBUG_ALLOCATED(pointer, bytes, file, line, allocator)
#define DEBUG_DEALLOCATED(pointer, bytes, file, line, allocator)
#endif

struct AlignWrapper {
    size_t align;
};

[[nodiscard]] inline void* operator new(size_t size, AlignWrapper align
#ifdef TRACK_ALLOCS
    , const char* file, int line
#endif
) {
    void* ptr = malloc(size);
    DEBUG_ALLOCATED(ptr, size, file, line, nullptr);
    return ptr;
}

template<typename Allocator>
[[nodiscard]] void* operator new(size_t size, AlignWrapper align, 
#ifdef TRACK_ALLOCS
    const char* file, int line,
#endif 
    Allocator& allocator) {
    void* ptr = allocator.allocate(size, align.align);
    if (allocator.NeedDeallocate())
        DEBUG_ALLOCATED(ptr, size, file, line, &allocator);
    return ptr; 
}

template<typename T, typename Allocator>
void deleteArray(T* pointer, 
#ifdef TRACK_ALLOCS
    const char* file, int line, 
#endif 
    Allocator& allocator, size_t count) {
    static_assert(!std::is_void_v<T>, "Cannot delete void pointer, use dealloc instead");
    if (allocator.NeedDeallocate())
        DEBUG_DEALLOCATED(pointer, sizeof(T) * count, file, line, &allocator);
    allocator.Delete(pointer, count);
}

template<typename T>
void deleteArray(T* pointer, 
#ifdef TRACK_ALLOCS
    const char* file, int line,
#endif
    size_t count) {
    static_assert(!std::is_void_v<T>, "Cannot delete void pointer, use dealloc instead");
    DEBUG_DEALLOCATED(pointer, sizeof(T) * count, file, line, nullptr);
    destruct(pointer, count);
    free(pointer);
}

template<typename T>
[[nodiscard]] T* newArray(size_t count
#ifdef TRACK_ALLOCS
    , const char* file, int line
#endif
) {
    T* ptr = new (malloc(count * sizeof(T))) T[count];
    DEBUG_ALLOCATED(ptr, count * sizeof(T), file, line, nullptr);
    return ptr;
}

template<typename T, typename Allocator>
[[nodiscard]] T* newArray(size_t count, const char* file, int line, Allocator& allocator) {
    T* ptr = allocator.template New<T>(count);
    if (allocator.NeedDeallocate())
        DEBUG_ALLOCATED(ptr, count * sizeof(T), file, line, &allocator);
    return ptr;
}

template<typename T, typename Allocator>
void deallocArray(T* pointer, 
#ifdef TRACK_ALLOCS
    const char* file, int line,
#endif
    Allocator& allocator, size_t count = 1) {
    if (allocator.NeedDeallocate())
        DEBUG_DEALLOCATED(pointer, sizeof(T) * count, file, line, &allocator);
    allocator.deallocate(pointer, count);
}

template<typename T, typename Allocator>
void deallocArray(T* pointer, 
#ifdef TRACK_ALLOCS
    const char* file, int line,
#endif
    size_t count, Allocator& allocator) {
    if (allocator.NeedDeallocate())
        DEBUG_DEALLOCATED(pointer, sizeof(T) * count, file, line, &allocator);
    allocator.deallocate(pointer, count);
}

template<typename Allocator>
inline void deallocArray(void* pointer, 
#ifdef TRACK_ALLOCS
    const char* file, int line,
#endif
    size_t size, size_t alignment, Allocator& allocator) {
    if (allocator.NeedDeallocate())
        DEBUG_DEALLOCATED(pointer, size, file, line, &allocator);
    allocator.deallocate(pointer, size, alignment);
}

inline void deallocArray(void* pointer, 
#ifdef TRACK_ALLOCS
    const char* file, int line, 
#endif
    size_t size) {
    DEBUG_DEALLOCATED(pointer, size, file, line, nullptr);
    free(pointer);
}

template<typename T>
void deallocArray(T* pointer, 
#ifdef TRACK_ALLOCS
    const char* file, int line, 
#endif
    size_t count = 1) {
    DEBUG_DEALLOCATED(pointer, sizeof(T) * count, file, line, nullptr);
    free(pointer);
}

template<typename T>
[[nodiscard]] T* allocArray(
#ifdef TRACK_ALLOCS
    const char* file, int line,
#endif
    size_t count = 1) {
    T* ptr = (T*)malloc(count * sizeof(T));
    DEBUG_ALLOCATED(ptr, count * sizeof(T), file, line, nullptr);
    return ptr;
}

template<>
[[nodiscard]] inline void* allocArray<void>(
#ifdef TRACK_ALLOCS
    const char* file, int line,
#endif
    size_t count) {
    void* ptr = malloc(count);
    DEBUG_ALLOCATED(ptr, count, file, line, nullptr);
    return ptr;
}

template<typename T, typename Allocator>
[[nodiscard]] T* allocArray(
#ifdef TRACK_ALLOCS
    const char* file, int line, 
#endif
    Allocator& allocator, size_t count = 1) {
    T* ptr = allocator.template allocate<T>(count);
    if (allocator.NeedDeallocate())
        DEBUG_ALLOCATED(ptr, count * sizeof(T), file, line, &allocator);
    return ptr;
}

template<typename T, typename Allocator>
[[nodiscard]] T* allocArray(
#ifdef TRACK_ALLOCS
    const char* file, int line, 
#endif
    size_t count, Allocator& allocator) {
    T* ptr = allocator.template allocate<T>(count);
    if (allocator.NeedDeallocate())
        DEBUG_ALLOCATED(ptr, count * sizeof(T), file, line, &allocator);
    return ptr;
}


#ifndef TRACK_ALLOCS
    #define NEW(constructor, ...) new (AlignWrapper{alignof(decltype(constructor))}, ##__VA_ARGS__) constructor
    #define NEW_ARR(type, count, ...) newArray<type>(count)
    #define DELETE(pointer, ...) deleteArray(pointer, ##__VA_ARGS__, 1)
    #define DELETE_ARR(pointer, count, ...) deleteArray(pointer, ##__VA_ARGS__, count)
    #define DELETE_SIZED_OBJECT(pointer, size, ...) deleteSizedObject(pointer, size, ##__VA_ARGS__)
    #define ALLOC(type, ...) allocArray<type>(__VA_ARGS__)
    #define DEALLOC(pointer, ...) deallocArray(pointer, ##__VA_ARGS__)
#else
    #define NEW(constructor, ...) new (AlignWrapper{alignof(decltype(constructor))}, __FILE__, __LINE__, ##__VA_ARGS__) constructor
    #define NEW_ARR(type, count, ...) newArray<type>(count, __FILE__, __LINE__, ##__VA_ARGS__)
    #define DELETE(pointer, ...) deleteArray(pointer, __FILE__, __LINE__, ##__VA_ARGS__, 1)
    #define DELETE_ARR(pointer, count, ...) deleteArray(pointer, __FILE__, __LINE__, ##__VA_ARGS__, count)
    #define ALLOC(type, ...) allocArray<type>(__FILE__, __LINE__, ##__VA_ARGS__)
    #define DEALLOC(pointer, ...) deallocArray(pointer, __FILE__, __LINE__, ##__VA_ARGS__)
#endif

#endif