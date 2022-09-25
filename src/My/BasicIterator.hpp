#ifndef MY_BASIC_ITERATOR_INCLUDED
#define MY_BASIC_ITERATOR_INCLUDED

#include "MyUtils.hpp"

MY_CLASS_START

template<class T>
struct BasicIterator {
    BasicIterator(T* ptr): ptr(ptr){}
    BasicIterator operator++() { ++ptr; return *this; }
    bool operator!=(const BasicIterator& other) const { return ptr != other.ptr; }
    const T& operator*() const { return *ptr; }
    T& operator*() { return *ptr; }
    operator T*() { return ptr; };
    T* ptr;
};

MY_CLASS_END

#endif