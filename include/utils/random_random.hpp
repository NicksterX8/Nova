#include <stddef.h>
#include "llvm/Alignment.h"

template<typename T, size_t Align>
struct alignas(Align) AlignedStruct : T {
    AlignedStruct(const T& val) : T(val) {}
};

template<typename... Ts>
struct PackedTypeBuffer {
    char buf[sumSizes<Ts...>()];

    void construct(size_t offset) {}

    template<typename T, typename... Rest>
    void construct(size_t offset, const T& val, const Rest&... rest) {
        memcpy(buf + offset, &val, sizeof(T));
        construct(offset + sizeof(T), rest...);
    }

    PackedTypeBuffer(const Ts&... vals) {
        construct(0, vals...);
    }
};

/// Returns a multiple of A needed to store `Size` bytes.
inline constexpr uint64_t alignTo(uint64_t size, size_t align) {
  // The following line is equivalent to `(Size + Value - 1) / Value * Value`.

  // The division followed by a multiplication can be thought of as a right
  // shift followed by a left shift which zeros out the extra bits produced in
  // the bump; `~(Value - 1)` is a mask where all those bits being zeroed out
  // are just zero.

  // Most compilers can generate this code but the pattern may be missed when
  // multiple functions gets inlined.
  return (size + align - 1) & ~(align - 1U);
}

/// Returns the offset to the next integer (mod 2**64) that is greater than
/// or equal to \p Value and is a multiple of \p Align.
inline constexpr uint64_t offsetToAlignment(uint64_t value, size_t alignment) {
  return alignTo(value, alignment) - value;
}

struct StructDimensions {
    size_t size;
    size_t align;
};

namespace detail {
    template<typename First>
    inline constexpr StructDimensions getStructDimensions(StructDimensions dimensions) {
        size_t alignOffset = (alignof(First) - (dimensions.size % alignof(First))) % alignof(First);
        dimensions.size += sizeof(First) + alignOffset;
        dimensions.align = MAX(dimensions.align, alignof(First));
        return dimensions;
    }

    template<typename First, typename Second, typename... Rest>
    inline constexpr StructDimensions getStructDimensions(StructDimensions dimensions) {
        return getStructDimensions<Second, Rest...>(getStructDimensions<First>(dimensions));
    }
}

template<typename... Ts>
inline constexpr StructDimensions getStructDimensions() {
    auto dimensions = detail::getStructDimensions<Ts...>({0, 0});
    return StructDimensions{alignTo(dimensions.size, dimensions.align), dimensions.align};
}

template<typename... Ts>
struct AlignedTypeBuffer {
private:
    static constexpr StructDimensions Dimensions = getStructDimensions<Ts...>();
public:
    alignas(Dimensions.align) char buffer[Dimensions.size];

    void construct(size_t offset) {}

    template<typename T, typename... Rest>
    void construct(size_t offset, const T& val, const Rest&... rest) {
        size_t alignOffset = offsetToAlignment(offset, alignof(T));
        memcpy(buffer + offset + alignOffset, &val, sizeof(T));
        construct(offset + sizeof(T) + alignOffset, rest...);
    }

    AlignedTypeBuffer(const Ts&... vals) {
        construct(0, vals...);
    }
};

static_assert(sizeof(AlignedTypeBuffer<int>) == sizeof(int), "");
static_assert(sizeof(AlignedTypeBuffer<int, int, float>) == 3 * sizeof(int), "");
static_assert(sizeof(AlignedTypeBuffer<int, double, float>) == 6 * sizeof(int), "");
static_assert(alignof(AlignedTypeBuffer<int, double, char>) == alignof(double), "");