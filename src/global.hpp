#ifndef GLOBAL_INCLUDED
#define GLOBAL_INCLUDED

#include "constants.hpp"

#ifdef new
#undef new
#endif

void* malloc_func(size_t size, const char* file, int line) ;
void free_func(void* ptr, const char* file, int line) noexcept;

void* operator new(size_t size, const char* file, int line);
void operator delete(void* ptr, const char* file, int line) noexcept;

#define new_debug new (__FILE__, __LINE__)
#define delete_debug delete (__FILE__, __LINE__)

#endif