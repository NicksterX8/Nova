#ifndef ECS_COMPONENT_MACROS_INCLUDED
#define ECS_COMPONENT_MACROS_INCLUDED

#include "utils/common-macros.hpp"
#include <type_traits>

#define BEGIN_COMPONENT(name) struct name {\
    constexpr static ComponentID ID = ComponentIDs::name;\
    constexpr static bool PROTOTYPE = false;\
    constexpr static const char* NAME = TOSTRING(name);

#define END_COMPONENT(name) }; static_assert(true | ComponentIDs::name, "Checking for id");

#define BEGIN_PROTO_COMPONENT(name) struct name {\
    constexpr static ComponentID ID = ComponentIDs::name;\
    constexpr static bool PROTOTYPE = true;\
    constexpr static const char* NAME = TOSTRING(name);

#define END_PROTO_COMPONENT(name) }; static_assert(true | ComponentIDs::name, "Checking for id");

#define FLAG_COMPONENT(name) BEGIN_COMPONENT(name) END_COMPONENT(name)
#define FLAG_PROTO_COMPONENT(name) BEGIN_PROTO_COMPONENT(name) END_PROTO_COMPONENT(name)

template<typename T>
constexpr bool IsPOD = (
    std::is_trivially_copy_constructible_v<T> &&
    std::is_trivially_move_constructible_v<T> &&
    std::is_trivially_destructible_v<T>);


#define GEN_IDS(name, type, list, elNumName) enum name : type { list, elNumName };

#endif