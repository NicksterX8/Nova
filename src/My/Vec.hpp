#ifndef MY_VECTOR_INCLUDED
#define MY_VECTOR_INCLUDED

#include <assert.h>
#include "MyInternals.hpp"
#include "Array.hpp"
#include <initializer_list>

MY_CLASS_START

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

    T* push(const T& val) {
        if (reserveAtleast(size+1)) {
            memcpy(&data[size], &val, sizeof(T));
            return &data[size++];
        }
        return nullptr;
    }

    inline void pop() {
        assert(size > 0 && "can't pop back element of empty vector");
        size--;
    }

    inline bool empty() const {
        return size == 0;
    }

    // Try to reserve to atleast minCapacity
    inline bool reserveAtleast(int minCapacity) {
        if (minCapacity > capacity)
            return reallocate((capacity*2 > minCapacity) ? capacity*2 : minCapacity);
        return true;
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

    bool push(T* elements, int count) {
        if (reserveAtleast(size + count)) {
            memcpy(&data[size], elements, count * sizeof(T));
            size += count;
            return true;
        }
        return false;
    }

    inline T* begin() const { return data; }
    inline T* end() const { return data + size; }
};

MY_CLASS_END

#endif