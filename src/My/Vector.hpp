#ifndef MY_VECTOR_INCLUDED
#define MY_VECTOR_INCLUDED

#include <assert.h>
#include "MyUtils.hpp"
#include "BasicIterator.hpp"
#include "Array.hpp"
#include <initializer_list>

MY_CLASS_START

/* A simple vector implementation for POD ONLY types!
 * Constructors and destructors will not be called!
 * Should be faster and smaller than std::vector most of the time, 
 * and result in faster compile times
 */
template<typename T>
struct Vector {
    T* data;
    int size;
    int capacity;

    using Type = T;

    // does not initialize data
    Vector() {
#ifdef DEBUG
        data = (T*)(1); // pointer that can't be null checked but should still crash on dereference
        size = 0x7ff8dead; // sentinel value that should mess everything up when used
        capacity = 0x7ff8dead; // sentinel value that should mess everything up when used
#endif
    }

    Vector(int startCapacity) {
        data = (T*)MY_malloc(startCapacity * sizeof(T));
        size = 0;
        capacity = startCapacity;
    }

    Vector(T* _data, int _size, int _capacity)
    : data(_data), size(_size), capacity(_capacity) {}

    /* Create a vector with the given size and copy the given data to the vector
     * 
     */
    Vector(const T* _data, int _size) {
        data = (T*)MY_malloc(_size * sizeof(T));
        memcpy(data, _data, _size * sizeof(T));
        size = _size;
        capacity = _size;
    }

    Vector(std::initializer_list<T> il) {
        *this = Vector(il.begin(), il.size());
    }

    inline T& operator[](int index) const {
        assert(index < size && "vector index out of bounds");
        assert(index > -1    && "vector index out of bounds");
        return data[index];
    }

    T* push_back(const T& val) {
        if (size >= capacity) {
            int increasedCapacity = capacity*2;
            // resize to atleast size+1 capacity. This is useful if for example capacity is 0,
            // resulting in increased capacity being 0 too, not big enough to hold the new element
            int newCapacity = resize((increasedCapacity > size+1) ? increasedCapacity : size+1);
            if (newCapacity == -1 // can't push back if unable to resize to fit the new element
            || size >= newCapacity) { // or if the capacity still isn't big enough, give up
                return nullptr;
            }
        }

        data[size] = val;
        return &data[size++];
    }

    void pop_back() {
        assert(size > 0 && "can't pop back element of empty vector");
        size--;
    }

    inline bool is_full() const {
        return size >= capacity;
    }

    inline bool is_empty() const {
        return size == 0;
    }

    /* Resize the vector, giving it capacity 'newSize'. 
     * @return The capacity of the vector after resizing. If an error hasn't occured, this will be equal to 'newSize'.
     * When malloc fails, -1 will be returned
     */
    int resize(int newCapacity) {
        int newSize = (size < newCapacity) ? size : newCapacity;
        // this check is basically unavoidable because malloc and realloc return null when passed 0 size,
        // and we need to know that isn't because we are out of memory
        if (newCapacity != 0) {
            T* newData = (T*)MY_realloc(data, newCapacity * sizeof(T));
            if (!newData) {
                newData = (T*)MY_malloc(newCapacity * sizeof(T));
                if (newData) {
                    memcpy(newData, data, newSize * sizeof(T));
                    MY_free(data);
                } else { // malloc and realloc both failed.
                    return -1;
                }
            }
            data = newData;
        } else {
            MY_free(data);
            data = nullptr;
        }
        
        size = newSize;
        capacity = newCapacity;
        return capacity;
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

        //int elementsToMove = size - index;
        for (int i = index; i < size-1; i++) {
            memcpy(&data[i], &data[i+1], sizeof(T));
        }

        size--;
    }

    Array<T> asArray() const {
        return Array<T>(size, data);
    }

    inline BasicIterator<T> begin() const { return BasicIterator<T>(data); }
    inline BasicIterator<T> end() const { return BasicIterator<T>(data + size); }
};

//template<typename T> inline BasicIterator<T> begin(const My::Vector<T>& vector) { return BasicIterator<T>(vector.data); }
//template<typename T> inline BasicIterator<T>   end(const My::Vector<T>& vector) { return BasicIterator<T>(vector.data + vector.size); }

MY_CLASS_END

#endif