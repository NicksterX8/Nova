#pragma once

#include <stddef.h>

// Allocates memory on the stack, while keeping the heap as a backup if the size is larger than a certain size
// StackElements: The number of elements to allocate on the stack
template<typename T, size_t StackElements>
struct StackAllocate {
    constexpr static int NumStackElements = StackElements;
    static_assert(NumStackElements > 0, "Max struct size too small to hold any elements!");
private:
    std::array<T, NumStackElements> stackElements;
    T* _data;

    using Me = StackAllocate<T, StackElements>;
public:
    StackAllocate(int size) {
        if (size <= NumStackElements) {
            _data = stackElements.data();
        } else {
            _data = new T[size];
        }
    }

    StackAllocate(int size, const T& initValue) {
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

    operator T*() {
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
template<typename T, size_t StackElements, typename Allocator>
struct StackAllocateWithAllocator {
    constexpr static size_t NumStackElements = StackElements;
    static_assert(NumStackElements > 0, "Max struct size too small to hold any elements!");
private:
    std::array<char, std::max(sizeof(T*), NumStackElements)> stackStorage;
    size_t size;
    Allocator* _allocator;

    void setHeapData(T* data) {
        T** stackStoragePtr = (T**)stackStorage.data();
        *stackStoragePtr = data;
    }

    T* getHeapData() {
        return *(T**)stackStorage.data();
    }
public:
    StackAllocateWithAllocator(size_t size, Allocator* allocator) : _allocator(allocator) {
        setData(_allocator->template allocate<T>(size));
    }

    StackAllocateWithAllocator(size_t size, const T& initValue, Allocator* allocator) : _allocator(allocator) {
        T* data;
        if (size <= NumStackElements) {
            data = stackStorage.data();
        } else {
            data = _allocator->template allocate<T>(size);
            setHeapData(data);
        }

        for (int i = 0; i < size; i++) {
            data[i] = initValue;
        }
    }

    T& operator[](int index) {
        return data()[index];
    }

    T* data() {
        if (size <= NumStackElements) {
            return stackStorage.data();
        }
        return getHeapData();
    }

    ~StackAllocateWithAllocator() {
        if (size > StackElements) {
            _allocator->deallocate(getHeapData(), size);
        }
    }
};