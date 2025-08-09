#include <gtest/gtest.h>
#include "memory/allocators.hpp"
#include "llvm/Allocator.h"

template<typename Allocator>
class AllocatorTest : public testing::Test {
public:

    Allocator allocator;
};

struct TestScratchAllocator : ScratchAllocator<> {
    TestScratchAllocator() : ScratchAllocator<>(100000) {}
};

struct AllocatorNames {
    template <typename T>
    static std::string GetName(int) {
        return T{}.getAllocatorStats().name;
    }
};


using Allocators = testing::Types<Mallocator, TestScratchAllocator, llvm::BumpPtrAllocatorImpl<>, BlockAllocator<256, 4>, FreelistAllocator<>>;

TYPED_TEST_SUITE(AllocatorTest, Allocators, AllocatorNames);

TYPED_TEST(AllocatorTest, AllocateAndFree) {
    int* ints = this->allocator.template allocate<int>(100);
    ASSERT_NE(ints, nullptr);
    this->allocator.deallocate(ints, 100);
}

TYPED_TEST(AllocatorTest, ZeroSizedAllocReturnsNonnull) {
    int* valid = this->allocator.template allocate<int>(0);
    EXPECT_NE(valid, nullptr);
}

TYPED_TEST(AllocatorTest, AllocGivesAlignedPointer) {
    size_t alignments[] = {1, 2, 4, 8, 16, 32, 64, 128, 256, 1024, 4096};
    for (int i = 0; i < sizeof(alignments) / sizeof(size_t); i++) {
        auto alignment = alignments[i];
        if (alignment > this->allocator.MaxAlignment()) continue;
        void* ptr = this->allocator.allocate(1, alignment);
        ASSERT_TRUE(ptr) << "Failed to allocate pointer with alignment " << alignment;
        uintptr_t uptr = (uintptr_t)ptr;
        EXPECT_EQ(uptr % alignment, 0) << "Tried to allocate pointer with alignment " << alignment << " but alignment was off";
        this->allocator.deallocate(ptr, 1, alignment);
    }
}