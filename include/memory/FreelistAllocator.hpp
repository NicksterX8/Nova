#pragma once

#include "memory/Allocator.hpp"
#include "llvm/MathExtras.h"

// unfinished

// This allocator is NOT thread safe!
template<typename AllocatorT = Mallocator, size_t SmallestPowerOfTwoStored = 64, size_t LargestPowerOfTwoStored = 512>
struct FreelistAllocator : public AllocatorBase<FreelistAllocator<AllocatorT, SmallestPowerOfTwoStored, LargestPowerOfTwoStored>> {
private:
    using Base = AllocatorBase<FreelistAllocator<AllocatorT, SmallestPowerOfTwoStored, LargestPowerOfTwoStored>>;
    static_assert(llvm::CTLog2<LargestPowerOfTwoStored>() > 0, "Need positive power of two!");
    static_assert(llvm::CTLog2<SmallestPowerOfTwoStored>() > 0, "Need positive power of two");
    static_assert(LargestPowerOfTwoStored >= SmallestPowerOfTwoStored, "Largest Power of two cannot be smaller than smallest!");

    struct Node {
        Node* next;
    };

    static constexpr uint8_t NumLists = llvm::CTLog2<LargestPowerOfTwoStored>() - llvm::CTLog2<SmallestPowerOfTwoStored>() + 1;
    static constexpr uint8_t FirstListPowerOfTwo = llvm::CTLog2<SmallestPowerOfTwoStored>();

    Node* lists[NumLists] = {nullptr};

    EMPTY_BASE_OPTIMIZE AllocatorT allocator;
public:

    // return one of the nodes if it has the given size, if no nodes have that size, return null
    void* allocate(size_t size, size_t alignment) {
        uint8_t sizeLog2 = MAX(llvm::Log2_64_Ceil(size), FirstListPowerOfTwo);
        uint8_t listIndex = sizeLog2 - FirstListPowerOfTwo;
        if (LIKELY(listIndex < NumLists && alignment <= alignof(std::max_align_t))) {
            Node* node = lists[listIndex];
            if (node) {
                lists[listIndex] = node->next;
                return node;
            } else {
                // no free nodes, allocate one
                return allocator.allocate(1ULL << sizeLog2, alignof(std::max_align_t));
            }
        } else {
            // do custom alloc
            return allocator.allocate(size, alignment);
        }
    }
    
    using Base::allocate;

    void deallocate(void* ptr, size_t size, size_t alignment) {
        Node* node = (Node*)ptr;
        uint8_t sizeLog2 = MAX(llvm::Log2_64_Ceil(size), FirstListPowerOfTwo);
        uint8_t listIndex = sizeLog2 - FirstListPowerOfTwo;
        if (LIKELY(listIndex < NumLists && alignment <= alignof(std::max_align_t))) {
            // insert node at head of linked list
            node->next = lists[listIndex];
            lists[listIndex] = node;
        } else {
            // unsupported size or alignment
            allocator.deallocate(ptr, size, alignment);
        }
    }

    using Base::deallocate;

    // can only deallocate memory not currently in use. We have no way of tracking used allocations
    void deallocateAll() {
        if constexpr (AllocatorT::NeedDeallocate()) {
            for (uint8_t listIndex = 0; listIndex < NumLists; listIndex++) {
                Node* node = lists[listIndex];
                while (node) {
                    Node* n = node;
                    allocator.deallocate(node, 1ULL << (FirstListPowerOfTwo + listIndex), alignof(std::max_align_t));
                    node = n->next;
                }
                lists[listIndex] = nullptr;
            }
        }
    }

    static constexpr size_t goodSize(size_t minSize) {
        return llvm::PowerOf2Ceil(minSize); // we like powers of two
    }

    size_t calcUsedBytes() const {
        size_t usedBytes = 0;
        for (uint8_t listIndex = 0; listIndex < NumLists; listIndex++) {
            Node* node = lists[listIndex];
            while (node) {
                usedBytes += 1ULL << (FirstListPowerOfTwo + listIndex);
                node = node->next;
            }
        }
        return usedBytes;
    }

    AllocatorStats getAllocatorStats() const {
        return AllocatorStats{
            .name = "FreelistAllocator",
            .used = string_format("Used bytes: %zu in free nodes", calcUsedBytes()),
            .estimatedBytesUsed = calcUsedBytes()
        };
    }
};