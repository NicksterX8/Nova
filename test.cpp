#include <iostream>
#include "math.h"
#include "stdlib.h"
 
using namespace std;

struct S {};

int main() {
    int x = 12;
    int* iPtr = &x;
    void* cPtr = (void*)&x;

    if ((char*)iPtr == (char*)cPtr) {
        cout << "equal\n";
    }

    cout << "iPtr: " << iPtr << " cPtr: " << cPtr << "\n";

    return 0;
}