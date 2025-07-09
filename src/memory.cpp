#include "memory.hpp"
#include <stdio.h>
#include <execinfo.h>
#include <signal.h>
#include <unistd.h>

namespace Mem {

std::function<void()> needMemoryCallback;
std::function<void()> memoryFailCallback;

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

MemoryBlock failedToAlloc(bool required, AllocCallback callback) {
    printf("FAILED TO ALLOC!\n");
    static bool allocFailed = false;
    if (!allocFailed) {
        needMemoryCallback();
        // guard callback with fail so this can't be recursively called
        allocFailed = true;
        auto block = callback();
        allocFailed = false;
        return block;
    } else if (required) {
        memoryFailCallback(); // exit process
    }
    return {nullptr, 0}; // if not required then return null
}

/* variations */

void handler(int sig) {
  void *array[10];
  size_t size;

  // get void*'s for all entries on the stack
  size = backtrace(array, 10);

  // print out all the frames to stderr
  fprintf(stderr, "Error: signal %d:\n", sig);
  backtrace_symbols_fd(array, size, STDERR_FILENO);
  exit(1);
}

/* debug */

void* debug_malloc(size_t size, const char* file, int line) {
    printf("Allocated %lu bytes at %s:%d\n", size, file, line);
    return _malloc(size);
}

void debug_free(void* ptr, const char* file, int line) noexcept {
    printf("Freed %p at %s:%d\n", ptr, file, line);
    _free(ptr);
}

}