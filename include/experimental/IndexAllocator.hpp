

template<typename Derived, template<typename> class PointerType>
class AllocatorIndexBase : public AllocatorI, public AllocateMethods<Derived>, public DeallocateMethods<Derived> {
    // Required to define to be an allocator:
    // void* allocate(size_t size, size_t alignment)
    // void deallocate(void* ptr, size_t size, size_t alignment)
    // even if deallocation isn't actually needed, define it because of template allocator usage and preferably poison the memory
    // optional:
    // reallocate(void* ptr, size_t oldSize, size_t newSize, size_t alignment)
    // static constexpr bool needDeallocate() if deallocate isn't necessary like a scratch allocator
    // getAllocatorStats() recommended
    // deallocateAll() if possible (not possible for something like Mallocator)

public:
    template<typename T>
    using pointer_type = PointerType<T>;
    using void_pointer_type = PointerType<void>;

    static constexpr bool NeedDeallocate() {
        return true; // default to yes. overrideable by derived if it's not
    }

    template<typename T>
    void deallocate(pointer_type<T> ptr, size_t count = 1) {
        static_cast<Derived*>(this)->deallocate(void_pointer_type(ptr), count * sizeof(T), alignof(T));
    }

    template<typename T>
    void Delete(pointer_type<T> ptr, size_t count = 1) {
        static_cast<Derived*>(this)->destruct(ptr, count);
        deallocate(ptr, count);
    }

    template<typename T>
    pointer_type<T> allocate(size_t count = 1) {
        return pointer_type<T>(static_cast<Derived*>(this)->allocate(count * sizeof(T), alignof(T)));
    }

    void_pointer_type reallocate(void_pointer_type ptr, size_t oldSize, size_t newSize, size_t alignment) {
        void_pointer_type newPtr = static_cast<Derived*>(this)->allocate(newSize, alignment);
        size_t sharedSize = (oldSize < newSize) ? oldSize : newSize;
        static_cast<Derived*>(this)->copy(newPtr, ptr, sharedSize);
        static_cast<Derived*>(this)->deallocate(ptr, oldSize, alignment);
        return newPtr;
    }

    /* Convenience methods for type allocations */

    template<typename T>
    pointer_type<T> reallocate(pointer_type<T> ptr, size_t oldCount, size_t newCount) {
        return pointer_type<T>(static_cast<Derived*>(this)->reallocate(void_pointer_type(ptr), oldCount * sizeof(T), newCount * sizeof(T), alignof(T)));
    }

    static constexpr size_t goodSize(size_t minSize) {
        return minSize;
    }

    template<typename T>
    static constexpr size_t goodSize(size_t minCount) {
        return Derived::goodSize(minCount * sizeof(T)) / sizeof(T);
    }
};

class TestIndexAllocator;

template<typename T>
struct PointerType {
    uint32_t index;
protected:
    constexpr PointerType(uint32_t index) : index(index) {} 

    // operator uint32_t() const {
    //     return index;
    // }

    operator uintptr_t() const {
        return index;
    }
public:
    constexpr PointerType() {}

    template<typename U>
    constexpr explicit PointerType(PointerType<U> voidPtr) : index(voidPtr.index) {}

    PointerType<T> operator+(PointerType<T> rhs) const {
        return index + rhs.index;
    }

    PointerType<T>& operator+=(PointerType<T> rhs) const {
        index += rhs.index;
        return *this;
    }
    
    T* operator+(void* realPtr) {
        return (T*)(realPtr + index);
    }

    T* operator+(char* realPtr) {
        return (T*)(realPtr + index);
    }
private:
    friend class TestIndexAllocator;
};

/* Default allocator using malloc/free */
class TestIndexAllocator : public AllocatorIndexBase<TestIndexAllocator, PointerType> {
    using Base = AllocatorIndexBase<TestIndexAllocator, PointerType>;

    char* start;
    void_pointer_type current;
public:
    template<typename T>
    using pointer_type = PointerType<T>;
    using void_pointer_type = PointerType<void>;
    static constexpr void_pointer_type null = void_pointer_type(std::numeric_limits<uint32_t>::max());

    TestIndexAllocator(size_t size) {
        start = (char*)malloc(size);
        current = 0;
    }

    void* get(void_pointer_type pointer) const {
        if (pointer == null) return nullptr;
        return start + pointer.index;
    }

    template<typename T>
    T* get(pointer_type<T> pointer) const {
        if (pointer == null) return nullptr;
        return (T*)(start + pointer);
    }

    void_pointer_type allocate(size_t size, size_t alignment) {
        void_pointer_type ptr = current;
        current.index += size;
        return ptr;
    }

    using Base::allocate;

    void deallocate(void_pointer_type ptr, size_t size, size_t alignment) {
        
    }

    using Base::deallocate;


    void copy(void_pointer_type dst, void_pointer_type src, size_t size) {
        memcpy(get(dst), get(src), size);
    }

    template<typename T>
    void destruct(pointer_type<T> elements, size_t count) {
        for (int i = 0; i < count; i++) {
            T* address = get<T>(elements) + i;
            address->~T();
        }
    }

    static constexpr size_t goodSize(size_t minSize) {
    #ifdef MACOS
        return malloc_good_size(minSize);
    #else
        return minSize;
    #endif
    }

    AllocatorStats getAllocatorStats() const {
        return {
            .name = "mallocator"
        };
    }
};

template<typename T, typename Allocator>
struct VecSupportingIndexAllocator {
    using realAllocator = std::remove_all_extents_t<Allocator>;
    using pointer = typename realAllocator::template pointer_type<T>;
    static constexpr pointer null = pointer(Allocator::null);

    pointer data = null;
    int size = 0;
    int capacity = 0;
    Allocator allocator;

    VecSupportingIndexAllocator() {}

    VecSupportingIndexAllocator(Allocator& allocator) : allocator(allocator) {}

    void reserve(int newCapacity) {
        data = allocator.reallocate(data, capacity, newCapacity);
    }

    void push_back(T val) {
        if (size >= capacity) reserve(size+1);
        getData()[size++] = val;
    }

    T* getData() const {
        return allocator.get(data);
    }

    T& operator[](int index) const {
        return getData()[index];
    }
};