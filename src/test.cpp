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
#include <type_traits>
#include <array>

#define COMBINE1(X,Y) X##Y  // helper macro
#define COMBINE(X,Y) COMBINE1(X,Y)
#define FOR_EACH_VAR_TYPE(func_call) int COMBINE(_dummy_for_each_var_type_helper, __LINE__)[] = {0, (func_call, 0) ...}; (void)COMBINE(_dummy_for_each_var_type_helper, __LINE__);

//time elapsed: 949738584
//time elapsed: 1358284833
/*
const size_t innerIterations = 100000;

void func1(int *a, const int* b) {
    memcpy(a, b, sizeof(int));   
}

void func2(int* a, const int* b) {
    *a = *b;
}



#define COUNTER_READ_CRUMB( TAG, RANK, ACC ) counter_crumb( TAG(), constant_index< RANK >(), constant_index< ACC >() )
#define COUNTER_READ( TAG ) COUNTER_READ_CRUMB( TAG, 1, COUNTER_READ_CRUMB( TAG, 2, COUNTER_READ_CRUMB( TAG, 4, COUNTER_READ_CRUMB( TAG, 8, \
    COUNTER_READ_CRUMB( TAG, 16, COUNTER_READ_CRUMB( TAG, 32, COUNTER_READ_CRUMB( TAG, 64, COUNTER_READ_CRUMB( TAG, 128, 0 ) ) ) ) ) ) ) )
 
#define COUNTER_INC( TAG ) \
constant_index< COUNTER_READ( TAG ) + 1 > \
constexpr counter_crumb( TAG, constant_index< ( COUNTER_READ( TAG ) + 1 ) & ~ COUNTER_READ( TAG ) >, \
          					constant_index< ( COUNTER_READ( TAG ) + 1 ) & COUNTER_READ( TAG ) > ) { return {}; }
 
#define COUNTER_LINK_NAMESPACE( NS ) using NS::counter_crumb;
 
#include <utility>
 
template< std::size_t n >
struct constant_index : std::integral_constant< std::size_t, n > {};
 
template< typename id, std::size_t rank, std::size_t acc >
constexpr constant_index< acc > counter_crumb( id, constant_index< rank >, constant_index< acc > ) { return {}; } // found by ADL via constant_index
 
struct ComponentCounter {};

#define TAS ComponentCounter

template<class T>
constexpr int readCounter() {
    return COUNTER_READ(ComponentCounter);
}

template<class T, int id>
struct Struct {
    // read*/
    /*
    constexpr int GetID() const { COUNTER_READ_CRUMB( TAS, 1, 
            COUNTER_READ_CRUMB( TAS, 2, 
                COUNTER_READ_CRUMB( TAS, 4, 
                    COUNTER_READ_CRUMB( TAS, 8,
                        COUNTER_READ_CRUMB( TAS, 16,
                            COUNTER_READ_CRUMB( TAS, 32,
                                COUNTER_READ_CRUMB( TAS, 64, 
                                    COUNTER_READ_CRUMB( TAS, 128, 0 ) ) ) ) ) ) ) ); return 0; };

    constexpr static int getID() {
        Struct<T> self;
        return self.GetID();
    }

    // increment
    constant_index< COUNTER_READ( TAS ) + 1 >
    constexpr counter_crumb( TAS, constant_index< ( COUNTER_READ( TAS ) + 1 ) & ~ COUNTER_READ( TAS ) >, \
          					constant_index< ( COUNTER_READ( TAS ) + 1 ) & COUNTER_READ( TAS ) > ) const { return {}; }
    *//*
    constexpr static int ID = id;
};

template<class C>
struct Struct2 {

};

#define MIN(a, b) ((a < b) ? a : b)
#define MAX(a, b) ((a > b) ? a : b)

#define IABS(integer) ((integer) >= 0 ? (integer) : -(integer))

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

#define COMBINE1(X,Y) X##Y  // helper macro
#define COMBINE(X,Y) COMBINE1(X,Y)

#define FOR_EACH_VAR_TYPE(func_call) int COMBINE(_dummy_for_each_var_type_helper, __LINE__)[] = {0, (func_call, 0) ...}; (void)COMBINE(_dummy_for_each_var_type_helper, __LINE__);

#define ssizeof(v) ((ssize_t)sizeof(v)) 

#define LIKELY(expr) LLVM_LIKELY(expr)
#define UNLIKELY(expr) LLVM_UNLIKELY(expr)
*/

struct TestS {
    int x;
};

struct Big {
    size_t z;
    size_t y;
};

void func(int x) {}

int main() {
    constexpr bool t = true;
    if constexpr (t) {
        func(0);
    } else {
        func(0, 1);
    }

    return alignof(decltype(TestS()));
}
