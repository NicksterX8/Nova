#include <iostream>
#include "math.h"
#include "stdlib.h"
 
using namespace std;

struct S {};

int counter = 0;

template<class T>
constexpr int getRawID() {
    return -1;
}

template<>
constexpr int getRawID<const int>() {
    return 0;
}

template<>
constexpr int getRawID<const float>() {
    return 1;
}

template<class T>
constexpr int getID() {
    return getRawID<const T>();
}

template<class T>
int getWID() {
    cout << "id1: " << getID<T&>() << "\n";
    cout << "id2: " << getID<const T>() << "\n";
    return getID<T&>();
}

template<class T>
constexpr bool tIsMutable() {
    return (getID<T>() != -1) && getRawID<T>() == -1;
}

template<class T>
constexpr bool tIsConst() {
    return !tIsMutable<T>();
}

template<class T1, class T2, class Func>
void doCallback(Func callback) {
    int id1 = getID<T1>();
    int id2 = getID<T2>();

    cout << "callback id1: " << id1 << " id2: " << id2 << "\n";

    if (tIsConst<T1>()) {
        cout << "t1 is const\n";
    }
    if (tIsMutable<T2>()) {
        cout << "t2 is mutable\n";
    }

    cout << "id1: " << id1 << " id2: " << id2 << "\n";
    T1* t1 = new T1();
    T2* t2 = new T2();
    callback(*t1, *t2);
}

template<class T>
const T* ptr() {
    return new T();
}

void myCallback(const int& i, const float& f) {
    cout << "callback!\n";
}

int main() {
    int a = getID<int>();
    int f = getID<float>();
    int ca = getID<const int>();

    cout << "a: " << a << " ca: " << ca << "\n";

    doCallback<int, const float>(myCallback);

    const int* p = ptr<const int>();

    return 0;
}