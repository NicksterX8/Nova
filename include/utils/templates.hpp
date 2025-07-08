#ifndef UTILS_TEMPLATES_INCLUDED
#define UTILS_TEMPLATES_INCLUDED

#include <type_traits>
#include "utils/common-macros.hpp"

template<typename... Ts>
constexpr size_t sumSizes() {
    size_t sizes[] = {sizeof(Ts) ...};
    size_t size = 0;
    for (int i = 0; i < sizeof...(Ts); i++) {
        size += sizes[i];
    }
    return size;
}

/// @brief Defines the smallest standard unsigned integer type (uint8-uint64) that contains Size number of bytes
/// @tparam Size minimum number of bytes needed in the integer
template<size_t Size>
using SmallestUnsignedInteger = std::conditional_t<Size <= 4, std::conditional_t<Size <= 2, std::conditional_t<Size <= 1, std::conditional_t<Size == 0, void, uint8_t>, uint16_t>, uint32_t>, uint64_t>;


#endif