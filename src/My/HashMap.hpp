#ifndef MY_HASH_MAP_INCLUDED
#define MY_HASH_MAP_INCLUDED

#include "MyInternals.hpp"
#include "BucketArray.hpp"
#include "Vec.hpp"

#define BITMASK_BOTTOM_62 0x3fffffffffffffffULL

MY_CLASS_START

using Hash = size_t;

template<class T>
struct TrivialHashT {
    Hash operator()(const T& val) {
        return val;
    }
};

template<typename K, typename V, typename H=TrivialHashT<K>>
struct HashMap {
    static_assert(std::is_trivially_copyable<K>::value, "hashmap doesn't support complex types");
    static_assert(std::is_trivially_copyable<V>::value, "hashmap doesn't support complex types");
private:
    using Self = HashMap<K, V, H>;
public:
    enum BState {
		Bucket_Empty=0, // This needs to be 0 so that memsetting 0 on buckets will make them empty
		Bucket_Filled=1,
		Bucket_Removed=2,
	};

    struct Bucket {
        // Steal a couple bits from the hash value to say whether the bucket
		// is empty, filled, or removed (different from empty)
		size_t	hash:62;
		size_t	state:2;
    };

    Bucket* buckets;
    K*      keys;
    V*      values;
    int     size;
    int     bucketCount;

    HashMap() = default;

    HashMap(int startBuckets) : size(0), bucketCount(startBuckets) {
        buckets = (Bucket*)MY_malloc(startBuckets * sizeof(Bucket));
        keys    =      (K*)MY_malloc(startBuckets * sizeof(K));
        values  =      (V*)MY_malloc(startBuckets * sizeof(V));
        // buckets states need to be set to Bucket_Empty which is 0, so set all buckets to 0
        memset(buckets, 0, startBuckets * sizeof(Bucket));
    }

    static Self WithBuckets(int buckets) {
        return HashMap(buckets);
    }

    Hash hashKey(const K& key) const {
        H hashFunction;
        return hashFunction(key);
    }

    V* lookup(const K& key) const {
        // have to do this before doing hash % size because x % 0 is undefined behavior
        if (size < 1) return nullptr;

        Hash hash = hashKey(key) & BITMASK_BOTTOM_62;
        int startBucketIndex = hash % bucketCount;
        
        // search buckets until we find an empty one
        V* val = nullptr;
        int i = startBucketIndex;
        int iEnd = bucketCount;
        loop:
        for (; i < iEnd; i++) {
            Bucket* b = &buckets[i];
            if (b->state == Bucket_Filled) {
                if (b->hash == hash && keys[i] == key) {
                    // found the value
                    return &values[i];
                }
            }
            if (b->state == Bucket_Empty) {
                // lookup failed, no value exists for the key
                return nullptr;
            }
        }

        // loop back to the begining to try the earlier buckets now
        if (i > 0) {
            i = 0;
            iEnd = startBucketIndex;
            goto loop;
        }

        // even after all that we couldn't find a value for the key
        return nullptr;
    }

    void insert(const K& key, const V& value) {
        // Resize larger if the load factor goes over 2/3
        if (size * 3 > bucketCount * 2 || size == 0) {
            int minNewSize = bucketCount * 2 > 0 ? bucketCount * 2 : 1;
            rehash(minNewSize);
        }

        // Hash the key and find the starting bucket
        Hash hash = hashKey(key) & BITMASK_BOTTOM_62;
        int bucketStart = 0;
        if (bucketCount != 0)
            bucketStart = hash % bucketCount;

        // Search for an unused bucket
        Bucket* bucketTarget = nullptr;
        int bucketTargetIndex = 0;
        int i = bucketStart, iEnd = bucketCount;
    loop:
        for (; i < iEnd; ++i) {
            Bucket* b = &buckets[i];
            if (b->state == Bucket_Filled) {
                if (b->hash == hash && keys[i] == key) {
                    // key already inserted. Just set new value
                    values[i] = value;
                    return;
                }
            }
            else if (b->state != Bucket_Filled) {
                bucketTarget = b;
                bucketTargetIndex = i;
                break;
            }
        }

        if (!bucketTarget && i > 0) {
            i = 0;
            iEnd = bucketStart;
            goto loop;
        }

        assert(bucketTarget && "should find bucket target");

        // Store the hash, key, and value in the bucket
        bucketTarget->hash = hash;
        bucketTarget->state = Bucket_Filled;
        keys[bucketTargetIndex] = key;
        values[bucketTargetIndex] = value;

        ++size;
    }

    // remove empty buckets to save memory
    void trim() {
        rehash(0);
    }

    void reserve(int maxSize) {
        rehash(maxSize * 3 / 2);
    }

    void rehash(int newBucketCount) {
        // Can't rehash down to smaller than current size or initial size
        newBucketCount = std::max(newBucketCount, size);

        // Build a new set of buckets, keys, and values
        Bucket* newBuckets = (Bucket*)MY_malloc(newBucketCount * sizeof(Bucket));
        K* newKeys         =      (K*)MY_malloc(newBucketCount * sizeof(K));
        V* newValues       =      (V*)MY_malloc(newBucketCount * sizeof(V));
        memset(newBuckets, 0, newBucketCount * sizeof(Bucket));

        // Walk through all the current elements and insert them into the new buckets
        for (int i = 0; i < bucketCount; ++i) {
            Bucket* b = &buckets[i];
            if (b->state != Bucket_Filled)
                continue;

            // Hash the key and find the starting bucket
            size_t hash = b->hash;
            int bucketStart = hash % newBucketCount;

            // Search for an unused bucket
            Bucket* bucketTarget = nullptr;
            size_t bucketTargetIndex = 0;
            for (int j = bucketStart; j < newBucketCount; j++) {
                Bucket* newBucket = &newBuckets[j];
                if (newBucket->state != Bucket_Filled) {
                    bucketTarget = newBucket;
                    bucketTargetIndex = j;
                    break;
                }
            }
            if (!bucketTarget) {
                for (int j = 0; j < bucketStart; j++) {
                    Bucket* newBucket = &newBuckets[j];
                    if (newBucket->state != Bucket_Filled) {
                        bucketTarget = newBucket;
                        bucketTargetIndex = j;
                        break;
                    }
                }
            }

            assert(bucketTarget);

            // Store the hash, key, and value in the bucket
            bucketTarget->hash = hash;
            bucketTarget->state = Bucket_Filled;
            newKeys[bucketTargetIndex] = keys[i];
            newValues[bucketTargetIndex] = values[i];
        }

        // Swap the new buckets, keys, and values into place
        MY_free(buckets);
        MY_free(keys);
        MY_free(values);

        buckets = newBuckets;
        keys    = newKeys;
        values  = newValues;

        bucketCount = newBucketCount;
    }

    // returns true on removal, false when failed to remove
    bool remove(const K& key) {
        if (bucketCount == 0) return false;
        // Hash the key and find the starting bucket
        size_t hash = hashKey(key) & BITMASK_BOTTOM_62;
        size_t bucketStartIndex = hash % bucketCount;

        // Search the buckets until we hit an empty one
        for (int i = bucketStartIndex; i < bucketCount; i++) {
            Bucket* b = &buckets[i];
            if (b->state == Bucket_Empty) {
                return false;
            }
            // bucket must be filled to remove it
            if (b->state == Bucket_Filled) {
                if (b->hash == hash && keys[i] == key) {
                    b->hash = 0;
                    b->state = Bucket_Removed;
                    --size;
                    return true;
                }
            }
        }
        for (int i = 0; i < bucketStartIndex; i++) {
            Bucket* b = &buckets[i];
            if (b->state == Bucket_Empty) {
                return false;
            }
            // bucket must be filled to remove it
            if (b->state == Bucket_Filled) {
                if (b->hash == hash && keys[i] == key) {
                    b->hash = 0;
                    b->state = Bucket_Removed;
                    --size;
                    return true;
                }
            }
        }

        return false;
    }

    void destroy() {
        MY_free(buckets); buckets = nullptr;
        MY_free(keys); keys = nullptr;
        MY_free(values); values = nullptr;
        size = 0;
        bucketCount = 0;
    }

    void iterateKeys() {
        for (int i = 0; i < bucketCount; i++) {
            if (buckets[i].state == Bucket_Filled) {
                // callback(keys[i])
            }
        }
    }


};

MY_CLASS_END

#endif