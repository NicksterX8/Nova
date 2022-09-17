#include <iostream>
#include "math.h"
#include "stdlib.h"
#include <functional>
#include <math.h>
#include <algorithm>
 
using std::cout;

void thing(int size) {
    int arr[size];

    cout << "sizeof arr " << sizeof(arr) << "\n";
}

int func(int a) {
    return a;
}

int main() {

    int x = -1;
    if ((x = func(0))) {
        cout << "0";
    }
    if ((x = func(1))) {
        cout << "1";
    }

    return 0;
}