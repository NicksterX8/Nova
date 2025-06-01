#ifndef GECS_INCLUDED
#define GECS_INCLUDED

#include "utils/ints.hpp"
#include "My/Bitset.hpp"
#include "My/HashMap.hpp"
#include "My/SparseSets.hpp"
#include "My/Vec.hpp"
#include "utils/common-macros.hpp"
#include <string>
#include "global.hpp"
#include "ecs.hpp"


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


#define GEN_IDS(name, type, list, elNumName) enum name : type { list, elNumName };
//#define GEN_IDS_STARTING(type, list, elNumName, firstName, firstID) enum : type { firstName = firstID, list, elNumName };

#endif