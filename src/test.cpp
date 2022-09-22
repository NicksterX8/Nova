#include <iostream>
#include "math.h"
#include "stdlib.h"
#include <functional>
#include <math.h>
#include <string>
#include <algorithm>
#include <array>

#define MACRO2 1

#define macro MACRO2

#undef MACRO2
#define MACRO2 2
 
using std::cout;

struct C {
    void f() {
        cout << "function: " << __FUNCTION__;
    }
};

namespace D {
    void f() {
        cout << "function: " << __FUNCTION__;
    }   
}


void thing(int size) {
    int arr[size];

    cout << "sizeof arr " << sizeof(arr) << "\n";
}

int func(int a) {
    return a;
}

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)
#define VARD(...) TOSTRING(__VA_ARGS__)

template<int N1, int N2>
constexpr std::array<char, N1+N2> str_add(std::array<char, N1> a, std::array<char, N2> b) {

}

void takesStr(const char* a, const char* b) {

}

int main() {
    const char* fun = __FUNCTION__; const char* a = "a";
    takesStr(__FUNCTION__, __FUNCTION__);

    C c;
    c.f();
    D::f();

    const char* ch = __FILE__;

    return 0;
}