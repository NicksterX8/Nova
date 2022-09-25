#ifndef MY_ARRAY_INCLUDED
#define MY_ARRAY_INCLUDED

#include <array>
#include "MyUtils.hpp"
#include "BasicIterator.hpp"
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
        data = MY_malloc(size * sizeof(T));
    }

    Array(int _size, T* _data) {
        size = _size;
        data = _data;
    }

    static Array<T> from_ptr(int _size, const T* _data) {
        Array<T> array(_size);
        memcpy(data, _data, size * sizeof(T));
    }

    Array(const std::initializer_list<T>& il) {
        *this = Array(il.size(), il.begin());
    }

    inline T& operator[](int index) const {
        assert(index < size && "array index out of bounds");
        assert(index > -1    && "array index out of bounds");
        return data[index];
    }

    void free() {
        MY_free(data);
    }

    using Type = T;

    inline BasicIterator<T> begin() const { return BasicIterator<T>(data); }
    inline BasicIterator<T> end() const { return BasicIterator<T>(data + size); }
};

inline int takes(std::array<int, 2> arr) {

}

inline int tester() {
    std::array<int, 3> stdArray{0,1,2};
    My::Array<int> myArray{0,1,2};

    takes({1,2});

}

MY_CLASS_END

#endif