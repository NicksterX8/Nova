#ifndef MY_MY_INCLUDED
#define MY_MY_INCLUDED

#include "../constants.hpp"

#define MY_NAMESPACE_NAME My

#define MY_CLASS_START namespace MY_NAMESPACE_NAME {
#define MY_CLASS_END }

#define MY_malloc(size) malloc(size)
#define MY_realloc(ptr, size) realloc(ptr, size)
#define MY_free(ptr) free(ptr)

#define MY_SENTINEL_PTR(type) ((type*)1)
#define MY_SENTINEL_VAL 0x7ff8dead

#endif