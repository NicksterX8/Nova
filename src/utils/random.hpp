#ifndef UTILS_RANDOM_INCLUDED
#define UTILS_RANDOM_INCLUDED

#include <stdlib.h>

// returns a random integer in the range of min to max, inclusive.
inline int randomInt(int min, int max) {
    return min + rand() / (RAND_MAX / (max - min + 1) + 1);
}

// return a random integer in the range of min to max, inclusive, 
// that is negative half of the time
int randomIntHalfNegative(int min, int max) {
    return randomInt(min, max) * (randomInt(0, 1) ? 1 : -1);
}

#endif