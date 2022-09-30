#ifndef MY_ARRAY_INCLUDED
#define MY_ARRAY_INCLUDED

#include <array>
#include "MyInternals.hpp"
#include <initializer_list>

MY_CLASS_START

template<typename T>
struct Array {
    T* data;
    int size;

    // DOES NOT INITIALIZE ANYTHING
    Array() {
#ifdef DEBUG
        data = MY_SENTINEL_PTR(T);
        size = MY_SENTINEL_VAL;
#endif
    }

    Array(int _size) {
        size = _size;
        data = (T*)MY_malloc(size * sizeof(T));
    }

    Array(int _size, T* _data) {
        size = _size;
        data = _data;
    }

    Array(T* _data, int _size) {
        size = _size;
        data = _data;
    }

    static Array<T> from_ptr(int _size, const T* _data) {
        Array<T> array(_size);
        memcpy(array.data, _data, _size * sizeof(T));
        return array;
    }

    inline T& operator[](int index) const {
        assert(index < size && "array index out of bounds");
        assert(index > -1    && "array index out of bounds");
        return data[index];
    }

    inline void free() {
        MY_free(data);
        data = nullptr;
    }

    using Type = T;

    inline T* begin() const { return data; }
    inline T* end() const { return data + size; }
};

MY_CLASS_END

#endif