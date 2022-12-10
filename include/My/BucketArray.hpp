#ifndef MY_BUCKET_ARRAY_INCLUDED
#define MY_BUCKET_ARRAY_INCLUDED

#include "MyInternals.hpp"
#include <stddef.h>
#include "Vec.hpp"
#include "Allocation.hpp"

MY_CLASS_START

#define MY_BUCKET_ARRAY_START_BUCKET_COUNT 4

template<typename T, size_t BucketSize, class Allocator = DefaultAllocator, class BucketAllocator = DefaultAllocator>
struct BucketArray {
    static_assert(BucketSize > 0, "BucketArray bucket size cannot be zero!");
private:
    using Self = BucketArray<T, BucketSize, Allocator, BucketAllocator>;
public:
    using BucketContainer = Vec<T*, BucketAllocator>;
    BucketContainer buckets;
    int topBucketSlotsUsed;

    static Allocator allocator;
    static BucketAllocator bucketAllocator;

    /*
    BucketArray() = default;

    BucketArray(const Vec<T*, Allocator>& buckets, int topBucketSlotsUsed=0)
    : buckets(buckets), topBucketSlotsUsed(topBucketSlotsUsed) {}

    BucketArray(int elementCapacity) {
        int bucketCapacity = elementCapacity/BucketSize + (elementCapacity % BucketSize != 0);
        buckets = Vec<T*>{
            (T**)allocator.Alloc(bucketCapacity * sizeof(T*)), // data
            0, // size
            bucketCapacity // capacity
        };
        topBucketSlotsUsed = 0;
    }
    */

    static Self WithCapacity(int elementCapacity) {
        Self self;
        int bucketCapacity = elementCapacity/BucketSize + (elementCapacity % BucketSize != 0);
        self.buckets = BucketContainer{
            (T**)allocator.Alloc(bucketCapacity * sizeof(T*)), // data
            0, // size
            bucketCapacity // capacity
        };
        self.topBucketSlotsUsed = 0;
        return self;
    }

    static Self WithBuckets(int bucketCapacity) {
        return Self{BucketContainer::WithCapacity(bucketCapacity), 0};
    }

    static Self Empty() {
        return {{0,0,0}, 0};
    }

    int size() const {
        return ((buckets.size+(buckets.size==0)) * BucketSize) - BucketSize + topBucketSlotsUsed;
    }

    T* get(int elementIndex) const {
        if (elementIndex >= 0 && elementIndex < this->size()) {
            T* bucket = buckets.get(elementIndex / BucketSize);
            if (bucket) return bucket[elementIndex % BucketSize];
        }
        return nullptr;
    }

    T& operator[](int elementIndex) const {
        assert(elementIndex > -1 && "array index out of bounds");
        assert(elementIndex < this->size() && "array index out of bounds");
        return buckets[elementIndex / BucketSize][elementIndex % BucketSize];
    }

    bool empty() const {
        return buckets.size == 0 && topBucketSlotsUsed == 0;
    }

    void pushNewBucket() {
        buckets.push(allocator.template Alloc<T>(BucketSize));
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
        if (topBucketSlotsUsed + size > BucketSize || buckets.size == 0) {
            pushNewBucket();
        }
        auto reserved =  &buckets.back()[topBucketSlotsUsed];
        topBucketSlotsUsed += size;
        return reserved;
    }

    void pop() {
        if (topBucketSlotsUsed != 0) {
            topBucketSlotsUsed--;
        } else {
            buckets.pop(); // assertion fail if no buckets exist
            topBucketSlotsUsed = BucketSize-1;
        }
    }

    void popReduce(int maxUnusedCapacity) {
        if (topBucketSlotsUsed != 0) {
            topBucketSlotsUsed--;
        } else {
            buckets.pop(); // assertion fail if no buckets exist
            if (buckets.capacity > buckets.size + maxUnusedCapacity) {
                // don't use any extra memory
                buckets.reallocate(buckets.size);
            }
            topBucketSlotsUsed = BucketSize-1;
        }
    }

    // may be null
    T* getBucket(int bucketIndex) const {
        return buckets.get(bucketIndex);
    }

    void destroy() {
        for (T* bucket : buckets) {
            allocator.Free(bucket);
        }
        buckets.destroy();
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

template<typename T, size_t BucketSize, class Allocator, class BucketAllocator>
Allocator BucketArray<T, BucketSize, Allocator, BucketAllocator>::allocator = Allocator();

template<typename T, size_t BucketSize, class Allocator, class BucketAllocator>
BucketAllocator BucketArray<T, BucketSize, Allocator, BucketAllocator>::bucketAllocator = BucketAllocator();

MY_CLASS_END

#endif