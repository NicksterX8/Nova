#ifndef MY_BITSET_INCLUDED
#define MY_BITSET_INCLUDED

#include <stdint.h>
#include <stddef.h>
#include "MyInternals.hpp"

// result is undefined if x is 0
inline int highestBit(unsigned int x){
    return 31 - __builtin_clz(x);
}

// result is undefined if x is 0
inline int highestBit(unsigned long long x) {
    return 63 - __builtin_clzll(x);
}

template<class F>
void forEachSetBitUint(unsigned int bits, const F& functor) {
    while (bits != 0) {
        auto bitIndex = (unsigned int)highestBit(bits);
        functor(bitIndex);
        bits ^= (1U << bitIndex);
    }
}

template<class F>
void forEachSetBitULL(unsigned long long bits, const F& functor) {
    while (bits != 0) {
        auto bitIndex = (unsigned long long)highestBit(bits);
        functor(bitIndex);
        bits ^= (1ULL << bitIndex);
    }
}

template<typename IntegerT, class F>
void forEachSetBit(IntegerT bits, const F& functor) {
    static_assert(std::is_unsigned<IntegerT>::value, "Invalid type");
    if (sizeof(IntegerT) == sizeof(unsigned long long)) {
        forEachSetBitULL(bits, functor);
    } else if (sizeof(IntegerT) == sizeof(unsigned int)) {
        forEachSetBitUint(bits, functor);
    } else {
        for (IntegerT j = 0; bits != 0 && j < sizeof(IntegerT) * 8; j++) {
            if (bits & ((IntegerT)1 << j)) {
                functor(j);
                bits ^= ((IntegerT)1 << j);
            }
        }
    }
}

template<typename IntegerT, class F>
void forEachUnsetBit(IntegerT bits, const F& functor) {
    return forEachSetBit(~bits, functor);
}

MY_CLASS_START

template<size_t N, typename IntegerT = size_t>
struct Bitset {
    constexpr static size_t Size = N;
    constexpr static size_t IntegerSize = sizeof(IntegerT);
    constexpr static size_t IntegerBits = IntegerSize * CHAR_BIT;
    constexpr static size_t nInts  = (N/IntegerBits) + (N%IntegerBits != 0);
    constexpr static size_t nBytes = N/CHAR_BIT + (N%CHAR_BIT != 0);
    using IntegerType = IntegerT;
private:
    using Self = Bitset<N>;
public:
    static_assert(std::is_unsigned<IntegerT>::value, "Bitset integer types must be unsigned!");

    using SelfParamT = Self;

    IntegerT bits[nInts] = {0};

    constexpr Bitset() = default;

    constexpr Bitset(IntegerT startValue) {
        for (size_t i = 0; i < nInts; i++) {
            bits[i] = startValue;
        }
    }

    constexpr size_t size() const {
        return N;
    }

    constexpr size_t max_size() const {
        return UINT32_MAX;
    }

    constexpr bool get(uint32_t position) const {
        return bits[position/IntegerBits] & (1 << (position%IntegerBits));
    }    

    constexpr bool get(int32_t position) const {
        assert(position >= 0);
        return bits[position/IntegerBits] & (1 << (position%IntegerBits));
    }

    constexpr bool operator[](uint32_t position) const {
        return get(position);
    }

    constexpr bool operator[](int32_t position) const {
        return get(position);
    }

    constexpr void set(uint32_t position) {
        uint32_t position64 = position / IntegerBits;
        IntegerT digit = 1 << (position - (position64 * IntegerBits));
        bits[position64] |= digit;
    }

    constexpr void flipBit(uint32_t position) {
        uint32_t position64 = position / IntegerBits;
        IntegerT digit = 1 << (position - (position64 * IntegerBits));
        bits[position64] ^= digit;
    }

    constexpr void set(uint32_t position, bool value) {
        uint32_t position64 = position / IntegerBits;
        uint32_t modulus = (position - (position64 * IntegerBits));
        IntegerT digit = (uint64_t)value << (uint64_t)modulus;
        bits[position64] = (bits[position64] & ~((uint64_t)1 << (uint64_t)modulus)) | digit; // disabling part
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
        for (size_t i = 0; i < nInts; i++) {
            IntegerT n = bits[i];
            while (n != 0) {
                n = n & (n - 1);
                c++;
            }
        }
        
        return c;
    }

    constexpr bool hasAll(SelfParamT aBitset) const {
        return (*this & aBitset) == aBitset;
    }

    constexpr bool hasNone(SelfParamT aBitset) const {
        return !(*this & aBitset).any();
    }

    constexpr Self operator&(SelfParamT rhs) const {
        Self result;
        for (size_t i = 0; i < nInts; i++) {
            result.bits[i] = bits[i] & rhs.bits[i];
        }
        return result;
    }

    constexpr Self operator&(IntegerT rhs) const {
        Self result;
        result.bits[0] = bits[0] & rhs;
        for (size_t i = 1; i < nInts; i++) {
            result.bits[i] = 0;
        }
        return result;
    }

    constexpr Self& operator&=(SelfParamT rhs) {
        for (size_t i = 0; i < nInts; i++) {
            bits[i] &= rhs.bits[i];
        }
        return *this;
    }

    constexpr Self operator|(SelfParamT rhs) const {
        Self result;
        for (size_t i = 0; i < nInts; i++) {
            result.bits[i] = bits[i] | rhs.bits[i];
        }
        return result;
    }

    constexpr Self& operator|=(SelfParamT rhs) {
        for (size_t i = 0; i < nInts; i++) {
            bits[i] |= rhs.bits[i];
        }
        return *this;
    }

    #define MUT_METHOD_VERSION(type, method_call) const_cast(static_cast<const type*>(this)->method_call)

    constexpr Self operator^(SelfParamT rhs) const {
        Self result;
        for (size_t i = 0; i < nInts; i++) {
            result.bits[i] = bits[i] ^ rhs.bits[i];
        }
        return result;
    }

    constexpr Self& operator^=(SelfParamT rhs) {
        for (size_t i = 0; i < nInts; i++) {
            bits[i] ^= rhs.bits[i];
        }
        return *this;
    }

    constexpr bool operator==(SelfParamT rhs) const {
        for (size_t i = 0; i < nInts; i++) {
            if (bits[i] != rhs.bits[i]) {
                return false;
            }
        }
        return true;
    }

    constexpr bool operator!=(SelfParamT rhs) const {
        return !operator==(rhs);
    }

    constexpr Self operator~() const {
        Self result;
        for (size_t i = 0; i < nInts; i++) {
            result.bits[i] = ~bits[i];
        }
        return result;
    }

    template<class F>
    void forEachSet(const F& functor) const {
        for (size_t i = 0; i < nInts; i++)
            forEachSetBit<IntegerT, F>(bits[i], functor);
    }

    template<class F>
    void forEachUnset(const F& functor) const {
        for (size_t i = 0; i < nInts; i++)
            forEachUnsetBit<IntegerT, F>(bits[i], functor);
    }
};

MY_CLASS_END

#endif