#ifndef VAL_PTR_UNION_INCLUDED
#define VAL_PTR_UNION_INCLUDED

#include <stdint.h>
#include <stddef.h>
#include "llvm/PointerIntPair.h"
#include "utils/systemInfo.hpp"

// template<typename T, typename PtrT>
// class ValPtrUnion {
//     using PtrIntPair = llvm::PointerIntPair<PtrT, 1>;
//     static_assert(sizeof(T) < sizeof(PtrT), "T must be smaller than a pointer to fit in the union!");

//     // llvm::PointerIntPair uses low bits of the pointer to store the int
//     // make sure some low bits are open for it to use by using an offset into the pointer
//     // into the high bits of the pointer
//     static constexpr size_t TOffset = SystemIsLittleEndian ? (sizeof(PtrIntPair) - sizeof(T)) : 0;
//     static constexpr bool NeedPadding = TOffset > 0;
//     struct PaddedT {
//         std::array<char, TOffset> padding;
//         T value;
//     };
//     struct UnpaddedT {
//         T value;
//     };
//     // if on a big endian system, we need to add padding to put the value in the high bits
//     // so PointerIntPair can use the low bits for the int
//     using PairType = std::conditional_t<NeedPadding, PaddedT, UnpaddedT>;
//     union {
//         PairType tvalue;
//         PtrIntPair ptrAndDiscriminant;
//     };
//     enum class State {
//         Value = 0,
//         Pointer = 1
//     };

// public:
//     ValPtrUnion(T value) {
//         setValue(value);
//     }

//     ValPtrUnion(PtrT pointer) {
//         setPointer(pointer);
//     }

//     bool isPointer() const {
//         // always first check if value is the sentinel value

//         return ptrAndDiscriminant.getInt() == State::Pointer;
//     }

//     bool isValue() const {
//         return ptrAndDiscriminant.getInt() == State::Value;
//     }

//     // pointer is overwritten if it exists
//     void setValue(T value) {
//         tvalue.value = value;
//         ptrAndDiscriminant.setInt(State::Value);
//     }

//     // value is overwritten if it exists
//     void setPointer(PtrT pointer) {
//         ptrAndDiscriminant.setPointerAndInt(pointer, State::Pointer);
//     }

//     T& getValue() {
//         assert(isValue());
//         return tvalue.value;
//     }

//     const T& getValue() const {
//         assert(isValue());
//         return tvalue.value;
//     }
// };



template<typename T, typename PtrT, uint8_t UnusedBitIndex = sizeof(T) * 8, bool UnusedBitState = 0, bool AssertBitUnused = false>
struct ValPtrUnionFull {
    using Me = ValPtrUnionFull<T, PtrT, UnusedBitIndex>;
    static constexpr bool UseAllBits = sizeof(T) >= sizeof(PtrT);
    using PtrIntPair = llvm::PointerIntPair<PtrT, 1>;

    static constexpr bool AssertSentinelBitCorrect = AssertBitUnused;

    static constexpr bool SentinalStateValue = UnusedBitState;
    static constexpr bool SentinalStatePointer = !UnusedBitState;

    static constexpr uint8_t SentinelBitIndex = UnusedBitIndex;
    using Word = uintptr_t;
    static constexpr uint8_t WordBits = sizeof(Word) * 8;
    static constexpr uint8_t NumWords = (sizeof(T) + sizeof(Word) - 1) / sizeof(Word); // ceil(sizeof(T) / sizeof(Word))
    using BitsT = std::array<Word, NumWords>;
    static_assert(UnusedBitIndex < sizeof(BitsT) * 8, "Set a bit index within the range of T in template arguments");
    
    static constexpr uint8_t SentinelWord = SentinelBitIndex / WordBits;
    static constexpr uint8_t SentinelBitShift = SentinelBitIndex % WordBits;
    static constexpr Word SentinelBitMask = (Word)1 << SentinelBitShift;

    union Value {
        T tvalue;
        PtrIntPair ptrAndDiscriminant;

        Value() {
            setPointer(nullptr);
        }

        Value(PtrT pointer) {
            setPointer(pointer);
        }

        Value(T value) {
            setValue(value);
        }

        BitsT toBits() const {
            BitsT bits;
            memcpy(&bits, this, sizeof(bits));
            return bits;
        }

        void setBits(BitsT bits) {
            memcpy(this, &bits, sizeof(bits));
        }

        static Value fromBits(BitsT bits) {
            Value v;
            memcpy(&v, &bits, sizeof(bits));
            return v;
        }

        bool getSentinelValue() const {
            BitsT bits = toBits();
            return bits[SentinelWord] & SentinelBitMask;
        }

        void setSentinelValue(bool value) {
            BitsT bits = toBits();
            bits[SentinelWord] = (bits[SentinelWord] & ~SentinelBitMask) | ((Word)value << SentinelBitShift);
            setBits(bits);
        }

        void setPointer(PtrT pointer) {
            ptrAndDiscriminant.setPointer(pointer);

            bool sentinel = getSentinelValue(); // get bit where sentinel is
            // store it in discriminant
            ptrAndDiscriminant.setInt(sentinel);
            // then set sentinel to show it's not a value
            setSentinelValue(SentinalStatePointer);
        }

        // pointer is overwritten if it exists
        void setValue(T value) {
            tvalue = value;
        }
    } u;

    static_assert(sizeof(BitsT) == sizeof(Value));

public:
    ValPtrUnionFull() {
        setPointer(nullptr);
    }

    ValPtrUnionFull(T value) {
        setValue(value);
    }

    ValPtrUnionFull(PtrT pointer) {
        setPointer(pointer);
    }

    bool isPointer() const {
        // sentinel bit and discriminant (low bit) are same: pointer
        // different: value

        // case: ptr = 10. value = 10. set value - x = 100
        // ptr = 11 value = 01 - set value - x = 011
        // set ptr 11 = 111
        // set ptr = 00 000

        // store sentinel bit in discriminant
        // 

        // sentinel is always on for pointers and always off for values
        return u.getSentinelValue() == SentinalStatePointer;
   }

    bool isValue() const {
        return !isPointer();
    }

    // pointer is overwritten if it exists
    void setValue(T value) {
        Value val{value};
        if constexpr (AssertSentinelBitCorrect) {
            assert(val.getSentinelValue() == SentinalStateValue && "Sentinel bit set!");
        } else {
            val.setSentinelValue(SentinalStateValue);
        }
        u = val;
    }

    // value is overwritten if it exists
    void setPointer(PtrT pointer) {
        u.setPointer(pointer);
    }

    PtrT getPointer() const {
        bool sentinel = u.getSentinelValue();
        assert(sentinel == SentinalStatePointer && "sentinel should always be set for pointers");
        // discriminant holds the real value for the sentinel bit
        bool discriminant = u.ptrAndDiscriminant.getInt();
        // put sentinel bit into place
        // have to make copy to do so
        Value val = u;
        val.setSentinelValue(discriminant);
        return val.ptrAndDiscriminant.getPointer();
    }

    T& getValue() {
        assert(isValue());
        return u.tvalue;
    }

    const T& getValue() const {
        assert(isValue());
        return u.tvalue;
    }
};

template<typename T, typename PtrT, uint8_t UnusedBitIndex = sizeof(T) * 8, bool UnusedBitState = 0, bool AssertBitUnused = false>
using ValPtrUnion = ValPtrUnionFull<T, PtrT, UnusedBitIndex, UnusedBitState, AssertBitUnused>;



#endif