#ifndef ECS_GENERIC_SIGNATURE_INCLUDED
#define ECS_GENERIC_SIGNATURE_INCLUDED

#include "utils/ints.hpp"
#include "My/Bitset.hpp"

namespace ECS {

using ComponentID = Sint16;

#define ECS_MAX_COMPONENT 64
constexpr ComponentID MaxComponentID = ECS_MAX_COMPONENT;

template<class C>
constexpr ComponentID getID() {
    return C::ID;
}

// prototype components will result in a compilation error
template<class C>
constexpr ComponentID getIDNoProto() {
    static_assert(!C::PROTOTYPE, "Invalid use of prototype component, only regular components are accepted");
    return C::ID;
}

template<class C, typename = void>
struct HasComponentID_t : std::false_type {};

template<class C>
struct HasComponentID_t<C, std::void_t<decltype(C::ID)>>
    : std::is_convertible<decltype(C::ID), int> {};

template<class C>
inline constexpr bool IsComponent = HasComponentID_t<C>::value;

static_assert(IsComponent<int> == false, "Is component not detecting invalid components");

template <bool WantComponents, typename... Vars>
struct filter_components;

template <bool WantComponents>
struct filter_components<WantComponents> {
    using type = std::tuple<>;
};

template <bool WantComponents, typename Head, typename... Tail>
struct filter_components<WantComponents, Head, Tail...> {
private:
    using tail_filtered = typename filter_components<WantComponents, Tail...>::type;
public:
    using type = std::conditional_t<
        ECS::IsComponent<std::remove_pointer_t<Head>> == WantComponents,
        decltype(std::tuple_cat(std::declval<std::tuple<Head>>(), std::declval<tail_filtered>())),
        tail_filtered
    >;
};

template <bool WantComponents, typename... Vars>
using filter_components_t = typename filter_components<WantComponents, Vars...>::type;

template<class ...Cs>
constexpr static My::Bitset<MaxComponentID> getSignature() {
    constexpr ComponentID ids[] = {getID<Cs>() ...};
    // sum component signatures
    auto result = My::Bitset<MaxComponentID>(0);
    for (size_t i = 0; i < sizeof...(Cs); i++) {
        if (ids[i] < result.size())
            result.set(ids[i]);
    }
    return result;
}

// get the signature of only the const components
template<class ...Cs>
constexpr static My::Bitset<MaxComponentID> getConstSignature() {
    constexpr ComponentID ids[] = {getID<Cs>() ...};
    constexpr bool constness[] = {std::is_const_v<Cs> ...};
    // sum component signatures
    auto result = My::Bitset<MaxComponentID>(0);
    for (size_t i = 0; i < sizeof...(Cs); i++) {
        if (ids[i] < result.size() && constness[i])
            result.set(ids[i]);
    }
    return result;
}

// get the signature of only the mutable components
template<class ...Cs>
constexpr static My::Bitset<MaxComponentID> getMutableSignature() {
    constexpr ComponentID ids[] = {getID<Cs>() ...};
    constexpr bool constness[] = {std::is_const_v<Cs> ...};
    // sum component signatures
    auto result = My::Bitset<MaxComponentID>(0);
    for (size_t i = 0; i < sizeof...(Cs); i++) {
        if (ids[i] < result.size() && !constness[i])
            result.set(ids[i]);
    }
    return result;
}

// prototype components will result in a compilation error
template<class ...Cs>
constexpr static My::Bitset<MaxComponentID> getSignatureNoProto() {
    constexpr ComponentID ids[] = {getIDNoProto<Cs>() ...};
    // sum component signatures
    auto result = My::Bitset<MaxComponentID>(0);
    for (size_t i = 0; i < sizeof...(Cs); i++) {
        if (ids[i] < result.size())
            result.set(ids[i]);
    }
    return result;
}

struct Signature : My::Bitset<MaxComponentID> {
    private: using Base = My::Bitset<MaxComponentID>; public:

    constexpr Signature() {}

    constexpr Signature(typename Base::IntegerType startValue) : Base(startValue) {}

    constexpr Signature(const My::Bitset<MaxComponentID>& base) : Base(base) {}

    template<class C>
    constexpr bool getComponent() const {
        return this->operator[](C::ID);
    }

    template<class C>
    constexpr void setComponent(bool val = true) {
        My::Bitset<MaxComponentID>::set(C::ID, val);
    }

    template<class... Cs>
    constexpr bool hasComponents() const {
        return hasAll(getSignature<Cs...>());
    }
};

struct SignatureHash {
    size_t operator()(Signature self) const {
        //TODO: OMPTIMIZE improve hash
        // intsPerHash will always be 1 or greater as IntegerT cannot be larger than size_t
        constexpr size_t intsPerHash = sizeof(size_t) / Signature::IntegerSize;
        size_t hash = 0;
        for (unsigned i = 0; i < self.nInts; i++) {
            for (unsigned j = 0; j < intsPerHash; j++) {
                hash ^= (size_t)self.bits[i] << j * Signature::IntegerSize * CHAR_BIT;
            }
        }
        return hash;
    }
};

}

#endif