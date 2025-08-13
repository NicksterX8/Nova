#ifndef MY_INTERNALS_INCLUDED
#define MY_INTERNALS_INCLUDED

#include "memory/memory.hpp"

namespace My {

template<typename T, size_t Size = sizeof(T)>
using FastestParamType = typename std::conditional<(Size <= 2 * sizeof(void*)) && std::is_trivially_copyable<T>::value, T, const T&>::type;

}

#endif