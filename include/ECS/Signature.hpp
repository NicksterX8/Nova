#ifndef ECS_GENERIC_SIGNATURE_INCLUDED
#define ECS_GENERIC_SIGNATURE_INCLUDED

#include "utils/ints.hpp"

namespace ECS {

using ComponentID = Sint16;

template<size_t maxComponentID>
struct SignatureTemplate : My::Bitset<maxComponentID> {
    private: using Base = My::Bitset<maxComponentID>; public:
    using Self = Signature<maxComponentID>;

    constexpr Signature() {}

    constexpr Signature(typename Base::IntegerType startValue) : Base(startValue) {}

    template<class C>
    constexpr bool getComponent() const {
        constexpr ComponentID id = C::ID;
        return this->operator[](id);
    }

    template<class C>
    constexpr void setComponent(bool val) {
        constexpr ComponentID id = C::ID;
        My::Bitset<maxComponentID>::set(id, val);
    }
};

#define ECS_MAX_COMPONENT 64
constexpr ComponentID MaxComponentID = ECS_MAX_COMPONENT;
using Signature = SignatureTemplate<MaxComponentID>;

struct SignatureHash {
    size_t operator()(Signature self) const {
        //TODO: OMPTIMIZE improve hash
        // intsPerHash will always be 1 or greater as IntegerT cannot be larger than size_t
        constexpr size_t intsPerHash = sizeof(size_t) / Signature::IntegerSize;
        size_t hash = 0;
        for (int i = 0; i < self.nInts; i++) {
            for (int j = 0; j < intsPerHash; j++) {
                hash ^= (size_t)self.bits[i] << j * Signature::IntegerSize * CHAR_BIT;
            }
        }
        return hash;
    }
};

}

#endif