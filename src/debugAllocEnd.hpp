#include "memory.hpp"

#ifdef new
#undef new
#endif

#ifdef delete
#undef delete
#endif

#define malloc(size) malloc_func(size)

#define free(ptr) free_func(ptr)