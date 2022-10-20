#ifndef MY_STD_INCLUDED
#define MY_STD_INCLUDED

#include "MyInternals.hpp"

MY_CLASS_START

template<typename T>
void* memcpyT(T* dst, const T* src, size_t count) {
    return ::memcpy(dst, src, count * sizeof(T));
}

template<typename T>
void* memsetT(T* data, char value, size_t count) {
    return ::memset(data, value, count * sizeof(T));
}

MY_CLASS_END

#endif