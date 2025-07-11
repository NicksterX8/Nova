#ifndef VAL_PTR_UNION_INCLUDED
#define VAL_PTR_UNION_INCLUDED

#include <stdint.h>
#include <stddef.h>
#include "llvm/PointerIntPair.h"
#include "utils/system/sysinfo.hpp"

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