#ifndef CONCURRENCY_INCLUDED
#define CONCURRENCY_INCLUDED

#include <atomic>

template<typename T>
struct FixedCapacityConcurrentPushStack {
private:
    T* data;
    size_t capacity;
    std::atomic<uint64_t> count;
public:
    MultiProducerStack(T* data, size_t capacity) : data(data), capacity(capacity), count(0) {}

    void push(const T& value) {
        uint64_t countMinusOne = count.fetch_add(1, std::memory_order_relaxed);
        // now that we reserved an element, its safe to do whatever we want with the element
        assert(countMinusOne + 1 <= capacity && "Ran out of storage in fixed capacity stack");
        data[countMinusOne] = value;
    }

    uint64_t main_getCount() {
        return count.load();
    }

    T* data() {
        return data;
    }
};

#endif