#include <stddef.h>

template<typename T, size_t Align>
struct alignas(Align) AlignedStruct : T {
    AlignedStruct(const T& val) : T(val) {}
};