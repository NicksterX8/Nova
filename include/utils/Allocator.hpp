#ifndef UTILS_ALLOCATOR_INCLUDED
#define UTILS_ALLOCATOR_INCLUDED

#include <stdint.h>
#include <stddef.h>
#include <string>
#include <cassert>
#include "llvm/Compiler.h"

#define ADDRESS_SANITIZER_BUILD LLVM_ADDRESS_SANITIZER_BUILD

inline size_t getAlignedOffset(size_t offset, size_t align) {
    return ((offset + align - 1) & ~(align - 1U));
}

inline size_t getAlignmentOffset(size_t offset, size_t align) {
    return getAlignedOffset(offset, align) - offset;
}

// align the pointer to a given alignment, rounding up
inline void* alignPtr(void* ptr, size_t align) {
    uintptr_t ptrValue = (uintptr_t)ptr;
    return (void*)((ptrValue + align - 1) & ~(align - 1U));
}

// align the pointer to a given alignment, rounding up
inline const void* alignPtr(const void* ptr, size_t align) {
    uintptr_t ptrValue = (uintptr_t)ptr;
    return (void*)((ptrValue + align - 1) & ~(align - 1U));
}

template<typename Derived>
struct DeallocateMethods {
    template<typename T>
    void deallocate(T* ptr, size_t count = 1) {
        static_cast<Derived*>(this)->deallocate(ptr, count * sizeof(T), alignof(T));
    }

    template<typename T>
    void Delete(T* ptr, size_t count = 1) {
        // run destructor
        for (int i = 0; i < count; i++) {
            ptr[i].~T();
        }
        deallocate(ptr, count);
    }
};

template<typename Derived>
struct AllocateMethods {
    template<typename T>
    T* allocate(size_t count = 1) {
        return (T*)static_cast<Derived*>(this)->allocate(count * sizeof(T), alignof(T));
    }

    template<typename T, typename... Args>
    T* New(Args&... args) {
        T* mem = allocate<T>(1);
        new (mem) T(args...);
        return mem;
    }

    template<typename T>
    T* New(const T& value) {
        T* mem = allocate<T>(1);
        new (mem) T(value);
        return mem;
    }
};

class AllocatorI {};


template<typename Derived>
class AllocatorBase : AllocatorI, public AllocateMethods<Derived>, public DeallocateMethods<Derived> {
    // Required to define to be an allocator:
    // void* allocate(size_t size, size_t alignment)
    // void deallocate(void* ptr, size_t size, size_t alignment)
    // optional:
    // reallocate(void* ptr, size_t oldSize, size_t newSize, size_t alignment)
    // getAllocatorStats() recommended

public:
    using AllocateMethods<Derived>::allocate;

    void* reallocate(void* ptr, size_t oldSize, size_t newSize, size_t alignment) {
        void* newPtr = static_cast<Derived*>(this)->allocate(newSize, alignment);
        size_t sharedSize = (oldSize < newSize) ? oldSize : newSize;
        memcpy(newPtr, ptr, sharedSize);
        static_cast<Derived*>(this)->deallocate(ptr, oldSize, alignment);
        return newPtr;
    }

    /* Convenience methods for type allocations */

    template<typename T>
    T* reallocate(T* ptr, size_t oldCount, size_t newCount) {
        return (T*)static_cast<Derived*>(this)->reallocate(ptr, oldCount * sizeof(T), newCount * sizeof(T), alignof(T));
    }
};

struct AllocatorStats {
    std::string name;
    std::string allocated; // used by clients. ACTUALLY being used
    std::string used; // used by the allocator
};


template <typename Allocator>
class AllocatorHolder : public Allocator {
public:
    AllocatorHolder() = default;
    AllocatorHolder(const Allocator &allocator) : Allocator(allocator) {}
    AllocatorHolder(Allocator &&allocator) : Allocator(static_cast<Allocator &&>(allocator)) {}
    Allocator &getAllocator() { return *this; }
    const Allocator &getAllocator() const { return *this; }
};

template <typename Allocator>
class AllocatorHolder<Allocator&> {
    Allocator &allocator;
public:
    AllocatorHolder(Allocator& allocator) : allocator(allocator) {}
    Allocator &getAllocator() { return allocator; }
    const Allocator &getAllocator() const { return allocator; }
};


/* Polymorphic allocator made from any allocator base */
class AbstractAllocator {
protected:
    void* realAllocator;
    std::string name;
public:
    AbstractAllocator(void* realAllocator) : realAllocator(realAllocator) {}

    virtual void* allocate(size_t size, size_t alignment) {
        assert("No allocate method defined!");
        return nullptr;
    }

    virtual void deallocate(void* ptr, size_t size, size_t alignment) {
        assert("No deallocate method defined!");
    }

    virtual void* reallocate(void* ptr, size_t oldSize, size_t newSize, size_t alignment) {
        assert("No reallocate method defined!");
        return nullptr;
    }

    template<typename T>
    T* allocate(size_t count) {
        return (T*)allocate(count * sizeof(T), alignof(T));
    }

    template<typename T>
    void deallocate(T* ptr, size_t count) {
        deallocate(ptr, count * sizeof(T), alignof(T));
    }

    template<typename T>
    T* reallocate(T* ptr, size_t oldCount, size_t newCount) {
        return (T*)reallocate(ptr, oldCount * sizeof(T), newCount * sizeof(T), alignof(T));
    }

    virtual AllocatorStats getAllocatorStats() const {
        return {};
    }

    virtual ~AbstractAllocator() {}

    void setName(std::string_view name) {
        this->name = name;
    }

    const char* getName() const {
        if (name.empty()) return nullptr;
        return name.c_str();
    }
};


template<typename Allocator>
class AbstractAllocatorImpl : public AbstractAllocator {

public:
    AbstractAllocatorImpl(Allocator* realAllocator) : AbstractAllocator(realAllocator) {}

    void* allocate(size_t size, size_t alignment) override {
        return static_cast<Allocator*>(realAllocator)->allocate(size, alignment);
    }

    void deallocate(void* ptr, size_t size, size_t alignment) override {
        static_cast<Allocator*>(realAllocator)->deallocate(ptr, size, alignment);
    }

    void* reallocate(void* ptr, size_t oldSize, size_t newSize, size_t alignment) override {
        return static_cast<Allocator*>(realAllocator)->reallocate(ptr, oldSize, newSize, alignment);
    }

    AllocatorStats getAllocatorStats() const override {
        return static_cast<Allocator*>(realAllocator)->getAllocatorStats();
    }

    ~AbstractAllocatorImpl() {}
};

template<typename Allocator>
AbstractAllocator* makeAbstract(Allocator* allocator) {
    AbstractAllocator* abstract = new AbstractAllocatorImpl<Allocator>(allocator);
    return abstract;
}

/* Default allocator using malloc/free */
class Mallocator : public AllocatorBase<Mallocator> {
    using Base = AllocatorBase<Mallocator>;
public:

    void* allocate(size_t size) {
        return malloc(size);
    }

    void* allocate(size_t size, size_t alignment) {
        if (alignment > alignof(std::max_align_t))
            return aligned_alloc(alignment, size);
        else
            return malloc(size);
    }

    void deallocate(void* ptr) {
        free(ptr);
    }

    void deallocate(void* ptr, size_t size, size_t alignment) {
        free(ptr);
    }

    using Base::deallocate;

    void* reallocate(void* ptr, size_t newSize) {
        return realloc(ptr, newSize);
    }

    void* reallocate(void* ptr, size_t oldSize, size_t newSize, size_t alignment) {
        if (alignment < alignof(std::max_align_t)) {
            // just use regular realloc
            return realloc(ptr, newSize);
        }
        void* newPtr = allocate(newSize, alignment);
        size_t sharedSize = (oldSize < newSize) ? oldSize : newSize;
        memcpy(newPtr, ptr, sharedSize);
        deallocate(ptr, oldSize, alignment);
        return newPtr;
    }

    AllocatorStats getAllocatorStats() const {
        return {
            .name = "Mallocator"
        };
    }
};

static Mallocator mallocator;

#endif