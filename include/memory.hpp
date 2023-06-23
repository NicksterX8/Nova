#ifndef MEMORY2_INCLUDED
#define MEMORY2_INCLUDED

#include <stdlib.h>
#include <functional>

namespace Mem {

/* Wrappers */

void* _malloc(size_t size);
void  _free(void* ptr);
void* _realloc(void* ptr, size_t size);

/* Variations */

struct MemoryBlock {
    void* ptr;
    size_t size;
};

// TODO: get rid of this because it is a static constructor
extern std::function<void()> needMemoryCallback;
extern std::function<void()> memoryFailCallback;

using AllocCallback = std::function<MemoryBlock()>;

// \param required whether the memory is required to be allocated or NULL can be returned
MemoryBlock failedToAlloc(bool required, AllocCallback callback);

template<typename T>
struct MemoryBlockT {
    T* ptr;
    size_t size;
};

MemoryBlock min_malloc(size_t min, size_t preferred, bool nonnull=false);
inline MemoryBlock safe_min_malloc(size_t min, size_t preferred) {
    return min_malloc(min, preferred, true);
}

MemoryBlock min_realloc(void* ptr, size_t min, size_t preferred, bool nonnull=false);
inline MemoryBlock safe_min_realloc(void* ptr, size_t min, size_t preferred) {
    return min_realloc(ptr, min, preferred, true);
}

MemoryBlock min_aligned_malloc(size_t min, size_t preferred, size_t alignment, bool nonnull=false);

void* safe_malloc(size_t size);
void* safe_realloc(void* oldMem, size_t size);

/* Debug versions */

void* debug_malloc(void *malloc_func(size_t), size_t size, const char* file, int line);
void debug_free(void free_func(void*), void* ptr, const char* file, int line) noexcept;

} // namespace Mem

// new never returns null
void* operator new(size_t size, const char* file, int line);
void operator delete(void* ptr, const char* file, int line) noexcept;

namespace Mem {

void handler(int sig);

inline void init(std::function<void()> _needMemoryCallback, std::function<void()> _memoryFailCallback) {
   needMemoryCallback = _needMemoryCallback;
   memoryFailCallback = _memoryFailCallback;
   signal(SIGSTOP, handler);
}

inline void quit() {
    // nothing yet
}

/*
// unused 
inline void* my_aligned_malloc(size_t size, size_t alignment) {
    void* mem = _malloc(size + alignment);
    uintptr_t offset = alignment - (uintptr_t)mem % alignment;
    return (char*)mem + offset;
}

// unused
inline void my_aligned_free(void* mem, size_t alignment) {
    uintptr_t offset = alignment - (uintptr_t)mem % alignment;
    _free((char*)mem - offset);
}
*/

inline void* aligned_malloc(size_t size, size_t alignment, bool nonnull = false) {
    void* memory;
    int code = posix_memalign(&memory, alignment, size);
    (void)code;
    // TODO: something with error code
    return memory;
}

inline void aligned_free(void* ptr) {
    // for now it's the exact same, but only on unix
    _free(ptr);
}

// Not a very useful function right now
inline void* aligned_realloc(void* mem, size_t size, size_t alignment, bool nonnull = false) {
    void* newMem = aligned_malloc(size, alignment, nonnull);
    if (!newMem) return NULL;
    memcpy(newMem, mem, size);
    aligned_free(mem);
    return newMem;
}

MemoryBlock aligned_min_realloc(void* mem, size_t min, size_t preferred, size_t alignment, bool nonnull = false);

/* Interface */

template<typename T>
T* Alloc(size_t count = 1) {
    return (T*)safe_malloc(count * sizeof(T));
}

inline void* Alloc(size_t sizeBytes) {
    return safe_malloc(sizeBytes);
}

inline void* Alloc(size_t count, size_t typeSize) {
    return safe_malloc(count * typeSize);
}

template<typename T>
MemoryBlockT<T> AllocMin(size_t min, size_t preferred) {
    return (T*)safe_min_malloc(min * sizeof(T), preferred * sizeof(T));
}

inline MemoryBlock AllocMin(size_t min, size_t preferred, size_t typeSize) {
    return safe_min_malloc(min * typeSize, preferred * typeSize);
}

template<typename T>
T* Realloc(T* old, size_t newCount) {
    return (T*)safe_realloc(old, newCount * sizeof(T));
}

template<>
inline void* Realloc<void>(void* old, size_t newSize) {
    return safe_realloc(old, newSize);
}

template<typename T>
MemoryBlockT<T> ReallocMin(T* old, size_t minSize, size_t preferredSize) {
    auto block = safe_min_realloc(old, minSize * sizeof(T), preferredSize * sizeof(T));
    return {(T*)block.ptr, block.size};
}

inline MemoryBlock ReallocMin(void* old, size_t minSize, size_t preferredSize, size_t typeSize) {
    return safe_min_realloc(old, minSize*typeSize, preferredSize*typeSize);
}

template<typename T>
T* ReallocAligned(T* old, size_t count, size_t alignment) {
    return aligned_realloc(old, count * sizeof(T), alignment, true);
}

inline void Free(void* ptr) {
    _free(ptr);
}

inline void FreeAligned(void* ptr) {
    aligned_free(ptr);
}

template<typename T>
T* AllocAligned(size_t count, size_t alignment) {
    return aligned_malloc(count * sizeof(T), alignment, true);
}

inline void* AllocAligned(size_t sizeBytes, size_t alignment) {
    return aligned_malloc(sizeBytes, alignment, true);
}

template<typename T>
MemoryBlockT<T> AllocMinAligned(size_t minCount, size_t preferredCount, size_t alignment) {
    MemoryBlock block = min_aligned_malloc(minCount * sizeof(T), preferredCount * sizeof(T), alignment, true);
    return {(T*)block.ptr, block.size / sizeof(T)};
}

inline MemoryBlock AllocMinAligned(size_t minSize, size_t preferredSize, size_t alignment) {
    return min_aligned_malloc(minSize, preferredSize, alignment, true);
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

    constexpr size_t maxAllocationSize() const {
        return std::numeric_limits<size_type>::max();
    }

    void assertSafeSize(size_t size) {
        assert(size <= maxAllocationSize() && "Allocation too large!");
    }

    void* Alloc(size_t size) {
        assertSafeSize(size);
        //return base.Alloc(size);
        return derived().allocate(size);
    }

    template<typename T>
    T* Alloc(size_t count) {
        assertSafeSize(count * sizeof(T));
        //return (T*)base.Alloc(count * sizeof(T));
       // return (T*)Alloc(count * sizeof(T));
        return (T*)derived().allocate(count * sizeof(T));
    }

    void Free(void* ptr) {
        derived().deallocate(ptr);
    }

    template<typename T>
    T* Realloc(T* ptr, size_t count) {
        //assertSafeSize(count * sizeof(T));
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

template<size_t Alignment>
struct AlignmentAllocator : My::IAllocator< AlignmentAllocator<Alignment> > {
    using size_type = size_t;

    void* allocate(size_t size) const {
        return Mem::AllocAligned(size, Alignment);
    }
    void deallocate(void* ptr) const {
        Mem::FreeAligned(ptr);
    }
    void* reallocate(void* ptr, size_t size) const {
        return Mem::ReallocAligned(ptr, size, Alignment);
    }
};

#define ScopedAlloc(type, name, size) auto name = Alloc<type>(size); defer { Free(name); }; do {} while(0)

} // namespace Mem

using Mem::_free;
using Mem::_malloc;
using Mem::_realloc;

using Mem::safe_malloc;
using Mem::safe_min_malloc;
using Mem::safe_realloc;
using Mem::safe_min_realloc;

using Mem::Alloc;
using Mem::Realloc;
using Mem::Free;
using Mem::ReallocMin;

using Mem::MemoryBlock;
using Mem::MemoryBlockT;

// macros
//#define malloc(size) safe_malloc(size)
//#define free(ptr) _free(ptr)
//#define realloc(ptr, size) safe_realloc(ptr, size)

#define new_debug new (__FILE__, __LINE__)
#define delete_debug delete (__FILE__, __LINE__)

#endif