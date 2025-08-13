#ifndef MY_ITERATION_INCLUDED
#define MY_ITERATION_INCLUDED

namespace My {

template<typename T>
struct ReverseAdapter {
    using iterator = typename T::iterator;
    using reverse_iterator = typename T::reverse_iterator;

    ReverseAdapter(const T& c) : c(c) { }
    reverse_iterator begin() const { return {c.rbegin()}; }
    reverse_iterator end() const { return {c.rend()}; }

    const T& c;
};

template<typename T>
ReverseAdapter<T> reverse(const T& c) {
    return ReverseAdapter<T>(c);
}

}

#endif