#pragma once

#include <SDL3/SDL_timer.h>

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

#define START_TIME(timer) auto timer##_start = SDL_GetPerformanceCounter()

#define END_TIME(timer) auto timer##_end = SDL_GetPerformanceCounter()

inline void printTime(const char* timerName, Uint64 startTime, Uint64 endTime, int iterations = 1) {
    Uint64 diff = endTime - startTime;
    double ms = diff / (double)(SDL_GetPerformanceFrequency() / 1000);

    if (iterations > 1) {
        // average iterations
        double avg = ms / iterations;
        printf("%s - took %f ms on average\n", timerName, avg);
    } else {
        printf("%s - took %f ms\n", timerName, ms);
    }
}

#define PRINT_TIME(timer, ...) printTime(TOSTRING(timer), timer##_start, timer##_end, ##__VA_ARGS__)