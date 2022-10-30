#ifndef MY_VECTOR_INCLUDED
#define MY_VECTOR_INCLUDED

#include <assert.h>
#include "MyInternals.hpp"
#include "Array.hpp"
#include <initializer_list>
#include "std.hpp"
#include "../constants.hpp"
#include "Allocation.hpp"

MY_CLASS_START

namespace Vector {

namespace Generic {
    struct Vec {
        void* data;
        int size;
        int capacity;
    };

    bool vec_push(Vec* vec, int typeSize, const void* elements, int elementCount=1);
}

/* A simple vector implementation for POD ONLY types!
 * Constructors and destructors will not be called!
 * Should be faster and smaller than std::vector most of the time, 
 * and result in faster compile times
 */ 
template<typename T, class Allocator = DefaultAllocator>
struct Vec {
    static_assert(std::is_trivially_copyable<T>::value, "vec doesn't support complex types");
    using Type = T;

    T* data;
    int size;
    int capacity;

    static Allocator allocator;

private: 
    using Self = Vec<T, Allocator>;
    using ValueParamT = FastestParamType<T>;
public:

    // does not initialize data
    Vec() {
#ifdef DEBUG
        data = (T*)(1); // pointer that can't be null checked but should still crash on dereference
        size = 0x7ff8dead; // sentinel value that should mess everything up when used
        capacity = 0x7ff8dead; // sentinel value that should mess everything up when used
#endif
    }

    Vec(int startCapacity) {
        if (startCapacity > 0)
            data = (T*)allocator.Alloc((size_t)startCapacity * sizeof(T));
        else
            data = nullptr;
        size = 0;
        capacity = startCapacity;
    }

    inline static Self WithCapacity(int capacity) {
        return Self(capacity);
    }

    Vec(T* _data, int _size, int _capacity)
    : data(_data), size(_size), capacity(_capacity) {}

    static Self Empty() {
        return Self(nullptr, 0, 0);
    }

    /* Create a vector with the given size and copy the given data to the vector
     * 
     */
    Vec(const T* _data, int _size) {
        data = (T*)allocator.Alloc(_size * sizeof(T));
        memcpy(data, _data, _size * sizeof(T));
        size = _size;
        capacity = _size;
    }

    inline T* get(int index) const {
        if (data && index < size && index > -1) {
            return &data[index];
        }
        return nullptr;
    }

    inline T& operator[](int index) const {
#ifdef BOUNDS_CHECKS
        assert(index < size    && "vector index out of bounds");
        assert(index > -1      && "vector index out of bounds");
#endif
        assert(data != nullptr && "access of null vector");
        return data[index];
    }

    inline void push(ValueParamT val) {
        /*
        if (reserveAtleast(size+1)) {
            memcpy(&data[size], &val, sizeof(T));
            return &data[size++];
        }
        return nullptr;
        */
        //return vec_push((Generic::Vec*)this, sizeof(T), &val);
        if (size+1 > vec->capacity) {
            auto newData = ReallocMin<T>(vec->data, size+1, capacity*2);
            data = newData.ptr;
            capacity = newData.size;
        }
        memcpy(vec->data + vec->size, &val, sizeof(T));
        ++size;
    }

    void pushFront(const T* values, int count) {
        assert(count > 0 && "can't push 0 or less values!");
        if (size + count > capacity) {
            // reallocate. Fortunately this makes the algorithm a lot simpler. just copy the new values in, then the old values
            int newCapacity = MAX(size + count, capacity*2);
            T* newData = (T*)allocator.Alloc((size_t)newCapacity * sizeof(T));
            memcpy(&newData[0], values, (size_t)count * sizeof(T));
            memcpy(&newData[count], data, (size_t)size * sizeof(T));
            allocator.Free(data);
            data = newData;
            size += count;
            capacity = newCapacity;
        } else {
            // no allocation version, a bit more complicated
            // move all current elements forward at 'count' sized batches
            int elementsMoved = 0;
            while (elementsMoved < size) {
                int elementsLeftToMove = size - elementsMoved;
                int batchSize = elementsLeftToMove < count ? elementsLeftToMove : count;
                memcpy(&data[elementsMoved], &data[size - elementsMoved - 1], (size_t)batchSize * sizeof(T));
                elementsMoved += batchSize;
            }

            // copy new elements into now unused space at beginning
            memcpy(&data[0], values, (size_t)count * sizeof(T));
            size += count;
        }
    }

    void pushFront(ValueParamT val) {
        pushFront(&val, 1);
    }

    void pop() {
        assert(size > 0 && "can't pop back element of empty vector");
        size--;
    }

    bool empty() const {
        return size == 0;
    }

    // reserve atleast capacity
    // \returns the new capacity of the vec
    int reserve(int capacity) {
        if (this->capacity < capacity) {
            return reallocate(capacity, this->capacity*2);
        }
        return this->capacity;
    }

    /* TODO: comment this */
    int reallocate(int min, int preferred) {
        MemoryBlockT<T> block = allocator.ReallocMin(data, min, preferred);
        data = block.ptr;
        size = (size < newCapacity) ? size : newCapacity;
        capacity = (int)block.size;
        return capacity;
    }

    /* Resize the vector to the given new capacity. 
     * If the current size of the vector is greater than the new capacity, the size will reduced to the new capacity,
     * losing any elements past index 'newCapacity' - 1.
     * @return The capacity of the vector after resizing. If an error hasn't occured, this will be equal to 'newSize'.
     * When the new capacity couldn't be allocated, -1 will be returned
     */
    void reallocate(int newCapacity) {
        T* newData = (T*)allocator.Realloc(data, newCapacity * sizeof(T));
        data = newData;
        size = (size < newCapacity) ? size : newCapacity;
        capacity = newCapacity;
    }

    void clear() {
        allocator.Free(data);
        size = 0;
        data = nullptr;
        capacity = 0;
    }

    void destroy() {
        allocator.Free(data);
    }

    T& back() const {
       assert(size > 0 && "can't get element of empty vector"); 
       return data[size-1];
    }

    T& front() const {
       assert(size > 0 && "can't get element of empty vector"); 
       return data[0];
    }

    void remove(int index) {
        assert(index < size && "vector index out of bounds");
        assert(index > -1    && "vector index out of bounds");

        for (int i = index; i < size-1; i++) {
            memcpy(&data[i], &data[i+1], sizeof(T));
        }

        size--;
    }

    bool push(const T* elements, int count) {
        /*
        if (reserveAtleast(size + count)) {
            memcpy(&data[size], elements, count * sizeof(T));
            size += count;
            return true;
        }
        return false;
        */
        return Generic::vec_push((Generic::Vec*)this, sizeof(T), elements, count);
    }

    void insertAt(int index, const T* values, int count) {
        T* at = &data[index];
        if (size + count > capacity) {
            // reallocate. Fortunately this makes the algorithm a lot simpler. just copy the new values in, then the old values
            int newCapacity = MAX(size + count, capacity*2);
            T* newData = (T*)allocator.Alloc(newCapacity * sizeof(T));
            // copy old data before index to range 0 - index
            memcpy(&newData[0], &data[0], index * sizeof(T));
            // copy values to range index - index + count
            memcpy(&newData[index], values, count * sizeof(T));
            // copy old data after index to range index + count -  size + count
            memcpy(&newData[index + count], &data[index], (size - index) * sizeof(T));

            allocator.Free(data);
            data = newData;
            size += count;
            capacity = newCapacity;
        } else {
            // no allocation version, a bit more complicated
            // move all current elements forward at 'count' sized batches
            int elementsMoved = 0;
            while (elementsMoved < size) {
                int elementsLeftToMove = size - elementsMoved;
                int batchSize = elementsLeftToMove < count ? elementsLeftToMove : count;
                memcpy(&at[elementsMoved], &at[size - elementsMoved - 1], batchSize * sizeof(T));
                elementsMoved += batchSize;
            }

            // copy new elements into now unused space at beginning
            memcpy(&at[0], values, count * sizeof(T));
        }
    }

    template<typename T2>
    Vec<T2> cast() const {
        return Vec<T2>(data, size, capacity);
    }

    using iterator = T*; // no iterator bs, just pointer

    inline T* begin() const { return data; }
    inline T* end() const { return data + size; }
};

template<typename T, class Allocator>
Allocator Vec<T, Allocator>::allocator = Allocator();

static_assert(sizeof(Generic::Vec) == sizeof(Vec<int>), "GenericVec must have same binary layout as normal vec");

template<typename T, int N = 0>
struct SmallVector {
    T* data;
    int size;
    int capacity;
    char buffer[N * sizeof(T)];

    T& operator[](int index) {
        return data[index];
    }
};

}

using Vector::Vec;

MY_CLASS_END

#endif