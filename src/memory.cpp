#include "memory.hpp"
#include <stdio.h>

std::function<void*(size_t sizeNeeded)> onAllocFail;

#undef malloc
#undef free
#undef realloc

void* malloc_func(size_t size) {
    void* ptr = malloc(size);
    if (ptr) {
        return ptr;
    }
    return onAllocFail(size);
}

void free_func(void* ptr) {
    free(ptr);
}

void* realloc_func(void* ptr, size_t size) {
    return realloc(ptr, size);
}

void* malloc_func_debug(size_t size, const char* file, int line) {
    printf("Allocated %lu bytes at %s:%d\n", size, file, line);
    return malloc_func(size);
}

void* operator new(size_t size, const char* file, int line) {
    return malloc_func_debug(size, file, line);
}

void free_func_debug(void* ptr, const char* file, int line) noexcept {
    printf("Freed %p at %s:%d\n", ptr, file, line);
    free_func(ptr);
}

void operator delete(void* ptr, const char* file, int line) noexcept {
    free_func_debug(ptr, file, line);
}