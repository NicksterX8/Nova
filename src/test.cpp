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
#include <malloc/malloc.h>

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

    Big() {
        z = 12;
    }

    Big(int x) {
        
    }
};

template<typename D>
struct Base {
    void* ptr;

    int foo() {
        return 12;
    }

    int bar() {
        return static_cast<D*>(this)->foo();
    }
};

struct Child : Base<Child> {
    int foo() {
        return 15;
    }
};

int main() {
    size_t totalBytes = 0;
    size_t totalbonus = 0;
    // for (size_t s = 0, s2 = 1, s3 = 1; s < 100000; s = s2, s2 = s3, s3 = s + s2) {
    //     if (s > 10) {
    //         s -= 3;
    //     }
    //     if (s > 100) {
    //         s -= 20;
    //     }
    //     if (s > 1000) {
    //         s -= 100;
    //     }
    //     void* ptr = malloc(s);
    //     printf("size: %zu. good size: %zu. bonus percent: %.3f\n", s, malloc_good_size(s), float(malloc_good_size(s) - s) / float(s) * 100);
    //     totalBytes += s;
    //     totalbonus += malloc_good_size(s) - s;
    // }
    for (size_t s = 20; s < 1000; s *= 2) {
        size_t s2 = s + rand() % 24 - 12;
        printf("size: %zu. good size: %zu. bonus percent: %.3f\n", s2, malloc_good_size(s2), float(malloc_good_size(s2) - s2) / float(s2) * 100);
        totalBytes += s2;
        totalbonus += malloc_good_size(s2) - s2;
    }
    printf("averaged bonus percent: %.2f", float(totalbonus)/ float(totalBytes) * 100);
    //memcpy((void*)&base, &child, sizeof(Child));

    // printf("x: %d", (&base)->foo());
    // Base* basePtr = &base;
    //printf("ptr: %d", basePtr->foo());
}
