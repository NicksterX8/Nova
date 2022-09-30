#ifndef MY_BASIC_ITERATOR_INCLUDED
#define MY_BASIC_ITERATOR_INCLUDED

#include "MyInternals.hpp"

MY_CLASS_START

// I think useless

template<class T>
struct BasicIterator {
    T* ptr;
    BasicIterator(T* ptr): ptr(ptr){}
    BasicIterator operator++() { ++ptr; return *this; }
    bool operator!=(const BasicIterator& other) const { return ptr != other.ptr; }
    T& operator*() const { return *ptr; }
    operator T*() { return ptr; };
};

MY_CLASS_END

#endif