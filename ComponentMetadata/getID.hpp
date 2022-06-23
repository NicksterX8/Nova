#ifndef GET_ID_INCLUDED
#define GET_ID_INCLUDED

template<class T>
constexpr unsigned int getRawID() {
    return -1;
}

template<class T>
constexpr unsigned int getID() {
    return getRawID<const T>();
}

template<class T>
constexpr bool componentIsMutable() {
    return (getID<T>() != -1) && getRawID<T>() == -1;
}

template<class T>
constexpr bool componentIsConst() {
    return !componentIsMutable<T>();
}

#endif