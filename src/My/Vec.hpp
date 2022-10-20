#ifndef MY_VECTOR_INCLUDED
#define MY_VECTOR_INCLUDED

#include <assert.h>
#include "MyInternals.hpp"
#include "Array.hpp"
#include <initializer_list>
#include "std.hpp"

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
template<typename T>
struct Vec {
    static_assert(std::is_trivially_copyable<T>::value, "vec doesn't support complex types");
    using Type = T;

    T* data;
    int size;
    int capacity;

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
            data = (T*)MY_malloc(startCapacity * sizeof(T));
        else
            data = nullptr;
        size = 0;
        capacity = startCapacity;
    }

    inline static Vec<T> WithCapacity(int capacity) {
        return Vec<T>(capacity);
    }

    Vec(T* _data, int _size, int _capacity)
    : data(_data), size(_size), capacity(_capacity) {}

    static Vec<T> Empty() {
        return Vec<T>(nullptr, 0, 0);
    }

    /* Create a vector with the given size and copy the given data to the vector
     * 
     */
    Vec(const T* _data, int _size) {
        data = (T*)MY_malloc(_size * sizeof(T));
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
        assert(index < size    && "vector index out of bounds");
        assert(index > -1      && "vector index out of bounds");
        assert(data != nullptr && "access of null vector");
        return data[index];
    }

    inline bool push(const T& val) {
        /*
        if (reserveAtleast(size+1)) {
            memcpy(&data[size], &val, sizeof(T));
            return &data[size++];
        }
        return nullptr;
        */
        return vec_push((Generic::Vec*)this, sizeof(T), &val);
    }

    bool pushFront(const T* values, int count) {
        if (size + count > capacity) {
            // reallocate. Fortunately this makes the algorithm a lot simpler. just copy the new values in, then the old values
            int newCapacity = MAX(size + count, capacity*2);
            T* newData = (T*)malloc(newCapacity * sizeof(T));
            if (!newData) return false;
            memcpy(&newData[0], values, count * sizeof(T));
            memcpy(&newData[count], data, size * sizeof(T));
            free(data);
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
                memcpy(&data[elementsMoved], &data[size - elementsMoved - 1], batchSize * sizeof(T));
                elementsMoved += batchSize;
            }

            // copy new elements into now unused space at beginning
            memcpy(&data[0], values, count * sizeof(T));
            size += count;
        }
        return true;
    }

    inline bool pushFront(const T& val) {
        return pushFront(&val, 1);
    }

    void pop() {
        assert(size > 0 && "can't pop back element of empty vector");
        size--;
    }

    bool empty() const {
        return size == 0;
    }

    // Try to reserve to atleast minCapacity
    bool reserve(int newCapacity) {
        if (capacity < newCapacity) {
            return reallocate((capacity*2 > newCapacity) ? capacity*2 : newCapacity);
        }
        return false;
    }

    /* Resize the vector to the given new capacity. 
     * If the current size of the vector is greater than the new capacity, the size will reduced to the new capacity,
     * losing any elements past index 'newCapacity' - 1.
     * @return The capacity of the vector after resizing. If an error hasn't occured, this will be equal to 'newSize'.
     * When the new capacity couldn't be allocated, -1 will be returned
     */
    bool reallocate(int newCapacity) {
        T* newData = (T*)MY_realloc(data, newCapacity * sizeof(T));
        if (newData) {
            data = newData;
            size = (size < newCapacity) ? size : newCapacity;
            capacity = newCapacity;
            return true;
        }
        // failed to allocate
        return false;
    }

    void clear() {
        MY_free(data);
        size = 0;
        data = nullptr;
        capacity = 0;
    }

    void destroy() {
        MY_free(data);
    }

    inline T& back() const {
       assert(size > 0 && "can't get element of empty vector"); 
       return data[size-1];
    }

    inline T& front() const {
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

    inline void copyRange(T* srcBegin, T* srcEnd, T* dstBegin, T* dstEnd) {
        assert(dstEnd > dstBegin);
        assert(srcEnd > srcBegin);
        assert(srcEnd - srcBegin == dstEnd - dstBegin);
        memcpy(dstBegin, srcBegin, (srcEnd - srcBegin) * sizeof(T));
    }

    void insertAt(int index, const T* values, int count) {
        T* at = &data[index];
        if (size + count > capacity) {
            // reallocate. Fortunately this makes the algorithm a lot simpler. just copy the new values in, then the old values
            int newCapacity = MAX(size + count, capacity*2);
            T* newData = (T*)malloc(newCapacity * sizeof(T));
            if (!newData) return;
            // copy old data before index to range 0 - index
            memcpy(&newData[0], &data[0], index * sizeof(T));
            // copy values to range index - index + count
            memcpy(&newData[index], values, count * sizeof(T));
            // copy old data after index to range index + count -  size + count
            memcpy(&newData[index + count], &data[index], (size - index) * sizeof(T));
            /*
            copyRange(&data[index], &data[size], &newData[index + count], &newData[size + count]);
            std::copy(&newData[0], &newData[index], &data[0]); // 0 - index
            std::copy(values, &values[count], &newData[index]);
            std::copy(&data[0], &data[size], &newData[index])
            */
            MY_free(data);
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

static_assert(sizeof(Generic::Vec) == sizeof(Vec<int>), "GenericVec must have same binary layout as normal vec");

}

using Vector::Vec;

MY_CLASS_END

#endif