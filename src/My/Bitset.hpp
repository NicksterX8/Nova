#include <stdint.h>
#include <stddef.h>
#include "MyInternals.hpp"

MY_CLASS_START

template<size_t N>
struct Bitset {
public:
    constexpr static size_t nInts  = (N/64) + (N%64 != 0);
    constexpr static size_t nBytes = N/8 + (N%8 != 0);
    using Self = Bitset<N>;

    Uint64 bits[nInts] = {0};

    constexpr Bitset() {}

    constexpr Bitset(Uint64 startValue) {
        for (unsigned int i = 0; i < nInts; i++) {
            bits[i] = startValue;
        }
    }

    constexpr size_t size() const {
        return N;
    }

    constexpr bool operator[](uint32_t position) const {
        return bits[position/64] & (1 << (position%64));
    }

    constexpr void set(uint32_t position) {
        if (position >= size()) return;
        const uint32_t position64 = position >> 6;
        const uint64_t digit = (1 << (position - (position64 << 6)));
        bits[position64] |= digit;
    }

    constexpr void flipBit(uint32_t position) {
        if (position >= size()) return;
        uint32_t position64 = position >> 6;
        uint64_t digit = (1 << (position - (position64 << 6)));
        bits[position64] ^= digit;
    }

    constexpr void set(uint32_t position, bool value) {
        if (position >= size()) return;
        uint32_t position64 = position >> 6;
        uint32_t remainder = (position - (position64 << 6));
        uint64_t digit = (uint64_t)value << remainder;
        bits[position64] = (bits[position64] | digit) & digit; // disabling part
    }

    constexpr bool any() const {
        for (size_t i = 0; i < nInts; i++) {
            if (bits[i])
                return true;
        }
        return false;
    }

    constexpr size_t count() const {
        size_t c = 0;
        for (size_t i = 0; i < N; i++) {
            if (bits[i/64] & (1 << (i%64))) {
                c++;
            }
        }
        return c;
    }

    constexpr bool hasAll(Bitset aBitset) const {
        return (*this & aBitset) == aBitset;
    }

    constexpr bool hasNone(Bitset aBitset) const {
        return !(*this & aBitset).any();
    }

    constexpr Self operator&(Self rhs) const {
        Self result;
        for (size_t i = 0; i < nInts; i++) {
            result.bits[i] = bits[i] & rhs.bits[i];
        }
        return result;
    }

    constexpr Self& operator&=(Self rhs) {
        for (size_t i = 0; i < nInts; i++) {
            bits[i] &= rhs.bits[i];
        }
        return *this;
    }

    constexpr Self operator|(Self rhs) const {
        Self result;
        for (size_t i = 0; i < nInts; i++) {
            result.bits[i] = bits[i] | rhs.bits[i];
        }
        return result;
    }

    constexpr Self& operator|=(Self rhs) {
        for (size_t i = 0; i < nInts; i++) {
            bits[i] |= rhs.bits[i];
        }
        return *this;
    }

    constexpr Self operator^(Self rhs) const {
        Self result;
        for (size_t i = 0; i < nInts; i++) {
            result.bits[i] = bits[i] ^ rhs.bits[i];
        }
        return result;
    }

    constexpr Self& operator^=(Self rhs) {
        for (size_t i = 0; i < nInts; i++) {
            bits[i] ^= rhs.bits[i];
        }
        return *this;
    }

    constexpr bool operator==(Self rhs) const {
        for (size_t i = 0; i < nInts; i++) {
            if (bits[i] != rhs.bits[i]) {
                return false;
            }
        }
        return true;
    }

    constexpr bool operator!=(Self rhs) const {
        return !operator==(rhs);
    }

    constexpr Self operator~() const {
        Self result;
        for (size_t i = 0; i < nInts; i++) {
            result.bits[i] = ~bits[i];
        }
        return result;
    }
};

MY_CLASS_END