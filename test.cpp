#include <iostream>
#include "math.h"
#include "stdlib.h"
#include <functional>
 
using namespace std;

class Base {};

class Sub : public Base {};

int tset(const int* x) {
    *const_cast<int*>(x) = 2;
}

int main() {
    int i = 2;
    const int& x = i;
    int* ptr = &const_cast<int&>(x);
    tset(&i);
    std::cout << "i: " << i << "\n";
    return 0;
}