#ifndef ALLOCATOR_VEC_INCLUDED
#define ALLOCATOR_VEC_INCLUDED

namespace My {

namespace AllocatorADT {

/* A simple vector implementation for POD ONLY types!
 * Constructors and destructors will not be called!
 * Should be faster and smaller than std::vector most of the time, 
 * and result in faster compile times
 */ 
template<typename T, typename Allocator>
struct Vec {
    static_assert(std::is_trivially_move_assignable<T>::value && std::is_trivially_move_constructible<T>::value, "vec doesn't support complex types");
    using Type = T;

    T* data;
    int size;
    int capacity;
    AllocatorHolder<Allocator> allocator;

private: 
    using Self = Vec<T, Allocator>;
    using ValueParamT = FastestParamType<T>;
public:

    Vec() = default;

    // inits to empty
    template<typename AllocT>
    Vec(AllocT&& allocator)
    : data(nullptr), size(0), capacity(0), allocator(std::forward<AllocT&&>(allocator)) {}

    inline static Self WithCapacity(Allocator&& allocator, int capacity) {
        return Self(allocator, capacity);
    }

    /* Create a vector with the given size and copy the given data to the vector
     */
    template<typename AllocT>
    Vec(AllocT&& allocator, const T* _data, int _size) : allocator(std::forward<AllocT&&>(allocator)) {
        data = this->allocator.get().template allocate<T>(_size);
        memcpy(data, _data, _size * sizeof(T));
        size = _size;
        capacity = _size;
    }

    inline T* get(int index) const {
        if (data && index < size && index > -1) {
            return &data[index];
        }
        return nullptr;
    }

    inline T& operator[](int index) const {
#ifdef BOUNDS_CHECKS
        assert(index < size    && "vector index out of bounds");
        assert(index > -1      && "vector index out of bounds");
#endif
        assert(data != nullptr && "access of null vector");
        return data[index];
    }

    void push(ValueParamT val) {
        /*
        reserve(size+1)
        memcpy(&data[size], &val, sizeof(T));
        */
        //return vec_push((Generic::Vec*)this, sizeof(T), &val);
        if (size+1 > capacity) {
            int newCapacity = capacity*2 > size+1 ? capacity*2 : size+1;
            data = allocator.get().reallocate(data, capacity, newCapacity);
            capacity = newCapacity;
        }
        memcpy(data + size, &val, sizeof(T));
        ++size;
    }

    void pushFront(const T* values, int count) {
        assert(count > 0 && "can't push 0 or less values!");
        if (size + count > capacity) {
            // reallocate. Fortunately this makes the algorithm a lot simpler. just copy the new values in, then the old values
            int newCapacity = MAX(size + count, capacity*2);
            T* newData = allocator->template allocate<T>((size_t)newCapacity);
            memcpy(&newData[0], values, (size_t)count * sizeof(T));
            memcpy(&newData[count], data, (size_t)size * sizeof(T));
            allocator.get().deallocate(data, capacity);
            data = newData;
            size += count;
            capacity = newCapacity;
        } else {
            // no allocation version, a bit more complicated
            // move all current elements forward at 'count' sized batches
            int elementsMoved = 0;
            while (elementsMoved < size) {
                int elementsLeftToMove = size - elementsMoved;
                int batchSize = elementsLeftToMove < count ? elementsLeftToMove : count;
                memcpy(&data[elementsMoved], &data[size - elementsMoved - 1], (size_t)batchSize * sizeof(T));
                elementsMoved += batchSize;
            }

            // copy new elements into now unused space at beginning
            memcpy(&data[0], values, (size_t)count * sizeof(T));
            size += count;
        }
    }

    void pushFront(ValueParamT val) {
        pushFront(&val, 1);
    }

    void pop() {
        assert(size > 0 && "can't pop element of empty vector");
        size--;
    }

    T popBack() {
        assert(size > 0 && "can't pop back element of empty vector");
        return data[--size];
    }

    bool empty() const {
        return size == 0;
    }

    // reserve atleast capacity
    // \returns the new capacity of the vec
    int reserve(int capacity) {
        assert(capacity >= 0 && "cannot have negative capacity");
        // parameter capacity shadows this->capacity, so have to explicitly use 'this'
        if (this->capacity < capacity) {
            reallocate(this->capacity*2 > capacity ? this->capacity*2 : capacity);
        }
        return this->capacity;
    }
 
    // resize the vector
    void resize(int size) {
        assert(size >= 0 && "cannot have negative size");
        reserve(size);
        this->size = size;
    }

    void resize(int size, ValueParamT value) {
        assert(size >= 0 && "cannot have negative size");
        reserve(size);
        for (int i = this->size; i < size; i++) {
            memcpy(&data[i], &value, sizeof(T));
        }
        this->size = size;
    }

    /* Resize the vector to the given new capacity. 
     * If the current size of the vector is greater than the new capacity, the size will reduced to the new capacity,
     * losing any elements past index 'newCapacity' - 1.
     * @return The capacity of the vector after resizing. If an error hasn't occured, this will be equal to 'newSize'.
     * When the new capacity couldn't be allocated, -1 will be returned
     */
    void reallocate(int newCapacity) {
        data = allocator->reallocate(data, capacity, newCapacity);
        size = (size < newCapacity) ? size : newCapacity;
        capacity = newCapacity;
    }

    void clear() {
        allocator->deallocate(data, size);
        data = nullptr;
        size = 0;
        capacity = 0;
    }

    void destroy() {
        clear();
    }

    T& back() const {
       assert(size > 0 && "can't get element of empty vector"); 
       return data[size-1];
    }

    T& front() const {
       assert(size > 0 && "can't get element of empty vector"); 
       return data[0];
    }

    void remove(int index) {
        assert(index < size && "vector index out of bounds");
        assert(index > -1    && "vector index out of bounds");

        for (int i = index; i < size-1; i++) {
            memcpy(&data[i], &data[i+1], sizeof(T));
        }

        size--;
    }

    void push(const T* elements, int count) {
        reserve(size + count);
        memcpy(&data[size], elements, count * sizeof(T));
        size += count;
    }

    void push(ArrayRef<T> array) {
        push(array.data(), array.size());
    }

    /* Reserve space for \p size elements at the back and increase the vector's size by that amount, leaving the elements in those spaces unitialized.
     * Like a push but without initialization.
     * \return The address of the elements required.
     */
    T* require(int size) {
        reserve(this->size + size);
        this->size += size;
        return &data[this->size - size];
    }

    /* Shrink the vector capacity by count number of elements, if possible.
     * If count is greater than capacity - size, count will be set to capacity - size
     */
    void shrink(int count) {
        count = std::min(count, capacity - size);
        if (count > 0) {
            data = allocator->reallocate(data, capacity, capacity - count);
            capacity -= count;
        }
    }

    /* Shrink capacity to size
     */
    int shrinkToFit() {
        if (capacity > size) {
            data = allocator->reallocate(data, capacity, size);
            capacity = size;
        }
    }

    void insertAt(int index, const T* values, int count) {
        T* at = &data[index];
        if (size + count > capacity) {
            // reallocate. Fortunately this makes the algorithm a lot simpler. just copy the new values in, then the old values
            int newCapacity = MAX(size + count, capacity*2);
            T* newData = allocator->template allocate<T>(newCapacity);
            // copy old data before index to range 0 - index
            memcpy(&newData[0], &data[0], index * sizeof(T));
            // copy values to range index - index + count
            memcpy(&newData[index], values, count * sizeof(T));
            // copy old data after index to range index + count -  size + count
            memcpy(&newData[index + count], &data[index], (size - index) * sizeof(T));

            allocator->deallocate(data, size);
            data = newData;
            size += count;
            capacity = newCapacity;
        } else {
            // no allocation version, a bit more complicated
            // move all current elements forward at 'count' sized batches
            int elementsMoved = 0;
            while (elementsMoved < size) {
                int elementsLeftToMove = size - elementsMoved;
                int batchSize = elementsLeftToMove < count ? elementsLeftToMove : count;
                memcpy(&at[elementsMoved], &at[size - elementsMoved - 1], batchSize * sizeof(T));
                elementsMoved += batchSize;
            }

            // copy new elements into now unused space at beginning
            memcpy(&at[0], values, count * sizeof(T));
        }
    }

    template<typename T2>
    Vec<T2, Allocator> cast() const {
        return Vec<T2, Allocator>(data, size, capacity);
    }

    using iterator = T*; // no iterator bs, just pointer

    inline T* begin() const { return data; }
    inline T* end() const { return data + size; }
};

}

template<typename T, typename A>
using VecWithAllocator = AllocatorADT::Vec<T, A>;

}

#endif