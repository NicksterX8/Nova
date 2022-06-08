#include <iostream>
#include "math.h"
#include "stdlib.h"
 
using namespace std;

int main() {
    float damage = INFINITY;
    float health = INFINITY;
    health -= damage;
    if (!(health >= 0.0f)) {
        health = 0.0f;
    }
    std::cout << "result: " << health << "\n";

    assert(1);
    return 0;
}