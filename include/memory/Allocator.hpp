#ifndef MEMORY_ALLOCATOR_INCLUDED
#define MEMORY_ALLOCATOR_INCLUDED

#include <stdint.h>
#include <stddef.h>
#include <string>
#include <cassert>
#include "utils/compiler.hpp"
#include "My/String.hpp"

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

// TODO: consider moving these directly into AllocateBase instead of inheriting them. 
// We don't inherit them anywhere else at the moment and they just add extra complexity

template<typename Derived>
struct DeallocateMethods {
    template<typename T>
    void deallocate(T* ptr, size_t count = 1) {
        static_cast<Derived*>(this)->deallocate((void*)ptr, count * sizeof(T), alignof(T));
    }

    template<typename T>
    void Delete(T* ptr, size_t count = 1) {
        // destruct then deallocate
        for (int i = 0; i < count; i++) {
            ptr[i].~T();
        }
        deallocate(ptr, count);
    }

    template<typename T>
    void DeleteWithSize(T* ptr, size_t size, size_t count = 1) {
        for (int i = 0; i < count; i++) {
            ptr[i].~T();
        }
        static_cast<Derived*>(this)->deallocate((void*)ptr, count * size, alignof(T));
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
        ::new (mem) T(args...); // use ::new for in place new to avoid overloaded version
        return mem;
    }

    template<typename T>
    T* New(const T& value) {
        T* mem = allocate<T>(1);
        ::new (mem) T(value);
        return mem;
    }
};

class AllocatorI {};

template<typename Derived>
class AllocatorBase : public AllocatorI, public AllocateMethods<Derived>, public DeallocateMethods<Derived> {
    // Required to define to be an allocator:
    // void* allocate(size_t size, size_t alignment)
    // void deallocate(void* ptr, size_t size, size_t alignment)
    // even if deallocation isn't actually needed, define it because of template allocator usage and preferably poison the memory
    // optional:
    // reallocate(void* ptr, size_t oldSize, size_t newSize, size_t alignment)
    // static constexpr bool needDeallocate() if deallocate isn't necessary like a scratch allocator
    // getAllocatorStats() recommended

public:
    static constexpr bool NeedDeallocate() {
        return true; // default to yes. overrideable by derived if it's not
    }

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


// template<typename Allocator>
// class AllocatorHolder {
//     EMPTY_BASE_OPTIMIZE Allocator allocator;
// public:
//     AllocatorHolder() = default;
//     AllocatorHolder(const Allocator &allocator) : allocator(allocator) {}
//     AllocatorHolder(Allocator &&allocator) : allocator(static_cast<Allocator &&>(allocator)) {}
//     Allocator &getAllocator() { return allocator; }
//     const Allocator &getAllocator() const { return allocator; }
// };

// template<typename Allocator>
// class AllocatorHolder<Allocator&> {
//     Allocator &allocator;
// public:
//     AllocatorHolder(Allocator& allocator) : allocator(allocator) {}
//     Allocator &getAllocator() { return allocator; }
//     const Allocator &getAllocator() const { return allocator; }
// };

class SameAllocator {};

// allocator holder for two allocators which allows space optimization for when both are the same
template<typename Allocator1, typename Allocator2>
class DoubleAllocatorHolder {
    EMPTY_BASE_OPTIMIZE Allocator1 allocator1;
    EMPTY_BASE_OPTIMIZE Allocator2 allocator2;
public:
    DoubleAllocatorHolder() = default;

    DoubleAllocatorHolder(Allocator1&& allocator1, Allocator2&& allocator2)
    : allocator1(static_cast<Allocator1&&>(allocator1)), allocator2(static_cast<Allocator2&&>(allocator2)) {}

    template<typename Allocator>
    Allocator& getAllocator() {
        if constexpr (std::is_same_v<Allocator, Allocator1>) {
            return allocator1;
        } else {
            return allocator2;
        }
    }
};

template<typename Allocator1>
class DoubleAllocatorHolder<Allocator1, SameAllocator> {
    EMPTY_BASE_OPTIMIZE Allocator1 allocator1;
public:
    DoubleAllocatorHolder() = default;

    DoubleAllocatorHolder(Allocator1&& allocator)
    : allocator1(static_cast<Allocator1&&>(allocator1)) {}

    template<typename Allocator>
    Allocator1& getAllocator() {
        return allocator1;
    }
};

// maybe implement
// enum class AllocatorType {
//     Permanent,
//     Temporary
// };

/* Polymorphic allocator made from any allocator base */
class AbstractAllocator {
protected:
    AllocatorI* realAllocator;
    std::string name;
    bool deallocNecessary = true;
public:
    AbstractAllocator(AllocatorI* realAllocator) : realAllocator(realAllocator) {}

    AllocatorI* getPointer() const {
        return realAllocator;
    }

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
    AbstractAllocatorImpl(Allocator* realAllocator) : AbstractAllocator(realAllocator) {
        deallocNecessary = Allocator::NeedDeallocate();
    }

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
};

template<typename Allocator>
AbstractAllocator* makeAbstract(Allocator* allocator) {
    AbstractAllocator* abstract = NEW(AbstractAllocatorImpl<Allocator>(allocator));
    return abstract;
}

/* Default allocator using malloc/free */
class Mallocator : public AllocatorBase<Mallocator> {
    using Base = AllocatorBase<Mallocator>;
public:

    using Base::allocate;
    using Base::deallocate;

    void* allocate(size_t size, size_t alignment) {
        void* mem;
        if (alignment > alignof(std::max_align_t))
            mem = aligned_alloc(alignment, size);
        else
            mem = malloc(size);
        __asan_poison_memory_region(mem, size);
        return mem;
    }

    void deallocate(void* ptr, size_t size, size_t alignment) {
        free(ptr);
        __asan_unpoison_memory_region(ptr, size);
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


#endif