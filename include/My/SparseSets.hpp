#ifndef MY_SPARSE_SETS_INCLUDED
#define MY_SPARSE_SETS_INCLUDED

#include "MyInternals.hpp"
#include "Allocation.hpp"
#include "ADT/ArrayRef.hpp"

#include <stdint.h>
#include <stddef.h>
#include <limits.h>

MY_CLASS_START

template<size_t MinNumBits>
using SmallestUintT =
    std::conditional_t<MinNumBits <=  8, uint8_t,
    std::conditional_t<MinNumBits <= 16, uint16_t,
    std::conditional_t<MinNumBits <= 32, uint32_t,
    std::conditional_t<MinNumBits <= 64, uint64_t,
        void>>>>;

constexpr size_t constHighestBit(size_t num) {
    size_t highest = 0;
    for (size_t i = 0; i < sizeof(size_t) * CHAR_BIT; i++) {
        if (num & (1UL << i)) {
            highest = i;
        }
    }
    return highest;
}

constexpr size_t ceilLog2(size_t num) {
    size_t highest = constHighestBit(num);
    if (num - (1UL << highest) != 0)
        highest += 1;
    return highest;
}

template<size_t num>
using SmallestUintToHoldNum = SmallestUintT<ceilLog2(num)>;

template<typename Key, Key MaxKeyValue>
class GenericDenseSparseSet {
    static_assert(std::is_integral<Key>::value, "Key must be usable as index!");
    static_assert(MaxKeyValue > 0, "Max key value cannot be 0 or negative!");
    using Self = GenericDenseSparseSet<Key, MaxKeyValue>;
protected:
    using SizeT = int;
    using Index = SmallestUintToHoldNum<(size_t)MaxKeyValue+1>;
    static constexpr Index NullIndex = std::numeric_limits<Index>::max();

    SizeT valueSize; // the size of one element in the map
public:
    SizeT size; // number of elements in the map
protected:
    SizeT capacity; // number of elements allocated for in the map

    Index* set; // a set connecting keys to values | Index by key to get the index of the element corresponding to that key
    char* values; // a byte buffer for values

    
public:
    Key* keys; // keys corresponding to each value

    void* getValue(Index index) const {
        return &values[index * valueSize];
    }
protected:

    void assertValidKey(Key key) const {
        assert(key >= 0 && key <= MaxKeyValue && "DenseSparseSet key out of bounds!");
    }

    void emptySet() {
        for (Key key = 0; key <= MaxKeyValue; key++) {
            set[key] = NullIndex;
        }
    }

public:
    SizeT getValueSize() const {
        return valueSize;
    }

    GenericDenseSparseSet() = default;

    GenericDenseSparseSet(SizeT valueSize)
    : valueSize(valueSize), size(0), capacity(0), values(nullptr), keys(nullptr) {
        assert(valueSize >= 0 && "Size can't be negative!");
        set = Alloc<Index>(MaxKeyValue+1);
        emptySet();
    }

    GenericDenseSparseSet(SizeT valueSize, SizeT startCapacity)
    : valueSize(valueSize), size(0), capacity(startCapacity) {
        assert(valueSize >= 0 && "Size can't be negative!");
        assert(capacity >= 0  && "Capacity can't be negative!");
        values = Alloc<char>(startCapacity * valueSize);
        set    = Alloc<Index>(MaxKeyValue+1);
        keys   = Alloc<Key>(startCapacity);
        emptySet();
    }

    static Self New(SizeT valueSize) {
        return Self(valueSize);
    }

    void* lookup(Key key) const {
        assertValidKey(key);

        auto index = set[key];
        if (index != NullIndex)
            return getValue(index);
        else 
            return nullptr;
    }

    void insert(Key key, void* value) {
        assertValidKey(key);
        if (UNLIKELY(size >= capacity)) {
            reallocate(MAX(size+1, capacity*2));
        }

        const auto index = (Index)size;
        keys[index] = key;
        set[key] = index;
        if (value) {
            auto* valuePos = getValue(index);
            memcpy(valuePos, value, valueSize);
        }
        size++;
    }

    void insert(Key key) {
        assertValidKey(key);
        if (UNLIKELY(size >= capacity)) {
            reallocate(MAX(size+1, capacity*2));
        }

        const auto index = (Index)size;
        keys[index] = key;
        set[key] = index;
        size++;
    }

    void insert(ArrayRef<Key> keys) {
        for (auto key : keys) {
            assertValidKey(key);
        }

        if (UNLIKELY(size + keys.size() >= capacity)) {
            reallocate(MAX(size+keys.size(), capacity*2));
        }

        const auto startIndex = (Index)size;
        memcpy(keys + startIndex, keys, keys.size() * sizeof(Key));
        for (SizeT i = 0; i < keys.size(); i++) {
            set[keys[i]] = startIndex + i;
        }
        size += keys.size();
    }

    void insertRange(Key keyBegin, Key keyEnd) {
        assertValidKey(keyBegin);
        assertValidKey(keyEnd);
    
        SizeT keyCount = (SizeT)IABS(keyEnd - keyBegin);

        if (UNLIKELY(size + keyCount >= capacity)) {
            reallocate(MAX(size+keyCount, capacity*2));
        }

        const auto startIndex = (Index)size;
        memcpy(keys + startIndex, keys, keyCount * sizeof(Key));
        for (SizeT i = 0; i < keyCount; i++) {
            set[keyBegin + i] = startIndex + i;
        }
        size += keyCount;
    }

    void* require(Key key) {
        assertValidKey(key);
        if (UNLIKELY(size >= capacity)) {
            reallocate(MAX(size+1, capacity*2));
        }

        const auto index = (Index)size;
        keys[index] = key;
        set[key] = index;
        size++;
        return getValue(index);
    }

    void remove(Key key) {
        assertValidKey(key);
        assert(contains(key) && "Tried to remove nonexistent key!");
        assert(size >= 0 && "No elements to remove!");

        const auto indexOfRemoved = set[key];
        const auto topIndex = (Index)size-1;

        const auto topKey = keys[topIndex];
        // move key, value, and index over
        keys[indexOfRemoved] = topKey;
        if (size > 1) {
            memcpy(getValue(indexOfRemoved), getValue(topIndex), valueSize);
            set[topKey] = indexOfRemoved; // top value moved
        }
        set[key] = NullIndex; // mark removed key as gone
        size--;
    }

    bool contains(Key key) const {
        assertValidKey(key);
        return set[key] != NullIndex;
    }

    void reallocate(SizeT newCapacity) {
        assert(newCapacity >= 0 && "can't have negative map capacity");
        newCapacity = MAX(size, newCapacity); // capacity can't be smaller than size

        values = Realloc(values, (size_t)(newCapacity * valueSize));
        keys   = Realloc(keys, (size_t)newCapacity);
        
        capacity = newCapacity;
    }

    void destroy() {
        Free(values);
        Free(set);
        Free(keys);
    }
};

template<typename Key, typename Value, typename Index, Key MaxKeyValue>
struct DenseSparseSet {
    static_assert(std::is_integral<Key>::value, "Key must be usable as index!");
    static_assert(MaxKeyValue > 0, "Max key value cannot be 0 or negative!");
    using Self = DenseSparseSet<Key, Value, Index, MaxKeyValue>;
protected:
    using SizeT = int32_t;
    static constexpr size_t MaximumCapacity = std::numeric_limits<Index>::max();
    static constexpr Index NullIndex = std::numeric_limits<Index>::max();

    SizeT size; // number of elements in the map
    SizeT capacity; // number of elements allocated for in the map

    Index* set; // a set connecting keys to values | Index by key to get the index of the element corresponding to that key
    Value* values; // contigious array of values
    Key* keys; // contigious array of keys, parallel to values

    void assertValidKey(Key key) const {
        assert(key >= 0 && key <= MaxKeyValue && "DenseSparseSet key out of bounds!");
    }

    void emptySet() {
        for (Key key = 0; key <= MaxKeyValue; key++) {
            set[key] = NullIndex;
        }
    }
public:
    DenseSparseSet() = default;

    static Self Empty() {
        Self self;
        self.size = 0;
        self.capacity = 0;
        self.set = nullptr;
        self.values = nullptr;
        self.keys = nullptr;
        self.set = Alloc<Index>(MaxKeyValue+1);
        self.emptySet();
        return self;
    }

    static Self WithCapacity(SizeT startCapacity) {
        return Self(startCapacity);
    }

    DenseSparseSet(SizeT startCapacity)
    : size(0), capacity(startCapacity) {
        assert(capacity >= 0  && "Capacity can't be negative!");
        values = Alloc<Value>(startCapacity);
        set    = Alloc<Index>(MaxKeyValue+1);
        keys   = Alloc<Key>(startCapacity);
        emptySet();
    }

    Value* lookup(Key key) const {
        assertValidKey(key);

        auto index = set[key];
        if (index != NullIndex)
            return &values[index];
        else 
            return nullptr;
    }

    void insert(Key key, const Value& value) {
        assertValidKey(key);
        if (UNLIKELY(size + 1 > capacity)) {
            reallocate(MAX(size+1, capacity*2));
        }

        const auto index = (Index)size;
        keys[index] = key;
        assert(set[key] == NullIndex && "Attempted to insert key already present in sparse set");
        set[key] = index;
        // memcpy(&values[index], &value, sizeof(Value));
        values[index] = value;
        size++;
    }

    Value* insert(Key key) {
        assertValidKey(key);
        if (UNLIKELY(size + 1 > capacity)) {
            reallocate(MAX(size+1, capacity*2));
        }

        const auto index = (Index)size;
        keys[index] = key;
        assert(set[key] == NullIndex && "Attempted to insert key already present in sparse set");
        set[key] = index;
        size++;
        return &values[index];
    }

    // returns the beginning of the range of values for the keys
    Value* insertList(ArrayRef<Key> newKeys) {
        for (auto key : newKeys) {
            assertValidKey(key);
        }

        if (UNLIKELY(size + newKeys.size() > capacity)) {
            reallocate(MAX(size+newKeys.size(), capacity*2));
        }

        const auto startIndex = (Index)size;
        Value* newValues = &values[startIndex];
        memcpy(&keys[startIndex], newKeys.data(), newKeys.size() * sizeof(Key));
        for (SizeT i = 0; i < newKeys.size(); i++) {
            assert(set[newKeys[i]] == NullIndex && "Attempted to insert key already present in sparse set");
            set[newKeys[i]] = startIndex + i;
        }
        size += newKeys.size();
        return newValues;
    }

    void insertRange(Key keyBegin, Key keyEnd) {
        assertValidKey(keyBegin);
        assertValidKey(keyEnd);
    
        SizeT count = (SizeT)IABS(keyEnd - keyBegin);

        if (UNLIKELY(size + count > capacity)) {
            reallocate(MAX(size+count, capacity*2));
        }

        const auto startIndex = (Index)size;
        memcpy(keys + startIndex, keys, count * sizeof(Key));
        for (SizeT i = 0; i < count; i++) {
            assert(set[keyBegin + i] == NullIndex && "Attempted to insert key already present in sparse set");
            set[keyBegin + i] = startIndex + i;
        }
        size += count;
    }

    void remove(Key key) {
        assertValidKey(key);

        const auto indexOfRemoved = set[key];
        const auto topIndex = (Index)size-1;
        const auto topKey = keys[topIndex];
        // move key, value, and index over
        keys[indexOfRemoved] = topKey;
        memcpy(&values[indexOfRemoved], &values[topIndex], sizeof(Value));
        set[topKey] = indexOfRemoved;
        assert(set[key] != NullIndex && "Attempted to remove key not present in sparse set");
        set[key] = NullIndex; // mark removed key as gone
        size--;
    }

    bool contains(Key key) const {
        return key >= 0 && key <= MaxKeyValue && set[key] != NullIndex;
    }

    void reallocate(SizeT newCapacity) {
        assert(newCapacity >= 0 && "can't have negative map capacity");
        newCapacity = MAX(size, newCapacity); // capacity can't be smaller than size

        values = Realloc(values, (size_t)newCapacity);
        keys   = Realloc(keys, (size_t)newCapacity);
        
        capacity = newCapacity;
    }

    int getSize() const {
        return size;
    }

    Value* getValueArray() const {
        return values;
    }

    const Key* getKeyArray() const {
        return keys;
    }

    void destroy() {
        Free(values);
        Free(set);
        Free(keys);
    }
};

MY_CLASS_END

#endif