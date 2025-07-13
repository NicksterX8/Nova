#ifndef MY_BUCKET_ARRAY_INCLUDED
#define MY_BUCKET_ARRAY_INCLUDED

#include "MyInternals.hpp"
#include <stddef.h>
#include "Allocation.hpp"
#include "ADT/SmallVector.hpp"

MY_CLASS_START

#define MY_BUCKET_ARRAY_START_BUCKET_COUNT 4

template<typename T, size_t BucketSize, typename BucketAllocator = Mallocator, typename MetaAllocator = Mallocator>
struct BucketArray {
    static_assert(BucketSize > 0, "BucketArray bucket size cannot be zero!");
    using BucketContainer = SmallVectorA<T*, MetaAllocator, 0>;
    BucketContainer buckets;
    int topBucketSlotsUsed = 0;

    EMPTY_BASE_OPTIMIZE DoubleAllocatorHolder<BucketAllocator, MetaAllocator> allocators;

    BucketArray() = default;

    template<typename BucketAllocatorT, typename MetaAllocatorT>
    BucketArray(BucketAllocatorT bucketAllocator, MetaAllocatorT metaAllocator)
    : allocators(static_cast<BucketAllocatorT&&>(bucketAllocator), static_cast<MetaAllocatorT&&>(metaAllocator)) {}

    template<typename BucketAllocatorT>
    BucketArray(BucketAllocatorT bucketAllocator)
    : allocators(static_cast<BucketAllocatorT&&>(bucketAllocator)) {}

    int size() const {
        return ((buckets.size() + (buckets.size() == 0)) * BucketSize) - BucketSize + topBucketSlotsUsed;
    }

    // may be null
    T* get(int elementIndex) const {
        if (elementIndex >= 0 && elementIndex < this->size()) {
            T* bucket = &buckets[elementIndex / BucketSize];
            if (bucket) return bucket[elementIndex % BucketSize];
        }
        return nullptr;
    }

    T& operator[](int elementIndex) const {
        assert(elementIndex >= 0 && "array index out of bounds");
        assert(elementIndex < this->size() && "array index out of bounds");
        return buckets[elementIndex / BucketSize][elementIndex % BucketSize];
    }

    bool empty() const {
        return buckets.size == 0 && topBucketSlotsUsed == 0;
    }

    void pushNewBucket() {
        buckets.push_back(allocators.template getAllocator<BucketAllocator>().template allocate<T>(BucketSize));
        topBucketSlotsUsed = 0;
    }

    T& back() const {
        assert(!this->empty() && "bucket array empty getting front");

        // we can simplify this if we assume buckets are never empty other than when the entire array is empty.
        // For now buckets can be empty, so we have to
        // check if topBucketSlotsUsed == 0. If buckets were never empty and the whole array wasn't empty, we could always access a bucket at index 0
        bool topBucketEmpty = topBucketSlotsUsed == 0;
        // get the end of second to top bucket instead of the top bucket when topBucketSlotsUsed == 0
        return buckets[buckets.size-1 - topBucketEmpty][topBucketSlotsUsed + topBucketEmpty * (BucketSize-1)];
    }

    T& front() const {
        assert(!this->empty() && "bucket array empty getting front");
        return buckets.front()[0];
    }

    T* push(FastestParamType<T> val) {
        // room for another element in top bucket?
        if (topBucketSlotsUsed >= BucketSize || buckets.size == 0) {
            pushNewBucket();
        }

        T* location = &buckets.back()[topBucketSlotsUsed++];
        *location = val;
        return location;
    }

    /* Same as push, but leave the value at the back uninitialized */
    T* reserveBack(int size = 1) {
        // room for another element in top bucket?
        if (topBucketSlotsUsed + size > BucketSize || buckets.empty()) {
            pushNewBucket();
        }
        auto topBucket = buckets.back();
        auto reserved = &topBucket[topBucketSlotsUsed];
        topBucketSlotsUsed += size;
        return reserved;
    }

    // assertion fail if no buckets exist
    void pop() {
        if (topBucketSlotsUsed > 0) {
            topBucketSlotsUsed--;
        } else {
            buckets.pop(); 
            topBucketSlotsUsed = BucketSize-1;
        }
    }

    // may be null
    T* getBucket(int bucketIndex) const {
        if (buckets.size() > bucketIndex)
            return &buckets[bucketIndex];
        else
            return nullptr;
    }

    void clear() {
        for (T* bucket : buckets) {
            allocators.template getAllocator<BucketAllocator>().deallocate(bucket);
        }
        buckets.clear();
    }

    void destroy() {
        clear();
    }

    ~BucketArray() {
        clear();
    }

    // iterator stuff

    struct iterator {
        T** bucketPtr;
        int index;

        iterator(T** buckets, int index) : bucketPtr(buckets), index(index) {}
        iterator operator++() {
            if (index < BucketSize) {
                index++;
            } else {
                index = 0;
                bucketPtr++;
            }
            return *this;
        }
        bool operator!=(const iterator& other) const { return bucketPtr != other.bucketPtr || index != other.index; }
        T& operator*() const { return (*bucketPtr)[index]; }
    };

    inline iterator begin() const {
        return iterator(&buckets.front(), 0);
    }

    inline iterator end() const {
        return iterator(&buckets.back(), topBucketSlotsUsed);
    }
};

MY_CLASS_END

#endif