#ifndef MY_BUCKET_ARRAY_INCLUDED
#define MY_BUCKET_ARRAY_INCLUDED

#include "MyUtils.hpp"
#include <stddef.h>
#include "Vec.hpp"

MY_CLASS_START

#define MY_BUCKET_ARRAY_START_BUCKET_COUNT 4

template<class T, size_t BucketSize>
struct BucketArray {
    using Self = BucketArray<T, BucketSize>;

    Vec<T*> buckets;
    int topBucketSlotsUsed;

    BucketArray() = default;

    BucketArray(const Vec<T*>& buckets, int topBucketSlotsUsed=0)
    : buckets(buckets), topBucketSlotsUsed(topBucketSlotsUsed) {}

    BucketArray(int elementCapacity) {
        int bucketCapacity = elementCapacity/BucketSize + (elementCapacity % BucketSize != 0);
        buckets = Vec<T*>{
            (T**)MY_malloc(bucketCapacity * sizeof(T*)), // data
            0, // size
            bucketCapacity // capacity
        };
        topBucketSlotsUsed = 0;
    }

    static Self WithCapacity(int elementCapacity) {
        return BucketArray(elementCapacity);
    }

    static Self WithBuckets(int bucketCapacity) {
        return Self(Vec<T*>::WithCapacity(bucketCapacity));
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

    T** pushNewBucket() {
        T* bucket = (T*)MY_malloc(BucketSize * sizeof(T));
        if (bucket) {
            T** bucketPtr = buckets.push(bucket);
            topBucketSlotsUsed = 0;
            return bucketPtr;
        }
        return nullptr;
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

    T* push(const T& val) {
        T** topBucketPtr;
        // room for another element in top bucket?
        if (topBucketSlotsUsed < BucketSize && buckets.size > 0) {
            topBucketPtr = &buckets.back();
        } else {
            // need new bucket
            topBucketPtr = pushNewBucket(); // failable on memory error
            if (!topBucketPtr) {
                return nullptr;
            }
        }

        T* const location = &((*topBucketPtr)[topBucketSlotsUsed++]);
        *location = val;
        return location;
    }

    /* Same as push, but leave the value at the back uninitialized */
    T* reserveBack() {
        T** topBucketPtr;
        // room for another element in top bucket?
        if (topBucketSlotsUsed < BucketSize && buckets.size > 0) {
            topBucketPtr = &buckets.back();
        } else {
            // need new bucket
            topBucketPtr = pushNewBucket(); // failable on memory error
            if (!topBucketPtr) {
                return nullptr;
            }
        }

        return &((*topBucketPtr)[topBucketSlotsUsed++]);
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
            MY_free(bucket);
        }
        buckets.destroy();
    }

    // iterator stuff

    struct Iterator {
        T** array;
        int bucket;
        int index;

        Iterator(T** array, int bucket, int index): array(array), bucket(bucket), index(index) {}
        Iterator operator++() {
            if (index < BucketSize) {
                index++;
            } else {
                index = 0;
                bucket++;
            }
            return *this;
        }
        bool operator!=(const Iterator& other) const { return bucket != other.bucket && index != other.index; }
        T& operator*() const { return array[bucket][index]; }
    };

    inline Iterator begin() const {
        return Iterator(buckets.data, 0, 0);
    }

    inline Iterator end() const {
        return Iterator(buckets.data, buckets.size-1, topBucketSlotsUsed);
    }
};

MY_CLASS_END

#endif