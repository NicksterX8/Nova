#ifndef UTILS_ALLOCATORS_INCLUDED
#define UTILS_ALLOCATORS_INCLUDED

#include <stddef.h>

// Allocates memory on the stack, while keeping the heap as a backup if the size is larger than a certain size
// StackElements: The number of elements to allocate on the stack
template<typename T, size_t StackElements>
struct StackAllocate {
    constexpr static size_t NumStackElements = StackElements;
    static_assert(NumStackElements > 0, "Max struct size too small to hold any elements!");
private:
    std::array<T, NumStackElements> stackElements;
    T* _data;

    using Me = StackAllocate<T, StackElements>;
public:
    StackAllocate(size_t size) {
        if (size <= NumStackElements) {
            _data = stackElements.data();
        } else {
            _data = new T[size];
        }
    }

    StackAllocate(size_t size, const T& initValue) {
        if (size <= NumStackElements) {
            _data = stackElements.data();
        } else {
            _data = new T[size];
        }

        for (int i = 0; i < size; i++) {
            _data[i] = initValue;
        }
    }

    StackAllocate(const Me& other) = delete;

    T* data() {
        return _data;
    }

    T& operator[](int index) {
        return _data[index];
    }

    ~StackAllocate() {
        if (_data != stackElements.data()) {
            delete[] _data;
        }
    }
};

// Allocates memory on the stack, while keeping the heap as a backup if the size is larger than a certain size
// MaxStructSize: Maximum number of bytes the struct will take up
template<typename T, size_t StackElements, typename A>
struct StackAllocateWithAllocator {
    constexpr static size_t NumStackElements = StackElements;
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