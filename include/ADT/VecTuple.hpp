#ifndef VEC_TUPLE_INCLUDED
#define VEC_TUPLE_INCLUDED

#include "memory/Allocator.hpp"

template<typename T0, typename T1, typename Allocator = Mallocator>
class VecTuple {
    void* _data = nullptr;
    int _size = 0;
    int _capacity = 0;
    Allocator* allocator;

    static constexpr int HarshestAlignment = std::max(alignof(T0), alignof(T1));
    static constexpr int MaxAlignmentPadding = alignof(T1) > alignof(T0) ? alignof(T1) - alignof(T0) : alignof(T0) - alignof(T1);
    static constexpr int CombinedSize = sizeof(T0) + sizeof(T1);
public:
    VecTuple(Allocator* allocator) : allocator(allocator) {}

    struct Tuple {
        T0 first;
        T1 second;

        template<typename T>
        T get() {
            if constexpr (std::is_same_v<T, T0>) {
                return first;
            } else if constexpr (std::is_same_v<T, T1>) {
                return second;
            } else {
                static_assert(0, "Type is not in vec tuple!");
            }
        }
    };

    template<typename T>
    T* data() {
        if constexpr (std::is_same_v<T, T0>) {
            return (T0*)_data;
        } else if constexpr (std::is_same_v<T, T1>) {
            return getT1(_data, _capacity);
        }
    }

    template<typename T>
    const T* data() const {
        if constexpr (std::is_same_v<T, T0>) {
            return (const T0*)_data;
        } else if constexpr (std::is_same_v<T, T1>) {
            return getT1(_data, _capacity);
        }
    }

    template<typename T>
    T& get(int index) {
        return data<T>()[index];
    }

    template<typename T>
    const T& get(int index) const {
        return data<T>()[index];
    }

    Tuple operator[](int index) const {
        return {get<T0>(index), get<T1>(index)};
    }

    void pop() {
        assert(_size > 0 && "can't pop element of empty vector");
        _size--;
    }

    bool empty() const {
        return _size == 0;
    }

    void push_back(const T0& t0, const T1& t1) {
        if (_size+1 > _capacity) {
            int newCapacity = _capacity*2 > _size+1 ? _capacity*2 : _size+1;
            reallocate(newCapacity);
        }
        get<T0>(_size) = t0;
        get<T1>(_size) = t1;
        ++_size;
    }

    void append(int count, const T0* t0s, const T1* t1s) {
        if (_size + count > _capacity) {
            int newCapacity = _capacity*2 > _size+count ? _capacity*2 : _size+count;
            reallocate(newCapacity);
        }
        std::copy(t0s, t0s + count, data<T0>() + _size);
        std::copy(t1s, t1s + count, data<T1>() + _size);
        _size += count;
    }

    // reserve atleast capacity
    // \returns the new capacity of the vec
    int reserve(int capacity) {
        assert(_capacity >= 0 && "cannot have negative capacity");
        // parameter capacity shadows this->capacity, so have to explicitly use 'this'
        if (this->_capacity < capacity) {
            reallocate(this->_capacity*2 > capacity ? this->_capacity*2 : capacity);
        }
        return this->_capacity;
    }
 
    // resize the vector
    void resize(int size) {
        assert(size >= 0 && "cannot have negative size");
        reserve(size);
        this->_size = size;
    }

    int getBytesAllocated(int capacity) {
        if (capacity == 0) return 0;
        return capacity * CombinedSize + MaxAlignmentPadding;
    }

    T1* getT1(void* data, int capacity) const {
        return (T1*)alignPtr((T1*)((T0*)data + capacity), alignof(T1));
    }

    /* Resize the vector to the given new capacity. 
     * If the current size of the vector is greater than the new capacity, the size will reduced to the new capacity,
     * losing any elements past index 'newCapacity' - 1.
     * @return The capacity of the vector after resizing. If an error hasn't occured, this will be equal to 'newSize'.
     * When the new capacity couldn't be allocated, -1 will be returned
     */
    void reallocate(int newCapacity) {
        void* newData = allocator->allocate(getBytesAllocated(newCapacity), HarshestAlignment);
        std::copy(data<T0>(), data<T0>() + _size, (T0*)newData);
        auto t1Data = data<T1>();
        std::copy(t1Data, t1Data + _size, getT1(newData, newCapacity));
        allocator->deallocate(_data, getBytesAllocated(_capacity), HarshestAlignment);
        _data = newData;
        _size = (_size < newCapacity) ? _size : newCapacity;
        _capacity = newCapacity;
    }
private:
    void deallocate() {
        allocator->deallocate(_data, getBytesAllocated(_capacity), HarshestAlignment);
    }
public:
    void clear() {
        deallocate();
        _data = nullptr;
        _size = 0;
        _capacity = 0;
    }

    void destroy() {
        deallocate();
    }
};

#endif