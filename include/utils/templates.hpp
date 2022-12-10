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

#endif