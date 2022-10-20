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

void func1(int *a, const int* b) {
    memcpy(a, b, sizeof(int));   
}

void func2(int* a, const int* b) {
    *a = *b;
}

int* a = new int(0);
int* b = new int(12);

struct Trash {
    int a;

    Trash(int x): a(x) {}

    ~Trash() {
        a = 3;
    }
};

struct S {
    int a;
    double d;
    int b;
};

using std::cout;

int fff(int a) {
    if (a) {
        return 0;
    } else {
        return 1;
    }
}

int main() {
/*
    int count = 100000;
    
    auto before = getNow();
    for(int i = 0; i < count; i++) {
        func1(a, b);
    }
    auto after1 = getNow();
    for(int i = 0; i < count; i++) {
        func2(a, b);
    }
    auto after2 = getNow();
    auto time1 = after1 - before;
    auto time2 = after2 - after1;
    printf("time1: %llu. time2: %llu\n", time1.count(), time2.count());

    const Trash trash = Trash(1);
    cout << trash.a << std::endl;
    

    trash.~Trash();

    cout << trash.a << std::endl;
*/
    for (size_t s = 1; s < 1000000000; s += 100) {
        size_t* ptr = (size_t*)malloc(s * sizeof(size_t));
        if (!ptr) continue;
        size_t stored = *(ptr);
        if (s == stored) {
            cout << "Stored == " << s;
        }
    }

    return 0;
}