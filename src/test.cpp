#include <iostream>
#include "math.h"
#include "stdlib.h"
#include <functional>
#include <math.h>
#include <string>
#include <algorithm>
#include <vector>
#include <memory>
#include <array>
#include <unistd.h>
#include <time.h>
#include <chrono>

std::chrono::time_point<std::chrono::high_resolution_clock> getNow() {
    return std::chrono::high_resolution_clock::now();
}

//time elapsed: 949738584
//time elapsed: 1358284833

constexpr size_t innerIterations = 100000;

void func(size_t i, std::vector<size_t>& array) {
    /*
    for (size_t j = 0; j < innerIterations; j++) {

       (*array)[j] = i;
    }
    /*/
    for (size_t& arrVal : array) {
        arrVal = rand();
    }
    //*/
}

void func2(size_t i, std::vector<size_t>& array) {
    for (size_t j = 0; j < array.size(); j++) {
        size_t& arrVal = array[j];
        arrVal = rand();
    }
}

int main() {
    auto before = getNow();

    size_t iterations = 10000;
    std::vector<size_t> array(innerIterations);
    for (size_t i = 0; i < iterations; i++) {
        func(i, array);
    }

    auto after = getNow();

    printf("time elapsed: %llu\n", (after - before).count());
    return 0;
}