#pragma once

#include "memory/Allocator.hpp"
#include "utils/compiler.hpp"

struct ScratchMemory : public AllocatorBase<ScratchMemory> {
private:
    using Base = AllocatorBase<ScratchMemory>;
protected:
    void* start;
    void* current;
    void* end;
public:

    ScratchMemory() : start(nullptr), current(nullptr), end(nullptr) {}

    ScratchMemory(void* start, void* end) : start(start), current(start), end(end) {}

    ScratchMemory(void* start, size_t size) : start(start), current(start), end((char*)start + size) {}

    template<typename Allocator>
    static ScratchMemory allocate(Allocator& allocator, size_t size) {
        return ScratchMemory(allocator.allocate(size, 1), size);
    }

    template<typename Allocator>
    static void deallocate(Allocator& allocator, const ScratchMemory& memory) {
        allocator.deallocate(memory.start, memory.allocationSize(), 1);
    }

    // allocate scratch memory with malloc of a given size. must call ScratchMemory::free when finished
    static ScratchMemory malloc(size_t size) {
        return ScratchMemory(::malloc(size), size);
    }

    // free memory created with ScratchMemory::malloc
    static void free(ScratchMemory& memory) {
        ::free(memory.start);
    }

    using Base::allocate;

    void* allocate(size_t size, size_t alignment) {
        void* ptr = alignPtr(current, alignment);
        current = (char*)ptr + size;
        __asan_unpoison_memory_region(ptr, size);
        assert(current <= end && "Out of space!");
        return ptr;
    }

    void deallocate(void* ptr, size_t size, size_t alignment) {
        // can not deallocate
        // act like we did though for sanitizers
        __asan_poison_memory_region(ptr, size);
    }

    static constexpr bool NeedDeallocate() {
        return false;
    }

    void reset() {
        // poision everything
        __asan_poison_memory_region(start, (uintptr_t)current - (uintptr_t)start);

        current = start;
    }

    size_t getAllocatedMemory() const {
        return (uintptr_t)current - (uintptr_t)start;
    }

    size_t spaceRemaining() const {
        return (uintptr_t)end - (uintptr_t)current;
    }

    size_t allocationSize() const {
        return (uintptr_t)end - (uintptr_t)start;
    }
};

template<typename Allocator = Mallocator>
class ScratchAllocator : public ScratchMemory, private AllocatorHolder<Allocator> {
    using Base = ScratchMemory;
    using Me = ScratchAllocator<Allocator>;
public:
    using AllocatorHolder<Allocator>::getAllocator;

    ScratchAllocator() = default;

    ScratchAllocator(size_t size) {
        init(size);
    }

    ScratchAllocator(size_t size, Allocator allocator) : AllocatorHolder<Allocator>(allocator) {
        init(size);
    }

    ScratchAllocator(const Me& other) = delete;

    ScratchAllocator(Me&& moved) {
        start = moved.start;
        current = moved.current;
        end = moved.end;

        moved.start = nullptr;
        moved.current = nullptr;
        moved.end = nullptr;
    }

    void init(size_t size) {
        assert(start == nullptr && "Scratch allocator already initialized!");
        start = getAllocator().allocate(size, 1);
        assert(start);
        __asan_poison_memory_region(start, size);
        current = start;
        end = (char*)start + size;
    }

    void deallocateAll() {
        __asan_poison_memory_region(start, (uintptr_t)current - (uintptr_t)start);
        getAllocator().deallocate(start, allocationSize(), 1);
        start = nullptr;
        current = nullptr;
        end = nullptr;
    }

    ~ScratchAllocator() {
        deallocateAll();
    }

    AllocatorStats getAllocatorStats() const {
        return {
            .name = "ScratchAllocator",
            .allocated = string_format("Allocated bytes: %zu", getAllocatedMemory()),
            .used = string_format("Used bytes: %zu", (uintptr_t)end - (uintptr_t)start)
        };
    }
};