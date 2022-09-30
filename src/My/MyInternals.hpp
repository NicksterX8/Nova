#ifndef MY_INTERNALS_INCLUDED
#define MY_INTERNALS_INCLUDED

#include "../constants.hpp"
#include "../utils/Log.hpp"

#define MY_NAMESPACE_NAME My

#define MY_CLASS_START namespace MY_NAMESPACE_NAME {
#define MY_CLASS_END }

#ifndef MY_malloc
#define MY_malloc(size) malloc(size)
#endif
#ifndef MY_realloc
#define MY_realloc(ptr, size) realloc(ptr, size)
#endif
#ifndef MY_free
#define MY_free(ptr) free(ptr)
#endif

#define MY_SENTINEL_PTR(type) ((type*)1)
#define MY_SENTINEL_VAL 0x7ff8dead

#endif