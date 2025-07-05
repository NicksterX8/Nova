#ifndef UTILS_ALLOCATORS_INCLUDED
#define UTILS_ALLOCATORS_INCLUDED

#include <stddef.h>

// Allocates memory on the stack, while keeping the heap as a backup if the size is larger than a certain size
// MaxStructSize: Maximum number of bytes the struct will take up
template<typename T, size_t MaxStructSize = 128>
struct StackAllocate {
    constexpr static size_t NumStackElements = MaxStructSize / sizeof(T);
    static_assert(NumStackElements > 0, "Max struct size too small to hold any elements!");
private:
    std::array<T, NumStackElements> stackElements;
    T* _data;
public:
    StackAllocate(size_t size) {
        if (size <= NumStackElements) {
            _data = stackElements.data();
        } else {
            _data = new T[size];
        }
    }

    T* data() {
        return _data;
    }

    ~StackAllocate() {
        if (_data != stackElements.data()) {
            delete[] _data;
        }
    }
};

// Allocates memory on the stack, while keeping the heap as a backup if the size is larger than a certain size
// MaxStructSize: Maximum number of bytes the struct will take up
template<typename T, size_t MaxStructSize = 128, typename A>
struct StackAllocate {
    constexpr static size_t NumStackElements = MaxStructSize / sizeof(T);
    static_assert(NumStackElements > 0, "Max struct size too small to hold any elements!");
private:
    std::array<T, NumStackElements> stackElements;
    T* _data;
    A* _allocator;
public:
    StackAllocate(size_t size, A* allocator) : _allocator(allocator) {
        if (size <= NumStackElements) {
            _data = stackElements.data();
        } else {
            _data = _allocator->Alloc<T>(size);
        }
    }

    T* data() {
        return _data;
    }

    ~StackAllocate() {
        if (_data != stackElements.data()) {
            _allocator->Free(_data);
        }
    }
};



#endif