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

class AllocatorI {};

template<typename Derived>
class AllocatorBase : AllocatorI {
    // Required to define to be an allocator:
    // void* allocate(size_t size, size_t alignment)
    // void deallocate(void* ptr, size_t size, size_t alignment)
    // optional:
    // reallocate(void* ptr, size_t oldSize, size_t newSize, size_t alignment)
    // getAllocatorStats() recommended

public:

    void* reallocate(void* ptr, size_t oldSize, size_t newSize, size_t alignment) {
        void* newPtr = static_cast<Derived*>(this)->allocate(newSize, alignment);
        size_t sharedSize = (oldSize < newSize) ? oldSize : newSize;
        memcpy(newPtr, ptr, sharedSize);
        static_cast<Derived*>(this)->deallocate(ptr, oldSize, alignment);
        return newPtr;
    }

    /* Convenience methods for type allocations */

    template<typename T>
    T* allocate(size_t count) {
        return (T*)static_cast<Derived*>(this)->allocate(count * sizeof(T), alignof(T));
    }

    template<typename T>
    void deallocate(T* ptr, size_t count) {
        static_cast<Derived*>(this)->deallocate(ptr, count * sizeof(T), alignof(T));
    }

    template<typename T>
    T* reallocate(T* ptr, size_t oldCount, size_t newCount) {
        return (T*)static_cast<Derived*>(this)->reallocate(ptr, oldCount * sizeof(T), newCount * sizeof(T), alignof(T));
    }

    /* initialization + allocation */

    template<typename T>
    T* New(const T& value) {
        T* mem = (T*)static_cast<Derived*>(this)->allocate(sizeof(T), alignof(T));
        new (mem) T(value);
        return mem;
    }

    // default construct
    template<typename T>
    T* New() {
        T* mem = allocate<T>(1);
        new (mem) T();
        return mem;
    }
};

struct AllocatorStats {
    std::string name;
    std::string allocated; // used by clients. ACTUALLY being used
    std::string used; // used by the allocator
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

template<typename, typename = void>
struct HasAllocateT : std::false_type {};
template<typename Allocator>
struct HasAllocateT<Allocator, std::void_t<decltype(noexcept(std::declval<Allocator>().allocate(0, 0)))>> : std::true_type {};

template<typename, typename = void>
struct HasDeallocateT : std::false_type {};
template<typename Allocator>
struct HasDeallocateT<Allocator, std::void_t<decltype(noexcept(std::declval<Allocator>().deallocate(nullptr, 0, 0)))>> : std::true_type {};

template<typename, typename = void>
struct HasAllocatorStatsT : std::false_type {};
template<typename Allocator>
struct HasAllocatorStatsT<Allocator, std::void_t<decltype(noexcept(std::declval<Allocator>().getAllocatorStats()))>> : std::true_type {};


template<typename Allocator>
class AbstractAllocatorImpl : public AbstractAllocator {

public:
    AbstractAllocatorImpl(Allocator* realAllocator) : AbstractAllocator(realAllocator) {}

    void* allocate(size_t size, size_t alignment) override {
        if constexpr (HasAllocateT<Allocator>::value) {
            return static_cast<Allocator*>(realAllocator)->allocate(size, alignment);
        } else {
            return nullptr;
        }
    }

    void deallocate(void* ptr, size_t size, size_t alignment) override {
        if constexpr (HasDeallocateT<Allocator>::value) {
            static_cast<Allocator*>(realAllocator)->deallocate(ptr, size, alignment);
        }
    }

    void* reallocate(void* ptr, size_t oldSize, size_t newSize, size_t alignment) override {
        if constexpr (HasAllocateT<Allocator>::value && HasDeallocateT<Allocator>::value) {
            return static_cast<Allocator*>(realAllocator)->reallocate(ptr, oldSize, newSize, alignment);
        } else {
            return nullptr;
        }
    }

    AllocatorStats getAllocatorStats() const override {
        if constexpr (HasAllocatorStatsT<Allocator>::value) {
            return static_cast<Allocator*>(realAllocator)->getAllocatorStats();
        } else {
            return {};
        }
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

    using Base::deallocate;

    void deallocate(void* ptr, size_t size, size_t alignment) {
        free(ptr);
    }

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