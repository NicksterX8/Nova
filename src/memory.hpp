#ifndef MEMORY_INCLUDED
#define MEMORY_INCLUDED

#include <stdlib.h>
#include <functional>
#include "My/Vec.hpp"

extern std::function<void*(size_t size)> onAllocFail;
extern thread_local void* dataBuffer;
extern thread_local size_t dataBufferCapacity;
const size_t defaultDataBufferCapacity = 2048;

void* malloc_func(size_t size);
void free_func(void* ptr);
void* realloc_func(void* ptr, size_t size);

void* malloc_func_debug(size_t size, const char* file, int line);
void free_func_debug(void* ptr, const char* file, int line) noexcept;

void* operator new(size_t size, const char* file, int line);
void operator delete(void* ptr, const char* file, int line) noexcept;

inline void memory_init(const std::function<void*(size_t size)>& onAllocFailCallback) {
    onAllocFail = onAllocFailCallback;
}

#define Malloc(size) malloc_func(size)
#define Free(ptr) free_func(ptr)

#define new_debug new (__FILE__, __LINE__)
#define delete_debug delete (__FILE__, __LINE__)

#define malloc(size) malloc_func(size)
#define free(ptr) free_func(ptr)
#define realloc(ptr, size) realloc_func(ptr, size)

#endif