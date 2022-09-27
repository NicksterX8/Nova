#include "global.hpp"

#define new new_debug
#define delete delete_debug

#define malloc(size) malloc_func(size, __FILE__, __LINE__)
#define free(ptr) free_func(ptr, __FILE__, __LINE__)