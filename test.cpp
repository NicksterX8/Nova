#include <iostream>
#include "math.h"
#include "stdlib.h"
#include <functional>
 
using namespace std;
#include <SDL2/SDL.h>

namespace ECS {

const int x = 2;

class ECS {
    static const int x = 3;
};

}

using namespace ECS;

int main() {
    printf("X: %d", ECS::x);
    return 0;
}