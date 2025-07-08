#ifndef CONSTRUCT_INCLUDED
#define CONSTRUCT_INCLUDED

template<typename T>
void bitCopy(T* dst, const T& value) {
    memcpy(dst, &value, sizeof(T));
}

template<typename T>
void construct(T* dst) {
    bitCopy(dst, T());
}

template<typename T>
void construct(T* dst, const T& value) {
    bitCopy(dst, value);
}

#endif