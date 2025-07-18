#ifndef UTILS_TEMPLATES_INCLUDED
#define UTILS_TEMPLATES_INCLUDED

#include <type_traits>
#include <tuple>
#include "utils/common-macros.hpp"

template<typename... Ts>
constexpr size_t sumSizes() {
    size_t sizes[] = {sizeof(Ts) ...};
    size_t size = 0;
    for (int i = 0; i < sizeof...(Ts); i++) {
        size += sizes[i];
    }
    return size;
}

/// @brief Defines the smallest standard unsigned integer type (uint8-uint64) that contains Size number of bytes
/// @tparam Size minimum number of bytes needed in the integer
template<size_t Size>
using SmallestUnsignedInteger = std::conditional_t<Size <= 4, std::conditional_t<Size <= 2, std::conditional_t<Size <= 1, std::conditional_t<Size == 0, void, uint8_t>, uint16_t>, uint32_t>, uint64_t>;


// Get the type of a type in a tuple.  
// Used in order to unwrap all elements of a tuple
template <typename T, typename... Ts>
struct TupleTypeIndex_;

template <typename T, typename... Ts>
struct TupleTypeIndex_<T, T, Ts...> : std::integral_constant<std::size_t, 0> {};

template <typename T, typename U, typename... Ts>
struct TupleTypeIndex_<T, U, Ts...> : std::integral_constant<std::size_t, 1 + TupleTypeIndex_<T, Ts...>::value> {};

template <typename T, typename... Ts>
constexpr std::size_t TupleTypeIndex = TupleTypeIndex_<T, Ts...>::value;


template <typename... Ts>
struct TupleMinusFirst_;

template <typename First, typename... Rest>
struct TupleMinusFirst_<First, Rest...> {
    using type = std::tuple<Rest...>;
};

template<typename... Types>
using TupleMinusFirst = typename TupleMinusFirst_<Types...>::type;

// Base case: only one type left — return empty tuple
template <typename... Ts>
struct TupleMinusLast_;

template <>
struct TupleMinusLast_<> {
    using type = std::tuple<>;
};

template <typename T>
struct TupleMinusLast_<T> {
    using type = std::tuple<>;
};

template <typename Head, typename... Tail>
struct TupleMinusLast_<Head, Tail...> {
private:
    using tail_result = typename TupleMinusLast_<Tail...>::type;
public:
    using type = decltype(std::tuple_cat(std::declval<std::tuple<Head>>(), std::declval<tail_result>()));
};

template <typename... Ts>
using TupleMinusLast = typename TupleMinusLast_<Ts...>::type;

// For free functions or static functions
template <typename T>
struct FunctionTraits;

// Function pointer
template <typename Ret, typename... Args>
struct FunctionTraits<Ret(*)(Args...)> {
    using ReturnType = Ret;
    using ArgsTuple = std::tuple<Args...>;
};

// Member function pointer
template <typename Class, typename Ret, typename... Args>
struct FunctionTraits<Ret(Class::*)(Args...)> {
    using ReturnType = Ret;
    using ClassType = Class;
    using ArgsTuple = std::tuple<Args...>;
};

// Const member function pointer
template <typename Class, typename Ret, typename... Args>
struct FunctionTraits<Ret(Class::*)(Args...) const> {
    using ReturnType = Ret;
    using ClassType = Class;
    using ArgsTuple = std::tuple<Args...>;
};

// Primary template: false by default
template <typename T, template <typename...> class Template>
struct is_instantiation_of : std::false_type {};

// Partial specialization: true if T is Template<...>
template <template <typename...> class Template, typename... Args>
struct is_instantiation_of<Template<Args...>, Template> : std::true_type {};

template<typename T, template<typename...> class Template>
constexpr bool is_instantiation_of_v = is_instantiation_of<T, Template>::value;

#endif