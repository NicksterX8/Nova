#ifndef GET_ID_INCLUDED
#define GET_ID_INCLUDED

#include <stdint.h>

typedef uint32_t ComponentID;

template<class T>
constexpr ComponentID getRawID() {
    return -1;
}

template<class T>
constexpr ComponentID getID() {
    return getRawID<const T>();
}

/*
template<class T>
constexpr bool componentIsMutable() {
    return (getID<T>() != -1) && getRawID<T>() == -1;
}

template<class T>
constexpr bool componentIsConst() {
    return !componentIsMutable<T>();
}
*/

#endif