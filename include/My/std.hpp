#ifndef MY_STD_INCLUDED
#define MY_STD_INCLUDED

#include "MyInternals.hpp"

namespace My {

template<typename T>
void* memcpyT(T* dst, const T* src, size_t count) {
    return ::memcpy(dst, src, count * sizeof(T));
}

template<typename T>
void* memsetT(T* data, char value, size_t count) {
    return ::memset(data, value, count * sizeof(T));
}

template<typename T>
void copyRange(const T* srcBegin, const T* srcEnd, T* dstBegin, T* dstEnd) {
    assert(dstEnd > dstBegin);
    assert(srcEnd > srcBegin);
    assert(srcEnd - srcBegin == dstEnd - dstBegin);
    memcpy(dstBegin, srcBegin, (srcEnd - srcBegin) * sizeof(T));
}

}

#endif