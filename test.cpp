#include <vector>
#include <iostream>

struct A {};

struct S : A {
    int a = 0;
    int b;
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

    return 0;
}