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

const size_t innerIterations = 100000;

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

struct My2 {
    char* c[10];
};

struct Random {
    int a;
    char c[2];
    void* vptr;
};

struct My {
    int* ints[3];
    int a;
    int b;
    
};

int main() {
    void* alloced = malloc(10000);
    if (!alloced) {
        printf("initial null\n");
    }
    for (size_t i = 0; i < 50000000; i++) {
        void* ptr = malloc(i*64);
        if (!ptr) {
            printf("PTR WAS NULL AT %lu!\n", i);
            malloc(i*32);
            malloc(i*16);
            malloc(i*8);
            malloc(i*4);
            malloc(i);
            for (int j = 100000; j > 0; j--) {
                malloc(j);
            }
            break;
        }
    }

    const size_t reallocS = 0;

    void* newAlloced = realloc(alloced, reallocS);
    if (newAlloced == alloced) {
        printf("new == old\n");
    }
    if (!newAlloced) {
        printf("new alloced null!!!\n");
        void* ma = malloc(reallocS);
        if (ma)
            printf("but malloc worked!\n");
    }

    void* b = malloc(128);
    void* a = realloc(b, 0);
    if (!a) {
        printf("a is null\n");
    } else {
        *((int*)a) = 2;
    }

    void* b2 = malloc(128);
    void* a2 = realloc(b, 1);
    if (!a2) {
        printf("a2 is null\n");
    } else {
        *((int*)a2) = 2;
    }


    return 0;
}