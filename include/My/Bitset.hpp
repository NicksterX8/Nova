#ifndef MY_BITSET_INCLUDED
#define MY_BITSET_INCLUDED

#include <stdint.h>
#include <stddef.h>
#include "llvm/MathExtras.h"
#include <array>

template<typename Word, class F>
void forEachSetBit(Word bits, const F& functor) {
    static_assert(std::is_unsigned<Word>::value, "Type must be unsigned!");
    while (bits) {
        int bitIndex = llvm::countTrailingZeros(bits, llvm::ZB_Undefined);
        functor(bitIndex);
        bits &= bits - 1;
    }
}

template<typename Word, class F>
void forEachUnsetBit(Word bits, const F& functor) {
    return forEachSetBit(~bits, functor);
}

template<typename Word, class F>
void forEachSetBitmask(Word bits, const F& functor) {
    static_assert(std::is_unsigned<Word>::value, "Type must be unsigned!");
    while (bits) {
        Word bitmask = bits & -bits;
        functor(bitmask);
        bits ^= bitmask;
    }
}

template<typename Word, class F>
void forEachUnsetBitmask(Word bits, const F& functor) {
    forEachSetBitmask(~bits, functor);
}

namespace My {

template<size_t N, typename WordType = size_t>
struct Bitset {
    using Word = WordType;
    static_assert(std::is_unsigned<Word>::value, "Bitset word type must be unsigned!");
    static_assert(N > 0, "Bitset cannot be zero sized");
    static_assert(N < INT_MAX, "Bitset too large!");

    constexpr static size_t Size = N;
    constexpr static size_t WordBits = sizeof(Word) * CHAR_BIT;
    constexpr static size_t WordCount  = (N/WordBits) + (N%WordBits != 0);

    constexpr static Word LastWordUsedBitmask = ((Word)1 << (N % WordBits)) - 1 | ((~0) * (N % WordBits == 0));
private:
    using Self = Bitset<N, Word>;
public:
    using SelfParamT = std::conditional_t<WordCount * sizeof(Word) <= 4 * sizeof(void*), Self, const Self&>;

    Word words[WordCount] = {0};

    constexpr Bitset() = default;

    constexpr Bitset(Word startValue) {
        words[0] = startValue;
    }

    static constexpr size_t size() {
        return N;
    }

    static constexpr size_t max_size() {
        return UINT_MAX;
    }

    constexpr bool get(unsigned int position) const {
        DASSERT(position < N);
        return words[position/WordBits] & ((Word)1 << (position%WordBits));
    }

    constexpr bool operator[](unsigned int position) const {
        DASSERT(position < N);
        return words[position/WordBits] & ((Word)1 << (position%WordBits));
    }

    constexpr void set(unsigned int position) {
        DASSERT(position < N);
        unsigned int word = position / WordBits;
        Word digit = (Word)1 << (position - (word * WordBits));
        words[word] |= digit;
    }

    constexpr void set(unsigned int position, bool value) {
        DASSERT(position < N);
        unsigned int word = position / WordBits;
        unsigned int modulus = (position - (word * WordBits));
        Word digit = (Word)value << modulus;
        words[word] = (words[word] & ~((Word)1 << modulus)) | digit; // disabling part
    }

    constexpr void flipBit(unsigned int position) {
        DASSERT(position < N);
        unsigned int word = position / WordBits;
        Word digit = (Word)1 << (position - (word * WordBits));
        words[word] ^= digit;
    }

    constexpr bool empty() const {
        for (size_t i = 0; i < WordCount; i++) {
            if (words[i])
                return false;
        }
        return true;
    }

    constexpr bool any() const {
        return !empty();
    }

    constexpr size_t count() const {
        size_t c = 0;
        for (size_t i = 0; i < WordCount; i++) {
            c += llvm::countPopulation(words[i]);
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
        for (size_t i = 0; i < WordCount; i++) {
            result.words[i] = words[i] & rhs.words[i];
        }
        return result;
    }

    // constexpr Self operator&(Word rhs) const {
    //     Self result;
    //     result.words[0] = words[0] & rhs;
    //     for (size_t i = 1; i < WordCount; i++) {
    //         result.words[i] = 0;
    //     }
    //     return result;
    // }

    constexpr Self& operator&=(SelfParamT rhs) {
        for (size_t i = 0; i < WordCount; i++) {
            words[i] &= rhs.words[i];
        }
        return *this;
    }

    constexpr Self operator|(SelfParamT rhs) const {
        Self result;
        for (size_t i = 0; i < WordCount; i++) {
            result.words[i] = words[i] | rhs.words[i];
        }
        return result;
    }

    constexpr Self& operator|=(SelfParamT rhs) {
        for (size_t i = 0; i < WordCount; i++) {
            words[i] |= rhs.words[i];
        }
        return *this;
    }
    
    constexpr Self operator^(SelfParamT rhs) const {
        Self result;
        for (size_t i = 0; i < WordCount; i++) {
            result.words[i] = words[i] ^ rhs.words[i];
        }
        return result;
    }

    constexpr Self& operator^=(SelfParamT rhs) {
        for (size_t i = 0; i < WordCount; i++) {
            words[i] ^= rhs.words[i];
        }
        return *this;
    }

    constexpr bool operator==(SelfParamT rhs) const {
        for (size_t i = 0; i < WordCount; i++) {
            if (words[i] != rhs.words[i]) {
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
        for (size_t i = 0; i < WordCount; i++) {
            result.words[i] = ~words[i];
        }
        if constexpr (N % WordBits != 0) {
            result.words[WordCount-1] &= LastWordUsedBitmask;
        }
        return result;
    }

    // if the bitset is all zeros, returns uintmax
    // @return the index of the highest set bit.
    unsigned int highestSet() const {
        for (int i = (int)WordCount - 1; i >= 0; i--) {
            unsigned int distFromEnd = llvm::countLeadingZeros(words[i]);
            if (distFromEnd != WordBits) 
                return N - 1 - distFromEnd + i * WordBits;
        }
        return (unsigned int)-1;
    }

     // if the bitset is all zeros, returns uintmax
    // @return the index of the lowest set bit.
    unsigned int lowestSet() const {
        for (int i = 0; i < (int)WordCount; i++) {
            unsigned int distFromStart = llvm::countTrailingZeros(words[i]);
            if (distFromStart != WordBits) 
                return distFromStart + i * WordBits;
        }
        return (unsigned int)-1;
    }

    // returns the lowest ([0]) and highest ([1]) set bit indices, or {uintmax, uintmax} if none are set
    std::array<unsigned int, 2> rangeSet() const {
        for (int i = (int)WordCount - 1; i >= 0; i--) {
            Word word = words[i];
            unsigned int distFromEnd = llvm::countLeadingZeros(word);
            if (distFromEnd != WordBits) {
                for (int j = 0; j < i; j++) {
                    unsigned int distFromStart = llvm::countTrailingZeros(words[j]);
                    if (distFromStart != WordBits) {
                        return {
                            distFromStart + i * WordBits,
                            N - 1 - distFromEnd + i * WordBits
                        };
                    }
                }
            }
        }
        return {(unsigned int)-1, (unsigned int)-1};
    }

    template<class F>
    void forEachSet(const F& functor) const {
        for (size_t i = 0; i < WordCount; i++)
            forEachSetBit<Word, F>(words[i], functor);
    }

    template<class F>
    void forEachUnsetBit(const F& functor) const {
        for (size_t i = 0; i < WordCount-1; i++)
            forEachSetBit(!words[i], functor);
        forEachSetBit(~(words[WordCount-1] & LastWordUsedBitmask), functor);
    }

    template<class F>
    void forEachSetBitmask(const F& functor) const {
        for (size_t i = 0; i < WordCount; i++)
            forEachSetBitmask<Word, F>(words[i], functor);
    }

    template<class F>
    void forEachUnsetBitmask(const F& functor) const {
        for (size_t i = 0; i < WordCount-1; i++)
            forEachSetBitmask(!words[i], functor);
        forEachSetBitmask(~(words[WordCount-1] & LastWordUsedBitmask), functor);
    }
};

}

#endif