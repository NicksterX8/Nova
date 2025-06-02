#ifndef ECS_GENERIC_SIGNATURE_INCLUDED
#define ECS_GENERIC_SIGNATURE_INCLUDED

#include "utils/ints.hpp"

namespace GECS {

using ComponentID = Sint16;

struct DefaultIDGetter {
    template<class C>
    static constexpr ComponentID get() {
        return C::ID;
    }
};

template<size_t maxComponentID, class IDGetter = DefaultIDGetter>
struct Signature : My::Bitset<maxComponentID> {
    private: using Base = My::Bitset<maxComponentID>; public:
    using Self = Signature<maxComponentID>;

    constexpr Signature() {}

    /*
    constexpr Signature(const Self& self) : Base(self) {

    }
    */

    constexpr Signature(typename Base::IntegerType startValue) : Base(startValue) {

    }
    
    /*
    constexpr Signature(Base bitset) : My::Bitset<maxComponentID>(bitset) {
        
    }
    */

    template<class C>
    constexpr bool getComponent() const {
        constexpr ComponentID id = IDGetter::template get<C>();
        return this->operator[](id);
    }

    template<class C>
    constexpr void setComponent(bool val) {
        constexpr ComponentID id = IDGetter::template get<C>();
        My::Bitset<maxComponentID>::set(id, val);
    }

    struct Hash {
        size_t operator()(Self self) const {
            //TODO: OMPTIMIZE improve hash
            // intsPerHash will always be 1 or greater as IntegerT cannot be larger than size_t
            constexpr size_t intsPerHash = sizeof(size_t) / Self::IntegerSize;
            size_t hash = 0;
            for (int i = 0; i < self.nInts; i++) {
                for (int j = 0; j < intsPerHash; j++) {
                    hash ^= (size_t)self.bits[i] << j * Self::IntegerSize * CHAR_BIT;
                }
            }
            return hash;
        }
    };
};

}

#endif