#ifndef MY_HASH_MAP_INCLUDED
#define MY_HASH_MAP_INCLUDED

#include "MyInternals.hpp"
#include "BucketArray.hpp"
#include "Vec.hpp"
#include "std.hpp"
#include "utils/Metadata.hpp"

#define BITMASK_BOTTOM_62 0x3fffffffffffffffULL

MY_CLASS_START

namespace Map {

using Hash = size_t;

enum BucketState {
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


namespace Generic {

using HashFunction = Hash(const void*);
using EqualityFunction = bool(const void* lhs, const void* rhs);

struct HashMap {
    void* memory;
    int size;
    int bucketCount;
};

void* hashmap_lookup(const HashMap* map, int keySize, int valueSize, const void* key, HashFunction* hashFunction, EqualityFunction* keyEquals);

}

template<class T>
struct StdHashT {
    Hash operator()(const T& val) const {
        return std::hash<T>{}(val);
    }
};

//extern int collisionCount;
//extern int lookupCount;
//, typename KeyParamT = K, typename ValueParamT = V
template<typename K, typename V, typename H=StdHashT<K>>
struct HashMap {
    static_assert(std::is_trivially_copyable<K>::value, "hashmap doesn't support complex types");
    static_assert(std::is_trivially_copyable<V>::value, "hashmap doesn't support complex types");
private:
    using Self = HashMap<K, V, H>;
    using ValueParamT = FastestParamType<V>;
    using KeyParamT   = FastestParamType<K>;
public:

    void* memory;
    int   size;
    int   bucketCount;

    static Bucket* getBuckets(void* mem, int bucketCount) {
        return (Bucket*)mem;
    }
    static K* getKeys(void* mem, int bucketCount) {
        return (K*)((char*)mem + bucketCount * sizeof(Bucket));
    }
    static V* getValues(void* mem, int bucketCount) {
        return (V*)((char*)mem + bucketCount * (sizeof(Bucket) + sizeof(K)));
    }
    Bucket* buckets() const { return getBuckets(memory, bucketCount); }
    K*      keys()    const { return getKeys(memory, bucketCount); }
    V*      values()  const { return getValues(memory, bucketCount); }

    HashMap() = default;

    HashMap(int startBuckets) : size(0), bucketCount(startBuckets) {
        memory = MY_malloc(startBuckets * (sizeof(Bucket) + sizeof(K) + sizeof(V)));
        // buckets states need to be set to Bucket_Empty which is 0, so set all buckets to 0
        My::memsetT(buckets(), 0, startBuckets);
    }

    static Self Empty() {
        Self self;
        self.memory = nullptr;
        self.size = 0;
        self.bucketCount = 0;
        return self;
    }

    static Self WithBuckets(int buckets) {
        return HashMap(buckets);
    }

    static Hash hashKey(KeyParamT key) {
        static constexpr H hashKeyType;
        return hashKeyType(key); 
    }

    static Hash cHashKeyFunc(const void* key) {
        static constexpr H hashKeyType;
        return hashKeyType(*((K*)key));
    }

    V* lookup(KeyParamT key) const {
        //lookupCount++;
        // have to do this before doing hash % size because x % 0 is undefined behavior
        
        if (size < 1) {
            return nullptr;
        }

        Hash hash = hashKey(key) & BITMASK_BOTTOM_62;
        int bucketStart = (int)(hash % (unsigned)bucketCount);
        // search buckets until we find an empty one

        /*
        // quadratic probing
        for (int i = 0; i < bucketCount; i++) {
            int index = (h + i*i) % bucketCount;
            Bucket* b = &buckets()[index];
            if (b->state == Bucket_Filled) {
                if (b->hash == hash && keys()[index] == key) {
                    // found the value
                    return &values()[index];
                } else {
                    // Collision
                    collisionCount += 1;
                }
            }
        }

        //Failed to find filled bucket with key
        return nullptr;
        */
        
        // linear probing
        for (int i = bucketStart; i < bucketCount; i++) {
            Bucket* b = &buckets()[i];
            if (b->state == Bucket_Filled) {
                if (b->hash == hash && keys()[i] == key) {
                    // found the value
                    return &values()[i];
                }
            }
            else if (b->state == Bucket_Empty) {
                // lookup failed, no value exists for the key
                return nullptr;
            }
        }
        // loop back to the begining to try the earlier buckets now
        for (int i = 0; i < bucketStart; i++) {
            Bucket* b = &buckets()[i];
            if (b->state == Bucket_Filled) {
                if (b->hash == hash && keys()[i] == key) {
                    // found the value
                    return &values()[i];
                }
            }
            else if (b->state == Bucket_Empty) {
                // lookup failed, no value exists for the key
                return nullptr;
            }
        }
        

        // even after all that we couldn't find a value for the key
        return nullptr;
    }

    V* insert(KeyParamT key, ValueParamT value) {
        // Resize larger if the load factor goes over 2/3 or if bucket count is 0
        if (!(size * 3 < bucketCount * 2)) {
            int minNewSize = bucketCount * 2 > 1 ? bucketCount * 2 : 1;
            rehash(minNewSize);
        }

        // Hash the key and find the starting bucket
        Hash hash = hashKey(key) & BITMASK_BOTTOM_62;
        int bucketStart = (int)(hash % (unsigned)bucketCount);

        // Search for an unused bucket
        for (int i = bucketStart; i < bucketCount; ++i) {
            Bucket* b = &buckets()[i];
            if (b->state != Bucket_Filled) {
                // Store the hash, key, and value in the bucket
                b->hash = hash;
                b->state = Bucket_Filled;
                keys()[i] = key;
                values()[i] = value;

                ++size;
                return &values()[i];
            }
        }
        for (int i = 0; i < bucketStart; ++i) {
            Bucket* b = &buckets()[i];
            if (b->state != Bucket_Filled) {
                // Store the hash, key, and value in the bucket
                b->hash = hash;
                b->state = Bucket_Filled;
                keys()[i] = key;
                values()[i] = value;

                ++size;
                return &values()[i];
            }
        }

        assert(false && "unable to find bucket target");
        return nullptr;       
    }

    bool update(KeyParamT key, ValueParamT value) {
        // Resize larger if the load factor goes over 2/3 or if bucket count is 0
        if (!(size * 3 < bucketCount * 2)) {
            int minNewSize = bucketCount * 2 > 1 ? bucketCount * 2 : 1;
            rehash(minNewSize);
        }

        // Hash the key and find the starting bucket
        Hash hash = hashKey(key) & BITMASK_BOTTOM_62;
        int bucketStart = (int)(hash % (unsigned)bucketCount);

        // Search for an unused bucket
        for (int i = bucketStart; i < bucketCount; ++i) {
            Bucket* b = &buckets()[i];
            if (b->state == Bucket_Filled) {
                if (b->hash == hash && keys()[i] == key) {
                    // key already inserted. Just set new value
                    values()[i] = value;
                    return true;
                }
            }
            else if (b->state != Bucket_Filled) {
                // Store the hash, key, and value in the bucket
                b->hash = hash;
                b->state = Bucket_Filled;
                keys()[i] = key;
                values()[i] = value;

                ++size;
                return false;
            }
        }
        for (int i = 0; i < bucketStart; ++i) {
            Bucket* b = &buckets()[i];
            if (b->state == Bucket_Filled) {
                if (b->hash == hash && keys()[i] == key) {
                    // key already inserted. Just set new value
                    values()[i] = value;
                    return true;
                }
            }
            else if (b->state != Bucket_Filled) {
                // Store the hash, key, and value in the bucket
                b->hash = hash;
                b->state = Bucket_Filled;
                keys()[i] = key;
                values()[i] = value;

                ++size;
                return false;
            }
        }

        assert(false && "unable to find bucket target"); return false;    
    }

    // remove empty buckets to save memory
    void trim() {
        rehash(size);
    }

    bool reserve(int maxSize) {
        return rehash(maxSize * 3 / 2);
    }

    bool rehash(int newBucketCount) {
        // Can't rehash down to smaller than current size or initial size
        newBucketCount = std::max(newBucketCount, size);
        if (newBucketCount == 0) {
            return true;
        }

        // Build a new set of buckets, keys, and values
        void* newMemory = MY_malloc(newBucketCount * (sizeof(Bucket) + sizeof(K) + sizeof(V)));
        if (!newMemory) { return false; }
        Bucket* newBuckets = getBuckets(newMemory, newBucketCount);
        K* newKeys         =    getKeys(newMemory, newBucketCount);
        V* newValues       =  getValues(newMemory, newBucketCount);
        
        My::memsetT(newBuckets, 0, newBucketCount);

        // Walk through all the current elements and insert them into the new buckets
        for (int i = 0; i < bucketCount; ++i) {
            Bucket* b = &buckets()[i];
            if (b->state != Bucket_Filled)
                continue;

            // Hash the key and find the starting bucket
            size_t hash = b->hash;
            int bucketStart = hash % (unsigned)newBucketCount;

            int targetBucketIndex = 0;
            for (int j = bucketStart; j < newBucketCount; j++) {
                if (newBuckets[j].state != Bucket_Filled) {
                    targetBucketIndex = j;
                    goto foundBucket;
                }
            }
            for (int j = 0; j < bucketStart; j++) {
                if (newBuckets[j].state != Bucket_Filled) {
                    targetBucketIndex = j;
                    goto foundBucket;
                }
            }

            assert(false && "Failed to find empty bucket for rehash");

        foundBucket:
            // Store the hash, key, and value in the bucket
            Bucket* targetBucket = &newBuckets[targetBucketIndex];
            targetBucket->hash = hash;
            targetBucket->state = Bucket_Filled;
            newKeys[targetBucketIndex] = keys()[i];
            newValues[targetBucketIndex] = values()[i];
        }

        // Swap the new buckets, keys, and values into place
        MY_free(memory);
        memory = newMemory;

        bucketCount = newBucketCount;
        return true;
    }

    // returns true on removal, false when failed to remove
    bool remove(KeyParamT key) {
        if (bucketCount == 0) return false;
        // Hash the key and find the starting bucket
        size_t hash = hashKey(key) & BITMASK_BOTTOM_62;
        int bucketStart = (int)(hash % (unsigned)bucketCount);
        Bucket* buckets_ = buckets();
        K* keys_ = keys();

        // Search the buckets until we hit an empty one
        for (int i = bucketStart; i < bucketCount; i++) {
            Bucket* b = &buckets_[i];
            if (b->state == Bucket_Empty) {
                return false;
            }
            // bucket must be filled to remove it
            if (b->state == Bucket_Filled) {
                if (b->hash == hash && keys_[i] == key) {
                    b->hash = 0;
                    b->state = Bucket_Removed;
                    --size;
                    return true;
                }
            }
        }
        for (int i = 0; i < bucketStart; i++) {
            Bucket* b = &buckets_[i];
            if (b->state == Bucket_Empty) {
                return false;
            }
            // bucket must be filled to remove it
            if (b->state == Bucket_Filled) {
                if (b->hash == hash && keys_[i] == key) {
                    b->hash = 0;
                    b->state = Bucket_Removed;
                    --size;
                    return true;
                }
            }
        }

        return false;
    }

    bool contains(KeyParamT key) const {
        // have to do this before doing hash % size because x % 0 is undefined behavior
        if (size < 1) return false;

        Hash hash = hashKey(key) & BITMASK_BOTTOM_62;
        int bucketStart = (int)(hash % (unsigned)bucketCount);
        // search buckets until we find a full one
        for (int i = bucketStart; i < bucketCount; i++) {
            Bucket* b = &buckets()[i];
            if (b->state == Bucket_Filled) {
                if (b->hash == hash && keys()[i] == key) {
                    // found the value
                    return true;
                }
            }
            else if (b->state == Bucket_Empty) {
                // lookup failed, no value exists for the key
                return false;
            }
        }
        // loop back to the begining to try the earlier buckets now
        for (int i = 0; i < bucketStart; i++) {
            Bucket* b = &buckets()[i];
            if (b->state == Bucket_Filled) {
                if (b->hash == hash && keys()[i] == key) {
                    // found the value
                    return true;
                }
            }
            else if (b->state == Bucket_Empty) {
                // lookup failed, no value exists for the key
                return false;
            }
        }
        
        // even after all that we couldn't find a value for the key
        return false;
    }

    void clear() {
        memset(memory, 0, bucketCount * (sizeof(Bucket) + sizeof(K) + sizeof(V)));
        size = 0;
    }

    void destroy() {
        MY_free(memory);
        size = 0;
        bucketCount = 0;
    }
};

static_assert(sizeof(HashMap<int,int>) == sizeof(Generic::HashMap), "generic hashmap binary representation same as normal");

}

using Map::HashMap;

MY_CLASS_END

#endif