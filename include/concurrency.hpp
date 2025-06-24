#ifndef CONCURRENCY_INCLUDED
#define CONCURRENCY_INCLUDED

#include <atomic>

template<typename T>
struct MultiPushOnlyStack {
    T* data;
    size_t capacity;
    std::atomic<uint64_t> count;

    MultiProducerStack(T* data, size_t capacity) : data(data), capacity(capacity), count(0) {}

    void push(const T& value) {
        uint64_t currentCount = count.load();
        while (count.compare_exchange_weak(currentCount, currentCount + 1)) {}
    }
};

#endif