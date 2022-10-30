#ifndef MY_ARRAY_INCLUDED
#define MY_ARRAY_INCLUDED

#include <array>
#include "MyInternals.hpp"
#include <initializer_list>

MY_CLASS_START

template<typename T>
struct DynArray {
    T* data;
    int size;

private:
    using Self = DynArray<T>;
public:

    static Self WithSize(int size) {
        return Self{
            .data = (T*)MY_malloc(size * sizeof(T)),
            .size = size
        };
    }

    static Self Duplicate(const T* data, int size) {
        Self self = WithSize(size);
        memcpy(self.data, data, size * sizeof(T));
        return self;
    }

    inline T& operator[](int index) const {
#ifdef BOUNDS_CHECKS
        assert(index < size && "array index out of bounds");
        assert(index > -1    && "array index out of bounds");
#endif
        return data[index];
    }

    inline void free() {
        MY_free(data);
    }

    T& front() const {
#ifdef BOUNDS_CHECKS
       assert(size > 0 && "can't get element of empty vector");
#endif
       return data[0];
    }

    T& back() const {
#ifdef BOUNDS_CHECKS
       assert(size > 0 && "can't get element of empty vector"); 
#endif
       return data[size-1];
    }

    using Type = T;
    
    using iterator = T*;
    inline T* begin() const { return data; }
    inline T* end() const { return data + size; }
};

MY_CLASS_END

#endif