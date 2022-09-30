#include "global.hpp"
#include "memory.hpp"

#define new new_debug
#define delete delete_debug

#define malloc(size) malloc_func_debug(size, __FILE__, __LINE__)
#define free(ptr) free_func_debug(ptr, __FILE__, __LINE__)