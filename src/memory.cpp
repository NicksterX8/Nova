#include "memory.hpp"
#include <stdio.h>
#include <execinfo.h>
#include <signal.h>
#include <unistd.h>

namespace Mem {

// dont accidentally use macros in implementation
#undef malloc
#undef free
#undef realloc

/* wrappers */

void* _malloc(size_t size) {
    return malloc(size);
}

void _free(void* mem) {
    free(mem);
}

void* _realloc(void* ptr, size_t size) {
    return realloc(ptr, size);
}


/* debug */

void* debug_malloc(size_t size, const char* file, int line) {
    printf("Allocated %lu bytes at %s:%d\n", size, file, line);
    return malloc(size);
}

void debug_free(void* ptr, const char* file, int line) noexcept {
    printf("Freed %p at %s:%d\n", ptr, file, line);
    free(ptr);
}

}