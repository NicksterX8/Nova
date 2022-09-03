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

int main() {

    thing(4);
    cout << "result2: " << std::max(1.0, (double)NAN) << "\n";

    return 0;
}