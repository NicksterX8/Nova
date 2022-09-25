#include <stdint.h>
#include <stddef.h>
#include "MyUtils.hpp"

MY_CLASS_START

template<size_t N>
struct Bitset {
public:
    constexpr static size_t nInts = (N/64) + ((N % 64) != 0);
    Uint64 bits[nInts] = {0};
    using Self = Bitset<N>;

    constexpr Bitset() {}

    constexpr Bitset(bool startValue) {
        for (size_t i = 0; i < nInts; i++) {
            if (startValue)
                bits[i] = (Uint64)(-1);
            else
                bits[i] = 0;
        }
    }

    constexpr size_t size() const {
        return N;
    }

    constexpr bool operator[](uint32_t position) const {
        return bits[position/64] & (1 << (position%64));
    }

    constexpr void set(uint32_t position) {
        if (position > size()) return;
        uint32_t position64 = position >> 6;
        uint64_t digit = (1 << (position - (position64 << 6)));
        bits[position64] |= digit;
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
};

MY_CLASS_END