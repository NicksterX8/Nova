#include "global.hpp"
#include <stdlib.h>

void* malloc_func(size_t size, const char* file, int line) {
    printf("Allocated %lu bytes at %s:%d\n", size, file, line);
    return malloc(size);
}

void* operator new(size_t size, const char* file, int line) {
    return malloc_func(size, file, line);
}

void free_func(void* ptr, const char* file, int line) noexcept {
    printf("Freed %p at %s:%d\n", ptr, file, line);
    free(ptr);
}

void operator delete(void* ptr, const char* file, int line) noexcept {
    free_func(ptr, file, line);
}