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

void func(int arr[5]) {

}

struct Charlie {
    int* intPtr;
    int y;

    Charlie(int x) {
        intPtr = &this->y;
    }
};

using SimpleFunc = void(void*, int N);

struct Base {
    using VirtualMethod = void (Base::*)(int N);

    int x;

    VirtualMethod method;

    template<typename MethodT>
    Base(MethodT method) : method((VirtualMethod)method) {}
};

struct Alpha : Base {
    int a = 20;

    Alpha(int a) : Base(&Alpha::Execute), a(a) {}

    void Execute(int N) {
        printf("N: %d", N + a);
    } 
};

struct FakeChild {
    Base* base;
    int size;
};

template<class T>
FakeChild makeBase(T* basePtr) {
    return {basePtr, sizeof(T)};
}

FakeChild copyBase(FakeChild fake) {
    // make enough space for full structure
    char* buf = new char[fake.size];
    // copy full structure to copy
    memcpy(buf, (void*)fake.base, fake.size);

    Base* copyPtr = (Base*)buf;
    return {copyPtr, fake.size};
}

void testY(void* data, int n) {

}

int* ints = new int(12);

struct Job {
    using ExeType = std::function<void(void*, int, int)>;
    ExeType exe;

    Job(const ExeType& exe) : exe(exe) {}

    bool parallelizable = false;
    bool mainThread = false;
};

template<class Child>
struct JobDecl : Job {

    JobDecl() : Job([](void* childPtr, int startN, int endN){
            for (int N = startN; N < endN; N++) {
                (((Child*)childPtr)->Child::Execute)(N);
            }
        }) {
        // exe = [](void* childPtr, int startN, int endN){
        //     for (int N = startN; N < endN; N++) {
        //         (((Child*)childPtr)->Child::Execute)(N);
        //     }
        // };
    }
};

template<class Child>
struct IJobParallelFor : JobDecl<Child> {
    IJobParallelFor() {
       
    }
};

#define JOB_STRUCT(name) struct name : JobDecl<name>
#define JOB_PARALLEL_FOR(name) struct name : IJobParallelFor<name>


JOB_PARALLEL_FOR(TestJob2) {
    void Execute(int N) {

    }
};

JOB_STRUCT(TestJob) {
    int counter = 0;

    TestJob() {

    }

    void Execute(int N) {
        counter += N;
    }
};

template<typename Child>
struct Mama {
    
    Mama() {
        static_assert(&Child::Execute != nullptr, "Must have execute method");
        static_assert(std::is_same<decltype(std::declval<Child>().Execute(0)), void>::value, "Execute should take one parameter and return void!");
    }
};

struct Baby : Mama<Baby> {
    void Execute(int N) {
       
    }
};

struct TestS {
    TestS(int x) {}
    
    TestS(const TestS& other) {
        printf("Running copy constructor\n");
    }

    void operator[](int x) {

    }
};

void func(TestS* ptr) {
    TestS value = *ptr;
    value[0];
}

int main() {
    Baby baby;

    TestJob testJob;
    Job* job = &testJob;

    TestS t = TestS(5);

    func(&t);
    
    return 0;
}