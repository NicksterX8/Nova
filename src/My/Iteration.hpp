#ifndef MY_ITERATION_INCLUDED
#define MY_ITERATION_INCLUDED

#include "MyInternals.hpp"

MY_CLASS_START

template<typename T>
struct ReverseAdapter {
    using iterator = typename T::iterator;
    struct reverse_iterator {
        iterator iter;
        auto operator*() const {
            return *iter;
        }
        reverse_iterator operator++() {
            return {--iter};
        }
        bool operator!=(reverse_iterator other) const {
            return iter != other.iter;
        }
    };

    ReverseAdapter(const T& c) : c(c) { }
    reverse_iterator begin() const { return {c.rbegin()}; }
    reverse_iterator end() const { return {c.rend()}; }

    const T& c;
};

template<typename T>
ReverseAdapter<T> reverse(const T& c) {
    return ReverseAdapter<T>(c);
}

MY_CLASS_END

#endif