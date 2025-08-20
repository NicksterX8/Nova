#ifndef MY_BITSET_INCLUDED
#define MY_BITSET_INCLUDED

#include <stdint.h>
#include <stddef.h>
#include "llvm/MathExtras.h"
#include <array>

template<typename IntegerT, class F>
void forEachSetBit(IntegerT bits, const F& functor) {
    static_assert(std::is_unsigned<IntegerT>::value, "Type must be unsigned!");
    while (bits) {
        int bitIndex = llvm::countTrailingZeros(bits, llvm::ZB_Undefined);
        functor(bitIndex);
        bits &= bits - 1;
    }
}

template<typename IntegerT, class F>
void forEachUnsetBit(IntegerT bits, const F& functor) {
    return forEachSetBit(~bits, functor);
}

template<typename IntegerT, class F>
void forEachSetBitmask(IntegerT bits, const F& functor) {
    static_assert(std::is_unsigned<IntegerT>::value, "Type must be unsigned!");
    while (bits) {
        IntegerT bitmask = bits & -bits;
        functor(bitmask);
        bits ^= bitmask;
    }
}

template<typename IntegerT, class F>
void forEachUnsetBitmask(IntegerT bits, const F& functor) {
    forEachSetBitmask(~bits, functor);
}

namespace My {

template<size_t N, typename IntegerT = size_t>
struct Bitset {
    constexpr static size_t Size = N;
    constexpr static size_t IntegerSize = sizeof(IntegerT);
    constexpr static size_t IntegerBits = IntegerSize * CHAR_BIT;
    constexpr static size_t nInts  = (N/IntegerBits) + (N%IntegerBits != 0);
    constexpr static size_t nBytes = N/CHAR_BIT + (N%CHAR_BIT != 0);
    using IntegerType = IntegerT;
private:
    using Self = Bitset<N, IntegerT>;
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
        uint32_t word = position / IntegerBits;
        IntegerT digit = (IntegerT)1 << (position - (word * IntegerBits));
        bits[word] |= digit;
    }

    constexpr void set(uint32_t position, bool value) {
        uint32_t word = position / IntegerBits;
        uint32_t modulus = (position - (word * IntegerBits));
        IntegerT digit = (IntegerT)value << modulus;
        bits[word] = (bits[word] & ~((IntegerT)1 << modulus)) | digit; // disabling part
    }

    constexpr void flipBit(uint32_t position) {
        uint32_t word = position / IntegerBits;
        IntegerT digit = (IntegerT)1 << (position - (word * IntegerBits));
        bits[word] ^= digit;
    }

    constexpr bool empty() const {
        for (size_t i = 0; i < nInts; i++) {
            if (bits[i])
                return false;
        }
        return true;
    }

    constexpr bool any() const {
        for (size_t i = 0; i < nInts; i++) {
            if (bits[i]) return true;
        }
        return false;
    }

    constexpr size_t count() const {
        // size_t c = 0;
        // for (size_t i = 0; i < nInts; i++) {
        //     IntegerT n = bits[i];
        //     while (n != 0) {
        //         n = n & (n - 1);
        //         c++;
        //     }
        // }
        
        // return c;

        size_t c = 0;
        for (size_t i = 0; i < nInts; i++) {
            c += llvm::countPopulation(bits[i]);
        }
        return c;
    }

    constexpr bool hasAll(SelfParamT aBitset) const {
        return (*this & aBitset) == aBitset;
    }

    constexpr bool hasAny(SelfParamT aBitset) const {
        return *this & aBitset;
    }

    constexpr bool hasNone(SelfParamT aBitset) const {
        return (*this & aBitset).empty();
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

    // if the bitset is all zeros, returns uintmax
    // @return the index of the highest set bit.
    unsigned int highestSet() const {
        for (int i = (int)nInts - 1; i >= 0; i--) {
            unsigned int distFromEnd = llvm::countLeadingZeros(bits[i]);
            if (distFromEnd != IntegerBits) 
                return N - 1 - distFromEnd + i * IntegerBits;
        }
        return (unsigned int)-1;
    }

     // if the bitset is all zeros, returns uintmax
    // @return the index of the lowest set bit.
    unsigned int lowestSet() const {
        for (int i = 0; i < (int)nInts; i++) {
            unsigned int distFromStart = llvm::countTrailingZeros(bits[i]);
            if (distFromStart != IntegerBits) 
                return distFromStart + i * IntegerBits;
        }
        return (unsigned int)-1;
    }

    // returns the lowest ([0]) and highest ([1]) set bit indices, or {uintmax, uintmax} if none are set
    std::array<unsigned int, 2> rangeSet() const {
        for (int i = (int)nInts - 1; i >= 0; i--) {
            IntegerT word = bits[i];
            unsigned int distFromEnd = llvm::countLeadingZeros(word);
            if (distFromEnd != IntegerBits) {
                for (int j = 0; j < i; j++) {
                    unsigned int distFromStart = llvm::countTrailingZeros(bits[j]);
                    if (distFromStart != IntegerBits) {
                        return {
                            distFromStart + i * IntegerBits,
                            N - 1 - distFromEnd + i * IntegerBits
                        };
                    }
                }
            }
        }
        return {(unsigned int)-1, (unsigned int)-1};
    }

    template<class F>
    void forEachSet(const F& functor) const {
        for (size_t i = 0; i < nInts; i++)
            forEachSetBit<IntegerT, F>(bits[i], functor);
    }

    template<class F>
    void forEachUnsetBit(const F& functor) const {
        for (size_t i = 0; i < nInts; i++)
            forEachUnsetBit<IntegerT, F>(bits[i], functor);
    }

    template<class F>
    void forEachSetBitmask(const F& functor) const {
        for (size_t i = 0; i < nInts; i++)
            forEachSetBitmask<IntegerT, F>(bits[i], functor);
    }

    template<class F>
    void forEachUnsetBitmask(const F& functor) const {
        for (size_t i = 0; i < nInts; i++)
            forEachUnsetBitmask<IntegerT, F>(bits[i], functor);
    }
};

}

#endif