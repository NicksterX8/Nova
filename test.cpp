#include <vector>
#include <iostream>
#include <bitset>

struct A {};

struct S : A {
    int a = 0;
    int b;
};

template<std::size_t N>
struct Flags {
    Uint64 bits[N/64];

    operator& {

    }
};

int main() {
    S s = {
        .a = 2,
        .b = 3
    };

    std::vector<int> vec;
    vec.push_back(0);
    vec.push_back(1);
    vec.push_back(2);

    std::cout << "front: " << vec.front() << ", back: " << vec.back() << "\n";

    vec.pop_back();

    std::cout << "front: " << vec.front() << ", back: " << vec.back() << "\n";

    std::bitset<128> bitset;
    std::bitset<128> bitset2;
    bitset |= 2;
    std::bitset bitset3 = bitset & bitset2;

    return 0;
}