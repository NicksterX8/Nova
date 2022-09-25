#include <iostream>
#include "math.h"
#include "stdlib.h"
#include <functional>
#include <math.h>
#include <string>
#include <algorithm>
#include <array>

using std::cout;

int func() {
    cout << "func run\n";
    return 3;
}

class Thing {
public:
    int priv = func();

    Thing();
};

Thing::Thing() {
    priv = 3;
}

struct SS {
    int x;
    int* xPtr;
};



int main() {

    int x = 0;
    Thing thing;
    cout << "thing val: " << thing.priv;

    return 0;
}