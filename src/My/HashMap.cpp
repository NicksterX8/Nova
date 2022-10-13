#include "HashMap.hpp"

namespace My {
namespace Map {
namespace Generic {
/*
void* hashmap_lookup(const HashMap* map, int keySize, int valueSize, const void* key, HashFunction* hashFunction, EqualityFunction* keyEquals) {
    // have to do this before doing hash % size because x % 0 is undefined behavior
    if (map->size < 1) return nullptr;

    Hash hash = hashFunction(key) & BITMASK_BOTTOM_62;
    int bucketStart = hash % map->bucketCount;
    
    // search buckets until we find an empty one
    int i,iEnd;
    for (i = bucketStart,iEnd=map->bucketCount; i < iEnd; i++) {
        Bucket* b = &map->buckets[i];
        if (b->state == Bucket_Filled) {
            if (b->hash == hash && keyEquals(((char*)map->keys) + i * keySize, key)) {
                // found the value
                return ((char*)map->values) + i * valueSize;
            }
        }
        else if (b->state == Bucket_Empty) {
            // lookup failed, no value exists for the key
            return nullptr;
        }
    }
    // loop back to the begining to try the earlier buckets now
    for (i = 0,iEnd=bucketStart; i < iEnd; i++) {
        Bucket* b = &map->buckets[i];
        if (b->state == Bucket_Filled) {
            if (b->hash == hash && keyEquals(((char*)map->keys) + i * keySize, key)) {
                // found the value
                return ((char*)map->values) + i * valueSize;
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
*/

}
}
}