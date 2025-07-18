#pragma once

#include <atomic>
#include <cassert>

#include "Spinlock.hpp"

class AtomicCountdown {
    std::atomic<int> counter;
public:
    AtomicCountdown() {}

    AtomicCountdown(int countDownFrom) : counter(countDownFrom) {}

    void increment(int count = 1) {
        counter.fetch_add(count, std::memory_order_relaxed);
    }

    // returns the count before it was subtracted
    int decrement(int count = 1) {
        int countBefore = counter.fetch_sub(count, std::memory_order_relaxed);
        assert(countBefore - count >= 0 && "Decremented too many times!");
        return countBefore;
    }

    // stall (spin lock) until the counter reaches 0
    void wait() {
        // TODO: use a real spin lock implementation
        while (counter.load(std::memory_order_relaxed) != 0) {}
    }
};